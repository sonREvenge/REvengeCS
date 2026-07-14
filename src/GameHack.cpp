#include "GameHack.hpp"

#include <windows.h>
#include <cmath>
#include <vector>

#include "imgui.h"

#include "Aimbot.hpp"
#include "OffsetScanner.hpp"
#include "Settings.hpp"
#include "VisibleCheck.hpp"

namespace Overlay
{
	constexpr ImU32 kShadow    = IM_COL32(0, 0, 0, 190);
	constexpr ImU32 kShadowSoft = IM_COL32(0, 0, 0, 130);

	inline ImDrawList* DL() { return ImGui::GetBackgroundDrawList(); }

	inline void ShadowLine(float x1, float y1, float x2, float y2, ImU32 color, float thickness = 1.0f)
	{
		ImDrawList* dl = DL();
		dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), kShadow, thickness + 1.5f);
		dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color,   thickness);
	}

	inline void ShadowCircle(float x, float y, float radius, ImU32 color, int segments = 40, float thickness = 1.0f)
	{
		ImDrawList* dl = DL();
		dl->AddCircle(ImVec2(x, y), radius, kShadow, segments, thickness + 1.0f);
		dl->AddCircle(ImVec2(x, y), radius, color,   segments, thickness);
	}

	inline void Dot(float x, float y, float radius, ImU32 color)
	{
		ImDrawList* dl = DL();
		dl->AddCircleFilled(ImVec2(x, y), radius + 1.0f, kShadow, 12);
		dl->AddCircleFilled(ImVec2(x, y), radius,        color,   12);
	}

	inline void ShadowText(float x, float y, ImU32 color, const char* text)
	{
		ImDrawList* dl = DL();
		dl->AddText(ImVec2(x + 1, y + 1), kShadow, text);
		dl->AddText(ImVec2(x, y),         color,   text);
	}

	inline void CornerBox(float x, float y, float w, float h, ImU32 color, float thickness = 1.4f)
	{
		if (w <= 0 || h <= 0) return;
		ImDrawList* dl = DL();
		float cl = h * 0.22f;
		if (cl > w * 0.4f) cl = w * 0.4f;

		auto stroke = [&](ImVec2 a, ImVec2 b) {
			dl->AddLine(a, b, kShadow, thickness + 1.5f);
			dl->AddLine(a, b, color,   thickness);
		};

		stroke(ImVec2(x, y),         ImVec2(x + cl, y));
		stroke(ImVec2(x, y),         ImVec2(x, y + cl));

		stroke(ImVec2(x + w, y),     ImVec2(x + w - cl, y));
		stroke(ImVec2(x + w, y),     ImVec2(x + w, y + cl));

		stroke(ImVec2(x, y + h),     ImVec2(x + cl, y + h));
		stroke(ImVec2(x, y + h),     ImVec2(x, y + h - cl));

		stroke(ImVec2(x + w, y + h), ImVec2(x + w - cl, y + h));
		stroke(ImVec2(x + w, y + h), ImVec2(x + w, y + h - cl));
	}

	inline void HealthBar(float boxX, float boxY, float boxH, int hp)
	{
		if (hp < 0) hp = 0;
		if (hp > 100) hp = 100;

		ImDrawList* dl = DL();

		const float barW = 3.5f;
		const float pad  = 5.0f;
		float x = boxX - pad - barW;
		float y = boxY;

		dl->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + barW + 1, y + boxH + 1),
		                  IM_COL32(0, 0, 0, 200), barW * 0.8f);

		float pct = hp / 100.0f;
		float fillTop = y + boxH * (1.0f - pct);
		ImU32 hpColor = IM_COL32(
			(int)(255.0f * (1.0f - pct)),
			(int)(255.0f * pct * 0.9f + 40.0f),
			32,
			255);
		if (fillTop < y + boxH) {
			dl->AddRectFilled(ImVec2(x, fillTop), ImVec2(x + barW, y + boxH),
			                  hpColor, barW * 0.5f);
		}

		if (hp < 100) {
			char buf[8];
			std::snprintf(buf, sizeof(buf), "%d", hp);
			ImVec2 ts = ImGui::CalcTextSize(buf);
			float tx = x + barW * 0.5f - ts.x * 0.5f;
			float ty = y + boxH + 2.0f;
			ShadowText(tx, ty, IM_COL32(240, 240, 245, 255), buf);
		}
	}

	inline void NamePill(float x, float y, ImU32 color, const char* text)
	{
		ImDrawList* dl = DL();
		ImVec2 ts = ImGui::CalcTextSize(text);
		float padX = 5.0f, padY = 2.0f;
		dl->AddRectFilled(ImVec2(x - padX, y - padY),
		                  ImVec2(x + ts.x + padX, y + ts.y + padY),
		                  IM_COL32(0, 0, 0, 170), 4.0f);
		dl->AddText(ImVec2(x, y), color, text);
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

	if (!(w > 0.01f))
		return false;

	float invW = 1.0f / w;
	float x = (matrix.m[0][0] * pos.x + matrix.m[0][1] * pos.y + matrix.m[0][2] * pos.z + matrix.m[0][3]) * invW;
	float y = (matrix.m[1][0] * pos.x + matrix.m[1][1] * pos.y + matrix.m[1][2] * pos.z + matrix.m[1][3]) * invW;

	screen.x = (width / 2.0f) * (1.0f + x);
	screen.y = (height / 2.0f) * (1.0f - y);

	if (!std::isfinite(screen.x) || !std::isfinite(screen.y)) return false;
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
	char pad[0x14];

};

struct BoneJointPos
{
	Vector3 Pos;
	Vector2 ScreenPos;
};

static uintptr_t ResolveHandle(uintptr_t entityList, uint32_t handle) {

	if (!entityList || !handle || handle == 0xFFFFFFFF) return 0;
	uint32_t idx = handle & 0x7FFF;
	uintptr_t listEntry = mem.Read<uintptr_t>(entityList + (8 * (idx >> 9)) + 16);
	if (!listEntry) return 0;
	return mem.Read<uintptr_t>(listEntry + 112 * (idx & 0x1FF));
}

	void HackGame() {

		ClearVisibleCache();

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

	float fovRadius = g_AimFovPx;
	float centerX = screenW / 2.0f;
	float centerY = screenH / 2.0f;

	uintptr_t bestTargetPawn = 0;
	float closestDistance = fovRadius;

	Vector2 bestTargetScreenPos = { 0.0f, 0.0f };

	int targetBoneId;
	switch (g_AimBone) {
		case 1: targetBoneId = NECK; break;
		case 2: targetBoneId = CHEST; break;
		case 3: targetBoneId = PELVIS; break;
		default: targetBoneId = HEAD; break;
	}

	if (g_VisualsEnabled && g_ESPFovCircle) {
		Overlay::ShadowCircle(centerX, centerY, fovRadius,
		                     ImGui::ColorConvertFloat4ToU32(g_ColorFovCircle), 64, 1.0f);
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

			if (g_ESPTeamCheck && localTeam != 0 && team == localTeam) {
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

						{
							const Vector3& rp = bonePosList[ROOT].Pos;
							if (rp.x == 0.0f && rp.y == 0.0f && rp.z == 0.0f) continue;
						}

							int pawnIndex = static_cast<int>(pawnHandle & 0x7FFF);
							bool entityVisible = IsPawnVisible(pawn, localPawn, entityList, pawnIndex);

							auto EspColor = [&](ImVec4 normal) -> ImU32 {
								return ImGui::ColorConvertFloat4ToU32(normal);
							};

					float targetBoxH = bonePosList[ROOT].ScreenPos.y - bonePosList[HEAD].ScreenPos.y;

						if (g_VisualsEnabled && g_ESPSkeleton) {
							ImU32 boneColor = EspColor(g_ColorSkeleton);

						float boneThickness = (targetBoxH > 220.0f) ? 1.3f : 1.0f;

						size_t linkCount = sizeof(local_skeleton_links) / sizeof(local_skeleton_links[0]);
						for (size_t i = 0; i < linkCount; i++)
						{
							unsigned int currentID = local_skeleton_links[i].a;
							unsigned int nextID = local_skeleton_links[i].b;

							Vector2 p1 = bonePosList[currentID].ScreenPos;
							Vector2 p2 = bonePosList[nextID].ScreenPos;

							if ((p1.x != 0 && p1.y != 0) && (p2.x != 0 && p2.y != 0))
							{
								Overlay::ShadowLine(p1.x, p1.y, p2.x, p2.y, boneColor, boneThickness);
							}
						}

							{
								static const unsigned int jointBones[] = {
									PELVIS, CHEST, NECK, HEAD,
									SHOULDER_L, ELBOW_L, HAND_L,
									SHOULDER_R, ELBOW_R, HAND_R,
									HIP_L, KNEE_L, FOOT_HEEL_L,
									HIP_R, KNEE_R, FOOT_HEEL_R
								};
								float dotR = (targetBoxH > 260.0f) ? 2.2f : (targetBoxH > 100.0f ? 1.6f : 1.0f);
								ImU32 dotColor = g_ESPSkeleton ? EspColor(g_ColorSkeleton) : IM_COL32(255, 255, 255, 220);
								for (unsigned int id : jointBones) {
									Vector2 jp = bonePosList[id].ScreenPos;
									if (jp.x != 0 && jp.y != 0) {
										Overlay::Dot(jp.x, jp.y, dotR, dotColor);
									}
								}
							}
					}

					Vector2 headPos = bonePosList[HEAD].ScreenPos;
					Vector2 neckPos = bonePosList[NECK].ScreenPos;

					Vector2 rootPose = bonePosList[ROOT].ScreenPos;

						if (g_VisualsEnabled && g_ESPHeadCircle && headPos.x != 0 && headPos.y != 0)
						{
							float headRadius = fabsf(headPos.y - neckPos.y);
							ImU32 headColor = EspColor(g_ColorHeadCircle);
						Overlay::ShadowCircle(headPos.x, headPos.y, headRadius, headColor, 28, 1.0f);

						ImU32 dotColor = (headColor & 0x00FFFFFF) | (60u << 24);
						ImGui::GetBackgroundDrawList()->AddCircleFilled(
							ImVec2(headPos.x, headPos.y), headRadius * 0.3f, dotColor, 16);
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

							bool skipDueToVis = (g_AimEnabled && g_AimVisibleCheck && !entityVisible);
							if (!skipDueToVis && distanceToCrosshair < closestDistance)
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

						if (g_VisualsEnabled && g_ESPBox) {
							Overlay::CornerBox(x, y, width, height,
							                  EspColor(g_ColorBox), 1.4f);
					}

					if (g_VisualsEnabled && g_ESPHealth) {
						Overlay::HealthBar(x, y, height, health);
					}

						if (g_VisualsEnabled && g_ESPWeaponName && boneArray) {
							uintptr_t ws = mem.Read<uintptr_t>(pawn + OffWeaponServices());
							if (ws) {
								uint32_t activeHandle = mem.Read<uint32_t>(ws + OffActiveWeapon());
								if (activeHandle) {
									uintptr_t wepEntity = ResolveHandle(entityList, activeHandle);
									if (wepEntity) {

										uintptr_t iv = wepEntity + OffAttributeManager() + OffItem();
										uint16_t defIdx = mem.Read<uint16_t>(iv + OffItemDefinitionIndex());

										const char* weaponName = nullptr;
										if (defIdx == 1) weaponName = "Desert Eagle";
										else if (defIdx == 2) weaponName = "Dual Berettas";
										else if (defIdx == 3) weaponName = "Five-SeveN";
										else if (defIdx == 4) weaponName = "Glock-18";
										else if (defIdx == 7) weaponName = "AK-47";
										else if (defIdx == 8) weaponName = "AUG";
										else if (defIdx == 9) weaponName = "AWP";
										else if (defIdx == 10) weaponName = "FAMAS";
										else if (defIdx == 11) weaponName = "G3SG1";
										else if (defIdx == 13) weaponName = "Galil AR";
										else if (defIdx == 14) weaponName = "M249";
										else if (defIdx == 16) weaponName = "M4A4";
										else if (defIdx == 17) weaponName = "MAC-10";
										else if (defIdx == 19) weaponName = "P90";
										else if (defIdx == 23) weaponName = "MP5-SD";
										else if (defIdx == 24) weaponName = "UMP-45";
										else if (defIdx == 25) weaponName = "XM1014";
										else if (defIdx == 26) weaponName = "PP-Bizon";
										else if (defIdx == 27) weaponName = "MAG-7";
										else if (defIdx == 28) weaponName = "Negev";
										else if (defIdx == 29) weaponName = "Sawed-Off";
										else if (defIdx == 30) weaponName = "Tec-9";
										else if (defIdx == 31) weaponName = "Zeus x27";
										else if (defIdx == 32) weaponName = "P2000";
										else if (defIdx == 33) weaponName = "MP7";
										else if (defIdx == 34) weaponName = "MP9";
										else if (defIdx == 35) weaponName = "Nova";
										else if (defIdx == 36) weaponName = "P250";
										else if (defIdx == 38) weaponName = "SCAR-20";
										else if (defIdx == 39) weaponName = "SG 553";
										else if (defIdx == 40) weaponName = "SSG 08";
										else if (defIdx == 60) weaponName = "M4A1-S";
										else if (defIdx == 61) weaponName = "USP-S";
										else if (defIdx == 63) weaponName = "CZ75-Auto";
										else if (defIdx == 64) weaponName = "R8 Revolver";
										else if (defIdx == 42 || defIdx == 59 || (defIdx >= 500 && defIdx <= 526)) weaponName = "Knife";

										if (weaponName) {
											ImVec2 ts = ImGui::CalcTextSize(weaponName);
											float tx = x + width * 0.5f - ts.x * 0.5f;
											float ty = y + height + 6.0f;
											Overlay::NamePill(tx, ty,
											                 ImGui::ColorConvertFloat4ToU32(g_ColorWeaponName),
											                 weaponName);
										}
									}
								}
							}
						}

							if (g_VisualsEnabled && g_ESPSnap) {
								Overlay::ShadowLine(screenW / 2.0f, screenH, rootPose.x, rootPose.y,
								                    entityVisible ? ImGui::ColorConvertFloat4ToU32(g_ColorSnapLine) : ImGui::ColorConvertFloat4ToU32(g_ColorHiddenTarget), 1.0f);
						}

				}
			}

		}

			if (bestTargetPawn != 0)
			{
				if (g_VisualsEnabled) {
					bool bestVisible = GetVisibleCache(bestTargetPawn);
					ImU32 tgtColor = bestVisible
						? ImGui::ColorConvertFloat4ToU32(g_ColorTargetLine)
						: ImGui::ColorConvertFloat4ToU32(g_ColorHiddenTarget);
					Overlay::ShadowLine(centerX, centerY, bestTargetScreenPos.x, bestTargetScreenPos.y,
					                    tgtColor, 1.2f);
					Overlay::Dot(bestTargetScreenPos.x, bestTargetScreenPos.y, 2.4f, tgtColor);
				}

			if (g_AimEnabled && localPawn) {
				Aimbot::Step(bestTargetScreenPos.x, bestTargetScreenPos.y, centerX, centerY);
			} else {
				Aimbot::NoTarget();
			}
		}
		else {

			Aimbot::NoTarget();
		}
	}
}