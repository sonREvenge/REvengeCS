#include "Settings.hpp"

#include <fstream>
#include <filesystem>
#include <string>

bool g_AimEnabled = false;
int g_AimBone = 0;
float g_AimFovPx = 200.0f;

bool g_VisualsEnabled = true;
bool g_ESPBox = true;
bool g_ESPSkeleton = true;
bool g_ESPHealth = true;
bool g_ESPHeadCircle = true;
bool g_ESPSnap = false;
bool g_ESPFovCircle = true;
bool g_ESPTeamCheck = true;
bool g_ESPWeaponName = false;

float g_AimSmoothness = 5.0f;
float g_AimHumanize = 0.5f;
int   g_AimLegitMode = 0;
bool  g_AimVisibleCheck = false;

int g_AimbotHotkey = 0;
int g_VisualsHotkey = 0;

	ImVec4 g_ColorBox         = ImVec4(0.94f, 0.94f, 0.96f, 1.00f);
	ImVec4 g_ColorSkeleton    = ImVec4(0.94f, 0.94f, 0.96f, 0.92f);
	ImVec4 g_ColorHeadCircle  = ImVec4(1.00f, 0.35f, 0.35f, 0.90f);
	ImVec4 g_ColorFovCircle   = ImVec4(0.49f, 0.23f, 0.93f, 0.63f);
	ImVec4 g_ColorSnapLine    = ImVec4(1.00f, 1.00f, 1.00f, 0.43f);
	ImVec4 g_ColorWeaponName  = ImVec4(1.00f, 0.86f, 0.51f, 0.94f);
	ImVec4 g_ColorTargetLine  = ImVec4(1.00f, 0.24f, 0.24f, 0.90f);
		ImVec4 g_ColorHiddenTarget = ImVec4(0.55f, 0.55f, 0.60f, 0.85f);

	static const char* kSettingsFile = "revengecs.cfg";
static const char* kSettingsTmp  = "revengecs.cfg.tmp";
static std::filesystem::file_time_type s_CfgLastWrite{};

void SaveSettings() {

	std::ofstream f(kSettingsTmp, std::ios::trunc);
	if (!f) return;
	f << "aim_enabled " << g_AimEnabled << "\n";
	f << "aim_bone " << g_AimBone << "\n";
	f << "aim_fov_px " << g_AimFovPx << "\n";
	f << "visuals_enabled " << g_VisualsEnabled << "\n";
	f << "esp_box " << g_ESPBox << "\n";
	f << "esp_skel " << g_ESPSkeleton << "\n";
	f << "esp_hp " << g_ESPHealth << "\n";
	f << "esp_head " << g_ESPHeadCircle << "\n";
	f << "esp_snap " << g_ESPSnap << "\n";
	f << "esp_fov " << g_ESPFovCircle << "\n";
	f << "esp_team " << g_ESPTeamCheck << "\n";
	f << "esp_weapon_name " << g_ESPWeaponName << "\n";
	f << "aim_smooth " << g_AimSmoothness << "\n";
	f << "aim_humanize " << g_AimHumanize << "\n";
	f << "aim_legit " << g_AimLegitMode << "\n";
	f << "aim_visible_check " << g_AimVisibleCheck << "\n";
	f << "aimbot_hotkey " << g_AimbotHotkey << "\n";
	f << "visuals_hotkey " << g_VisualsHotkey << "\n";

	auto saveColor = [&](const char* key, const ImVec4& c) {
		f << key << " " << c.x << " " << c.y << " " << c.z << " " << c.w << "\n";
	};
	saveColor("color_box",         g_ColorBox);
	saveColor("color_skeleton",    g_ColorSkeleton);
	saveColor("color_head_circle", g_ColorHeadCircle);
	saveColor("color_fov_circle",  g_ColorFovCircle);
	saveColor("color_snap_line",   g_ColorSnapLine);
	saveColor("color_weapon_name", g_ColorWeaponName);
		saveColor("color_target_line", g_ColorTargetLine);
		saveColor("color_hidden_target", g_ColorHiddenTarget);

		f.close();

	std::error_code ec;
	std::filesystem::rename(kSettingsTmp, kSettingsFile, ec);
	if (ec) {

		std::filesystem::copy_file(kSettingsTmp, kSettingsFile,
			std::filesystem::copy_options::overwrite_existing, ec);
		std::filesystem::remove(kSettingsTmp, ec);
	}

	std::error_code stampErr;
	auto now = std::filesystem::last_write_time(kSettingsFile, stampErr);
	if (!stampErr) s_CfgLastWrite = now;
}

void LoadSettings() {
	std::ifstream f(kSettingsFile);
	if (!f) return;
	std::string key;
	while (f >> key) {
		if      (key == "aim_enabled")     f >> g_AimEnabled;
		else if (key == "aim_bone")        f >> g_AimBone;
		else if (key == "aim_fov_px")      f >> g_AimFovPx;
		else if (key == "visuals_enabled") f >> g_VisualsEnabled;
		else if (key == "esp_box")         f >> g_ESPBox;
		else if (key == "esp_skel")        f >> g_ESPSkeleton;
		else if (key == "esp_hp")          f >> g_ESPHealth;
		else if (key == "esp_head")        f >> g_ESPHeadCircle;
		else if (key == "esp_snap")        f >> g_ESPSnap;
		else if (key == "esp_fov")         f >> g_ESPFovCircle;
		else if (key == "esp_team")        f >> g_ESPTeamCheck;
		else if (key == "esp_weapon_name") f >> g_ESPWeaponName;
		else if (key == "esp_skin_name")   f >> g_ESPWeaponName;

		else if (key == "aim_smooth")       f >> g_AimSmoothness;
		else if (key == "aim_humanize")     f >> g_AimHumanize;
			else if (key == "aim_legit")        f >> g_AimLegitMode;
			else if (key == "aim_visible_check") f >> g_AimVisibleCheck;
			else if (key == "aimbot_hotkey")   f >> g_AimbotHotkey;
			else if (key == "visuals_hotkey")  f >> g_VisualsHotkey;
			else if (key == "color_box")         f >> g_ColorBox.x >> g_ColorBox.y >> g_ColorBox.z >> g_ColorBox.w;
		else if (key == "color_skeleton")    f >> g_ColorSkeleton.x >> g_ColorSkeleton.y >> g_ColorSkeleton.z >> g_ColorSkeleton.w;
		else if (key == "color_head_circle") f >> g_ColorHeadCircle.x >> g_ColorHeadCircle.y >> g_ColorHeadCircle.z >> g_ColorHeadCircle.w;
		else if (key == "color_fov_circle")  f >> g_ColorFovCircle.x >> g_ColorFovCircle.y >> g_ColorFovCircle.z >> g_ColorFovCircle.w;
		else if (key == "color_snap_line")   f >> g_ColorSnapLine.x >> g_ColorSnapLine.y >> g_ColorSnapLine.z >> g_ColorSnapLine.w;
		else if (key == "color_weapon_name") f >> g_ColorWeaponName.x >> g_ColorWeaponName.y >> g_ColorWeaponName.z >> g_ColorWeaponName.w;
				else if (key == "color_target_line")  f >> g_ColorTargetLine.x >> g_ColorTargetLine.y >> g_ColorTargetLine.z >> g_ColorTargetLine.w;
				else if (key == "color_hidden_target") f >> g_ColorHiddenTarget.x >> g_ColorHiddenTarget.y >> g_ColorHiddenTarget.z >> g_ColorHiddenTarget.w;
			else { std::string skip; std::getline(f, skip); }

		if (f.fail() && !f.eof()) {
			f.clear();
			std::string skip; std::getline(f, skip);
		}
	}
}

void CheckReloadSettings() {
	try {
		auto current = std::filesystem::last_write_time(kSettingsFile);
		if (current != s_CfgLastWrite) {
			s_CfgLastWrite = current;
			LoadSettings();
		}
	} catch (...) {}
}