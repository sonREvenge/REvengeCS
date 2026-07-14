#include "SettingsUI.hpp"

#include "imgui.h"

#include "OffsetScanner.hpp"
#include "Settings.hpp"

void ApplyCustomDarkTheme() {
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	style.WindowPadding = ImVec2(16, 14);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 0.0f;
	style.FramePadding = ImVec2(8, 4);
	style.FrameRounding = 3.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8, 6);
	style.ItemInnerSpacing = ImVec2(6, 4);
	style.IndentSpacing = 18.0f;
	style.ScrollbarSize = 10.0f;
	style.ScrollbarRounding = 4.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 3.0f;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 0.0f;
	style.PopupRounding = 4.0f;
	style.SeparatorTextBorderSize = 1.0f;

		colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.42f, 0.43f, 0.46f, 1.00f);

		colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

		colors[ImGuiCol_Border] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

		colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);

		colors[ImGuiCol_Header] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);

		colors[ImGuiCol_Separator] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
}

static const ImVec4 kAccent(0.55f, 0.50f, 0.95f, 1.0f);
static const ImVec4 kGood(0.40f, 0.85f, 0.55f, 1.0f);
static const ImVec4 kMuted(0.42f, 0.43f, 0.46f, 1.0f);

static bool ToggleSwitch(const char* id, bool* v) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 p = ImGui::GetCursorScreenPos();

	float height = ImGui::GetFontSize() * 1.3f;
	float width = height * 1.8f;
	float radius = height * 0.5f;

	ImGui::InvisibleButton(id, ImVec2(width, height));
	bool clicked = ImGui::IsItemClicked();
	if (clicked) *v = !*v;

	bool hovered = ImGui::IsItemHovered();
	ImU32 bgOn = ImGui::ColorConvertFloat4ToU32(hovered
		? ImVec4(kAccent.x + 0.08f, kAccent.y + 0.08f, kAccent.z + 0.05f, 1.0f)
		: kAccent);
	ImU32 bgOff = hovered ? IM_COL32(58, 58, 64, 255) : IM_COL32(45, 45, 50, 255);

	drawList->AddRectFilled(p, ImVec2(p.x + width, p.y + height), *v ? bgOn : bgOff, radius);

	float knobX = *v ? (p.x + width - radius) : (p.x + radius);
	drawList->AddCircleFilled(ImVec2(knobX, p.y + radius), radius - 3.0f, IM_COL32(240, 240, 245, 255));

	return clicked;
}

static void BuildAimbotPage() {
	ImGui::Text("Enabled");
	ImGui::SameLine(ImGui::GetWindowWidth() - 60);
	ToggleSwitch("##aim_on", &g_AimEnabled);

	ImGui::Dummy(ImVec2(0, 6));

	ImGui::TextColored(kAccent, "Targeting");
	ImGui::Separator();
	static const char* kBones[] = { "Head", "Neck", "Chest", "Pelvis" };
	ImGui::SetNextItemWidth(160);
	ImGui::Combo("Aim bone", &g_AimBone, kBones, IM_ARRAYSIZE(kBones));
	ImGui::SetNextItemWidth(160);
	ImGui::SliderFloat("FOV radius", &g_AimFovPx, 20.0f, 600.0f, "%.0f px");

	ImGui::Dummy(ImVec2(0, 8));
	ImGui::TextColored(kAccent, "Speed");
	ImGui::Separator();
	ImGui::SetNextItemWidth(160);
	ImGui::SliderFloat("Smoothness", &g_AimSmoothness, 1.0f, 30.0f, "%.1f");
	ImGui::SameLine();
	ImGui::TextColored(kMuted, "1=snap  |  30=slow");

	ImGui::Dummy(ImVec2(0, 8));
	ImGui::TextColored(kAccent, "Behavior");
	ImGui::Separator();
	{
		const char* modes[] = { "Rage", "Legit Light", "Legit Full" };
		ImGui::SetNextItemWidth(160);
		ImGui::Combo("Aim mode", &g_AimLegitMode, modes, IM_ARRAYSIZE(modes));
		ImGui::SameLine();
		if (g_AimLegitMode == 0)
			ImGui::TextColored(kMuted, "raw linear snap");
		else if (g_AimLegitMode == 1)
			ImGui::TextColored(kMuted, "eased curve + reaction");
		else
			ImGui::TextColored(kMuted, "full humanization");
	}

	ImGui::SetNextItemWidth(160);
	ImGui::SliderFloat("Humanize", &g_AimHumanize, 0.0f, 1.0f, "%.2f");
	ImGui::SameLine();
	ImGui::TextColored(kMuted, g_AimLegitMode < 2 ? "requires Legit Full" : "jitter + bias + slowdown");

	ImGui::Dummy(ImVec2(0, 4));
	ImGui::Text("Visible check");
	ImGui::SameLine(ImGui::GetWindowWidth() - 60);
	ToggleSwitch("##vis_check", &g_AimVisibleCheck);
	ImGui::TextColored(kMuted, "ignore targets behind walls");
}

static void BuildEspPage() {
	ImGui::Text("Enabled");
	ImGui::SameLine(ImGui::GetWindowWidth() - 60);
	ToggleSwitch("##esp_on", &g_VisualsEnabled);

	ImGui::Dummy(ImVec2(0, 6));

	ImGui::BeginDisabled(!g_VisualsEnabled);

	ImGui::TextColored(kAccent, "Display");
	ImGui::Separator();

	float colW = ImGui::GetContentRegionAvail().x * 0.5f;
	ImGui::Columns(2, "##esp_cols", false);
	ImGui::SetColumnWidth(0, colW);
	ImGui::Checkbox("Bounding box", &g_ESPBox);
	ImGui::Checkbox("Skeleton", &g_ESPSkeleton);
	ImGui::Checkbox("Health bar", &g_ESPHealth);
	ImGui::Checkbox("Head circle", &g_ESPHeadCircle);
	ImGui::NextColumn();
	ImGui::Checkbox("Snap lines", &g_ESPSnap);
	ImGui::Checkbox("FOV circle", &g_ESPFovCircle);
	ImGui::Checkbox("Weapon names", &g_ESPWeaponName);
	ImGui::Columns(1);

	ImGui::Dummy(ImVec2(0, 4));
	ImGui::TextColored(kAccent, "Filtering");
	ImGui::Separator();
	ImGui::Checkbox("Skip teammates", &g_ESPTeamCheck);

	ImGui::EndDisabled();

	ImGui::Dummy(ImVec2(0, 6));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0, 6));

	if (ImGui::CollapsingHeader("Colors")) {
		ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar;
		ImGui::PushItemWidth(180);
		ImGui::ColorEdit4("Box##c",         (float*)&g_ColorBox,         flags);
		ImGui::ColorEdit4("Skeleton##c",    (float*)&g_ColorSkeleton,    flags);
		ImGui::ColorEdit4("Head circle##c", (float*)&g_ColorHeadCircle,  flags);
		ImGui::ColorEdit4("FOV circle##c",  (float*)&g_ColorFovCircle,   flags);
		ImGui::ColorEdit4("Snap line##c",   (float*)&g_ColorSnapLine,    flags);
		ImGui::ColorEdit4("Weapon name##c", (float*)&g_ColorWeaponName,  flags);
		ImGui::ColorEdit4("Target line##c", (float*)&g_ColorTargetLine,  flags);
		ImGui::ColorEdit4("Hidden target##c", (float*)&g_ColorHiddenTarget, flags);
		ImGui::PopItemWidth();
		ImGui::Dummy(ImVec2(0, 4));
		if (ImGui::Button("Reset to defaults", ImVec2(160, 22))) {
			g_ColorBox         = ImVec4(0.94f, 0.94f, 0.96f, 1.00f);
			g_ColorSkeleton    = ImVec4(0.94f, 0.94f, 0.96f, 0.92f);
			g_ColorHeadCircle  = ImVec4(1.00f, 0.35f, 0.35f, 0.90f);
			g_ColorFovCircle   = ImVec4(0.49f, 0.23f, 0.93f, 0.63f);
			g_ColorSnapLine    = ImVec4(1.00f, 1.00f, 1.00f, 0.43f);
			g_ColorWeaponName  = ImVec4(1.00f, 0.86f, 0.51f, 0.94f);
			g_ColorTargetLine   = ImVec4(1.00f, 0.24f, 0.24f, 0.90f);
			g_ColorHiddenTarget = ImVec4(0.55f, 0.55f, 0.60f, 0.85f);
		}
	}
}

static void KeyBindButton(const char* label, int& keyVar) {
	static int activeId = 0;
	int id = ImGui::GetID(label);

	char buf[64];
	if (activeId == id) {
		strcpy_s(buf, "Press key...");

		for (int i = 8; i < 256; i++) {
			if (i == VK_LBUTTON || i == VK_RBUTTON || i == VK_MBUTTON) continue;
			if (GetAsyncKeyState(i) & 0x8000) {
				if (i == VK_ESCAPE) {
					keyVar = 0;
				} else {
					keyVar = i;
				}
				activeId = 0;
				SaveSettings();
				break;
			}
		}
	} else {
		if (keyVar == 0) {
			strcpy_s(buf, "NONE");
		} else {
			unsigned int scanCode = MapVirtualKeyA(keyVar, MAPVK_VK_TO_VSC);
			char name[64] = { 0 };
			if (GetKeyNameTextA(scanCode << 16, name, sizeof(name))) {
				strcpy_s(buf, name);
			} else {
				sprintf_s(buf, "Key %d", keyVar);
			}
		}
	}

	ImGui::Text(label);
	ImGui::SameLine(120);
	char btnId[128];
	sprintf_s(btnId, "%s##%s", buf, label);
	if (ImGui::Button(btnId, ImVec2(140, 22))) {
		activeId = id;
	}
}

static void BuildKeybindsPage() {
	KeyBindButton("Aimbot Toggle", g_AimbotHotkey);
	ImGui::Dummy(ImVec2(0, 4));
	KeyBindButton("Visuals Toggle", g_VisualsHotkey);
}

static void BuildDumperPage() {
	ImGui::TextColored(mem.IsAttached() ? kGood : kMuted, mem.IsAttached() ? "ATTACHED" : "NOT ATTACHED");
	ImGui::Dummy(ImVec2(0, 4));

	ImGui::Text("client.dll base");
	ImGui::SameLine(160); ImGui::TextColored(kMuted, "0x%llX", (unsigned long long)module_base);

	ImGui::Text("Entity list");
	ImGui::SameLine(160);
	ImGui::TextColored(kMuted, "0x%llX%s", (unsigned long long)(g_ovEntityList ? g_ovEntityList : offsets::dwEntityList), g_ovEntityList ? "  (scanned)" : "  (built-in)");

	ImGui::Text("Local controller");
	ImGui::SameLine(160);
	ImGui::TextColored(kMuted, "0x%llX%s", (unsigned long long)(g_ovLocalController ? g_ovLocalController : offsets::dwLocalPlayerController), g_ovLocalController ? "  (scanned)" : "  (built-in)");

	ImGui::Text("View matrix");
	ImGui::SameLine(160);
	ImGui::TextColored(kMuted, "0x%llX%s", (unsigned long long)(g_ovViewMatrix ? g_ovViewMatrix : offsets::dwViewMatrix), g_ovViewMatrix ? "  (scanned)" : "  (built-in)");

	int scannedFields = (int)g_fldTeamNum.valid + (int)g_fldHealth.valid
		+ (int)g_fldGameSceneNode.valid + (int)g_fldPlayerPawnHandle.valid
		+ (int)g_fldModelState.valid + (int)g_fldEntitySpottedState.valid;
		constexpr int kExpected = 6;
		ImGui::Text("Class fields");
		ImGui::SameLine(160);
		ImGui::TextColored(scannedFields >= kExpected ? kGood : kMuted,
		                   "%d/%d scanned", scannedFields, kExpected);

	ImGui::Dummy(ImVec2(0, 6));
	if (ImGui::Button("Rescan", ImVec2(110, 24)) && module_base != 0) {
		RefreshOffsets(module_base, module_size);
	}
}

void BuildSettingsUI(int& activeTab) {
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_CheckMark]        = kAccent;
	style.Colors[ImGuiCol_Button]           = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.5f);
	style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.75f);
	style.Colors[ImGuiCol_ButtonActive]     = kAccent;

	const ImGuiViewport* vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);
	ImGui::SetNextWindowSize(vp->WorkSize);
	ImGui::Begin("##REVENGE_CS", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

		static const char* kTabLabels[] = { "Aimbot", "Visuals", "Keybinds", "Dumper" };
		for (int i = 0; i < IM_ARRAYSIZE(kTabLabels); i++) {
			bool sel = (activeTab == i);
			ImGui::PushStyleColor(ImGuiCol_Text, sel ? style.Colors[ImGuiCol_Text] : kMuted);
			if (ImGui::Selectable(kTabLabels[i], false, 0, ImVec2(ImGui::CalcTextSize(kTabLabels[i]).x, 0))) {
				activeTab = i;
			}
			ImGui::PopStyleColor();
			if (sel) {
				ImVec2 minPos = ImGui::GetItemRectMin();
				ImVec2 maxPos = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddLine(
					ImVec2(minPos.x, maxPos.y + 3), ImVec2(maxPos.x, maxPos.y + 3),
					ImGui::ColorConvertFloat4ToU32(kAccent), 2.0f);
			}
			if (i + 1 < IM_ARRAYSIZE(kTabLabels)) ImGui::SameLine(0, 20);
		}

		ImGui::Dummy(ImVec2(0, 6));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0, 8));

		switch (activeTab) {
			case 0: BuildAimbotPage(); break;
			case 1: BuildEspPage(); break;
			case 2: BuildKeybindsPage(); break;
			case 3: BuildDumperPage(); break;
		}

	ImGui::End();
}
