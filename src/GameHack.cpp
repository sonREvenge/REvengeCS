#include "GameHack.hpp"

#include <windows.h>
#include <cmath>
#include <vector>

#include "imgui.h"

#include "OffsetScanner.hpp"
#include "Settings.hpp"

namespace Overlay
{
	inline void Box(float x, float y, float width, float height, ImU32 color, float thickness = 1.0f, bool filled = false)
	{
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		if (filled) {
			drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), color);
		}
		else {
			drawList->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), color, 0.0f, 0, thickness);
		}
	}

	inline void Line(float x1, float y1, float x2, float y2, ImU32 color, float thickness = 1.0f)
	{
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
	}

	inline void Circle(float x, float y, float radius, ImU32 color, int segments = 32, float thickness = 1.0f, bool filled = false)
	{
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		if (filled) {
			drawList->AddCircleFilled(ImVec2(x, y), radius, color, segments);
		}
		else {
			drawList->AddCircle(ImVec2(x, y), radius, color, segments, thickness);
		}
	}
}

struct Vector3
{
	float x, y, z;
};

struct Vector2
{
	float x, y;
};

struct Matrix4x4
{
	float m[4][4];
};

static bool WorldToScreen(Vector3 pos, Vector2& screen, Matrix4x4 matrix, float width, float height)
{
	float w = matrix.m[3][0] * pos.x + matrix.m[3][1] * pos.y + matrix.m[3][2] * pos.z + matrix.m[3][3];
	if (w < 0.01f)
		return false;

	float invW = 1.0f / w;
	float x = (matrix.m[0][0] * pos.x + matrix.m[0][1] * pos.y + matrix.m[0][2] * pos.z + matrix.m[0][3]) * invW;
	float y = (matrix.m[1][0] * pos.x + matrix.m[1][1] * pos.y + matrix.m[1][2] * pos.z + matrix.m[1][3]) * invW;

	screen.x = (width / 2.0f) * (1.0f + x);
	screen.y = (height / 2.0f) * (1.0f - y);

	return true;
}

enum BONEINDEX : int
{
	ROOT = 0,
	PELVIS = 1,
	SPINE1 = 2,
	SPINE2 = 4,
	CHEST = 12,
	NECK = 6,
	HEAD = 7,

	SHOULDER_L = 9,
	ELBOW_L = 10,
	HAND_L = 11,

	SHOULDER_R = 13,
	ELBOW_R = 14,
	HAND_R = 15,

	HIP_L = 17,
	KNEE_L = 18,
	FOOT_HEEL_L = 19,

	HIP_R = 20,
	KNEE_R = 21,
	FOOT_HEEL_R = 22
};

struct BoneJointData
{
	Vector3 Pos;
	char pad[0x14]; // 20 bytes padding to complete the 32-byte stride
};

struct BoneJointPos
{
	Vector3 Pos;
	Vector2 ScreenPos;
};

// Resolves an entity handle (as returned by fields like m_hPlayerPawn) to its
// current entity pointer via the two-level entity list CS2 uses internally.
static uintptr_t ResolveHandle(uintptr_t entityList, uint32_t handle) {
	uintptr_t listEntry = mem.Read<uintptr_t>(entityList + (8 * ((handle & 0x7FFF) >> 9)) + 16);
	if (!listEntry) return 0;
	return mem.Read<uintptr_t>(listEntry + 112 * (handle & 0x1FF));
}

void HackGame() {
	float screenW = (float)GetSystemMetrics(SM_CXSCREEN);
	float screenH = (float)GetSystemMetrics(SM_CYSCREEN);

	Matrix4x4 viewMatrix = mem.Read<Matrix4x4>(module_base + (g_ovViewMatrix ? g_ovViewMatrix : offsets::dwViewMatrix));
	uintptr_t entityList = mem.Read<uintptr_t>(module_base + (g_ovEntityList ? g_ovEntityList : offsets::dwEntityList));

	int localTeam = 0;
	uintptr_t localPawn = 0;
	if (entityList) {
		uintptr_t localController = mem.Read<uintptr_t>(module_base + (g_ovLocalController ? g_ovLocalController : offsets::dwLocalPlayerController));
		if (localController) {
			uint32_t localPawnHandle = mem.Read<uint32_t>(localController + OffPlayerPawnHandle());
			if (localPawnHandle) {
				localPawn = ResolveHandle(entityList, localPawnHandle);
			}
		}
	}

	if (localPawn) {
		localTeam = mem.Read<int>(localPawn + OffTeamNum());
	}

	float fovRadius = kAimFovPx;
	float centerX = screenW / 2.0f;
	float centerY = screenH / 2.0f;

	uintptr_t bestTargetPawn = 0;
	float closestDistance = fovRadius; // anything farther than the FOV radius is ignored
	Vector2 bestTargetScreenPos = { 0.0f, 0.0f };

	const int targetBoneId = HEAD;

	if (g_ESPFovCircle) {
		Overlay::Circle(centerX, centerY, fovRadius, IM_COL32(0, 255, 255, 150), 32, 1.0f);
	}

	if (entityList)
	{
		for (int i = 1; i < 64; i++)
		{
			uintptr_t controller = ResolveHandle(entityList, (uint32_t)i);
			if (controller < 0x1000000 || (controller >> 48) != 0) continue;

			uint32_t pawnHandle = mem.Read<uint32_t>(controller + OffPlayerPawnHandle());
			if (!pawnHandle) continue;

			uintptr_t pawn = ResolveHandle(entityList, pawnHandle);
			if (!pawn) {
				continue;
			}

			if (pawn == localPawn)
			{
				continue;
			}

			int health = mem.Read<int>(pawn + OffHealth());
			int team = mem.Read<int>(pawn + OffTeamNum());

			if (health <= 0 || health > 1000) continue;

			if (g_ESPTeamCheck && team == localTeam && team != 0) {
				continue;
			}

			uintptr_t gameSceneNode = mem.Read<uintptr_t>(pawn + OffGameSceneNode());
			if (gameSceneNode)
			{
				uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + OffModelState() + 0x80);

				if (boneArray)
				{
					struct bone_link { int a, b; };

					static const bone_link local_skeleton_links[] = {
						{ PELVIS, SPINE1 }, { SPINE1, SPINE2 }, { SPINE2, CHEST }, { CHEST, NECK }, { NECK, HEAD },

						{ NECK, SHOULDER_L }, { SHOULDER_L, ELBOW_L }, { ELBOW_L, HAND_L },

						{ NECK, SHOULDER_R }, { SHOULDER_R, ELBOW_R }, { ELBOW_R, HAND_R },

						{ PELVIS, HIP_L }, { HIP_L, KNEE_L }, { KNEE_L, FOOT_HEEL_L },

						{ PELVIS, HIP_R }, { HIP_R, KNEE_R }, { KNEE_R, FOOT_HEEL_R }
					};

					std::vector<BoneJointPos> bonePosList(30, BoneJointPos{});

					unsigned int targetedBones[] = {
						ROOT, PELVIS, SPINE1, SPINE2, CHEST, NECK, HEAD,
						SHOULDER_L, ELBOW_L, HAND_L,
						SHOULDER_R, ELBOW_R, HAND_R,
						HIP_L, KNEE_L, FOOT_HEEL_L,
						HIP_R, KNEE_R, FOOT_HEEL_R
					};

					for (unsigned int id : targetedBones)
					{
						uintptr_t jointAddress = boneArray + (id * sizeof(BoneJointData));
						BoneJointData rawJoint = mem.Read<BoneJointData>(jointAddress);

						Vector2 screenCoords;
						if (WorldToScreen(rawJoint.Pos, screenCoords, viewMatrix, screenW, screenH))
						{
							bonePosList[id].Pos = rawJoint.Pos;
							bonePosList[id].ScreenPos = screenCoords;
						}
					}

					if (g_ESPSkeleton) {
						ImU32 boneColor = IM_COL32(255, 255, 255, 220);
						size_t linkCount = sizeof(local_skeleton_links) / sizeof(local_skeleton_links[0]);

						for (size_t i = 0; i < linkCount; i++)
						{
							unsigned int currentID = local_skeleton_links[i].a;
							unsigned int nextID = local_skeleton_links[i].b;

							Vector2 p1 = bonePosList[currentID].ScreenPos;
							Vector2 p2 = bonePosList[nextID].ScreenPos;

							if ((p1.x != 0 && p1.y != 0) && (p2.x != 0 && p2.y != 0))
							{
								Overlay::Line(p1.x, p1.y, p2.x, p2.y, boneColor, 1.5f);
							}
						}
					}

					Vector2 headPos = bonePosList[HEAD].ScreenPos;
					Vector2 neckPos = bonePosList[NECK].ScreenPos;

					Vector2 rootPose = bonePosList[ROOT].ScreenPos;

					Vector3 rootWorldPos = bonePosList[ROOT].Pos;
					if (rootWorldPos.x == 0 && rootWorldPos.y == 0 && rootWorldPos.z == 0) {
						continue;
					}

					if (g_ESPHeadCircle && headPos.x != 0 && headPos.y != 0)
					{
						float headRadius = abs(headPos.y - neckPos.y);
						Overlay::Circle(headPos.x, headPos.y, headRadius, IM_COL32(230, 50, 50, 255), 16, 1.0f);
					}
					Vector2 targetScreenPos = bonePosList[targetBoneId].ScreenPos;

					float distanceToCrosshair = fovRadius;
					bool projected = (targetScreenPos.x != 0.0f || targetScreenPos.y != 0.0f);
					if (projected)
					{
						float deltaX = targetScreenPos.x - centerX;
						float deltaY = targetScreenPos.y - centerY;
						distanceToCrosshair = sqrtf((deltaX * deltaX) + (deltaY * deltaY));
					}

					if (projected && distanceToCrosshair < fovRadius)
					{
						if (distanceToCrosshair < closestDistance)
						{
							closestDistance = distanceToCrosshair;
							bestTargetPawn = pawn;
							bestTargetScreenPos = targetScreenPos;
						}
					}

					float height = rootPose.y - headPos.y;
					float width = height / 2.0f;
					float x = headPos.x - width / 2.0f;
					float y = headPos.y;

					if (g_ESPBox) {
						Overlay::Box(x, y, width, height, IM_COL32(255, 255, 255, 255), 2.0f);
					}

					if (g_ESPHealth) {
						float healthHeight = (height * health) / 100.0f;
						ImU32 healthColor = IM_COL32(
							(int)(255 - (health * 2.55f)),
							(int)(health * 2.55f),
							0,
							255
						);
						Overlay::Line(x - 5, rootPose.y, x - 5, rootPose.y - healthHeight, healthColor, 2.0f);
					}

					if (g_ESPSnap) {
						Overlay::Line(screenW / 2.0f, screenH, rootPose.x, rootPose.y, IM_COL32(255, 255, 255, 100), 1.0f);
					}

				}
			}

		}

		if (bestTargetPawn != 0)
		{
			Overlay::Line(centerX, centerY, bestTargetScreenPos.x, bestTargetScreenPos.y, IM_COL32(255, 0, 0, 255), 1.0f);

			if (g_AimEnabled && localPawn)
			{
				float moveX = bestTargetScreenPos.x - centerX;
				float moveY = bestTargetScreenPos.y - centerY;
				moveX /= kAimSmoothness;
				moveY /= kAimSmoothness;

				const float kMax = 250.0f;
				if (moveX > kMax) moveX = kMax;
				if (moveX < -kMax) moveX = -kMax;
				if (moveY > kMax) moveY = kMax;
				if (moveY < -kMax) moveY = -kMax;

				INPUT input = { 0 };
				input.type = INPUT_MOUSE;
				input.mi.dwFlags = MOUSEEVENTF_MOVE;
				input.mi.dx = static_cast<LONG>(moveX);
				input.mi.dy = static_cast<LONG>(moveY);
				SendInput(1, &input, sizeof(INPUT));
			}
		}
	}
}
