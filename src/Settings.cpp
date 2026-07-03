#include "Settings.hpp"

#include <fstream>
#include <filesystem>
#include <string>

bool g_AimEnabled = false;

bool g_ESPBox = true;
bool g_ESPSkeleton = true;
bool g_ESPHealth = true;
bool g_ESPHeadCircle = true;
bool g_ESPSnap = false;
bool g_ESPFovCircle = true;
bool g_ESPTeamCheck = true;

static const char* kSettingsFile = "revengecs.cfg";
static std::filesystem::file_time_type s_CfgLastWrite{};

void SaveSettings() {
	std::ofstream f(kSettingsFile);
	if (!f) return;
	f << "aim_enabled " << g_AimEnabled << "\n";
	f << "esp_box " << g_ESPBox << "\n";
	f << "esp_skel " << g_ESPSkeleton << "\n";
	f << "esp_hp " << g_ESPHealth << "\n";
	f << "esp_head " << g_ESPHeadCircle << "\n";
	f << "esp_snap " << g_ESPSnap << "\n";
	f << "esp_fov " << g_ESPFovCircle << "\n";
	f << "esp_team " << g_ESPTeamCheck << "\n";
}

void LoadSettings() {
	std::ifstream f(kSettingsFile);
	if (!f) return;
	std::string key;
	while (f >> key) {
		if      (key == "aim_enabled")  f >> g_AimEnabled;
		else if (key == "esp_box")      f >> g_ESPBox;
		else if (key == "esp_skel")     f >> g_ESPSkeleton;
		else if (key == "esp_hp")       f >> g_ESPHealth;
		else if (key == "esp_head")     f >> g_ESPHeadCircle;
		else if (key == "esp_snap")     f >> g_ESPSnap;
		else if (key == "esp_fov")      f >> g_ESPFovCircle;
		else if (key == "esp_team")     f >> g_ESPTeamCheck;
		else { std::string skip; std::getline(f, skip); }
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
