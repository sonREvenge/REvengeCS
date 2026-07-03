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

	colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.035f, 0.04f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.035f, 0.035f, 0.04f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.08f, 0.98f);

	colors[ImGuiCol_Border] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.17f, 0.17f, 0.21f, 1.00f);

	colors[ImGuiCol_Header] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.17f, 0.21f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

	colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.035f, 0.035f, 0.04f, 0.0f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
}

static const ImVec4 kAccent(0.55f, 0.50f, 0.95f, 1.0f);
static const ImVec4 kGood(0.40f, 0.85f, 0.55f, 1.0f);
static const ImVec4 kMuted(0.42f, 0.43f, 0.46f, 1.0f);

// Minimal pill toggle switch, drawn on the current window's draw list.
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
}

static void BuildEspPage() {
	ImGui::Checkbox("Bounding box", &g_ESPBox);
	ImGui::Checkbox("Skeleton", &g_ESPSkeleton);
	ImGui::Checkbox("Health bar", &g_ESPHealth);
	ImGui::Checkbox("Head circle", &g_ESPHeadCircle);
	ImGui::Checkbox("Snap lines", &g_ESPSnap);
	ImGui::Checkbox("FOV circle", &g_ESPFovCircle);
	ImGui::Checkbox("Skip teammates", &g_ESPTeamCheck);
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

	int scannedFields = (int)g_fldTeamNum.valid + (int)g_fldHealth.valid + (int)g_fldGameSceneNode.valid
		+ (int)g_fldPlayerPawnHandle.valid + (int)g_fldModelState.valid;
	ImGui::Text("Class fields");
	ImGui::SameLine(160);
	ImGui::TextColored(scannedFields == 5 ? kGood : kMuted, "%d/5 scanned", scannedFields);

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

	static const char* kTabLabels[] = { "Aimbot", "Visuals", "Dumper" };
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
		case 2: BuildDumperPage(); break;
	}

	ImGui::End();
}
