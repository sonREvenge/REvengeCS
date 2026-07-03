#pragma once

// Aimbot
extern bool g_AimEnabled;
extern int g_AimBone;       // 0 Head, 1 Neck, 2 Chest, 3 Pelvis
extern float g_AimFovPx;    // radius (in pixels) of the FOV cone the aimbot tracks inside
extern float g_AimSmoothness; // 1.0 = rage (snap), higher = legit/smooth

// Visuals / ESP
extern bool g_VisualsEnabled;
extern bool g_ESPBox;
extern bool g_ESPSkeleton;
extern bool g_ESPHealth;
extern bool g_ESPHeadCircle;
extern bool g_ESPSnap;
extern bool g_ESPFovCircle;
extern bool g_ESPTeamCheck;

// Hotkeys (VK codes)
extern int g_AimbotHotkey;
extern int g_VisualsHotkey;

// Persists the toggles above to revengecs.cfg next to the executable.
void SaveSettings();
void LoadSettings();

// Reloads revengecs.cfg if it changed on disk since the last check, so
// external edits to the file take effect without restarting the app.
void CheckReloadSettings();
