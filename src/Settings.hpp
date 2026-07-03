#pragma once

// Aimbot
extern bool g_AimEnabled;
inline constexpr float kAimFovPx = 200.0f;
inline constexpr float kAimSmoothness = 5.0f;

// Visuals / ESP
extern bool g_ESPBox;
extern bool g_ESPSkeleton;
extern bool g_ESPHealth;
extern bool g_ESPHeadCircle;
extern bool g_ESPSnap;
extern bool g_ESPFovCircle;
extern bool g_ESPTeamCheck;

// Persists the toggles above to revengecs.cfg next to the executable.
void SaveSettings();
void LoadSettings();

// Reloads revengecs.cfg if it changed on disk since the last check, so
// external edits to the file take effect without restarting the app.
void CheckReloadSettings();
