#pragma once

#include <cstdint>

#include "imgui.h"

extern bool g_AimEnabled;
extern int g_AimBone;

extern float g_AimFovPx;

extern float g_AimSmoothness;

extern float g_AimHumanize;

extern int   g_AimLegitMode; // 0=Rage, 1=Legit Light, 2=Legit Full

extern bool  g_AimVisibleCheck;

extern bool g_VisualsEnabled;
extern bool g_ESPBox;
extern bool g_ESPSkeleton;
extern bool g_ESPHealth;
extern bool g_ESPHeadCircle;
extern bool g_ESPSnap;
extern bool g_ESPFovCircle;
extern bool g_ESPTeamCheck;
extern bool g_ESPWeaponName;

extern int g_AimbotHotkey;
extern int g_VisualsHotkey;

extern ImVec4 g_ColorBox;
extern ImVec4 g_ColorSkeleton;
extern ImVec4 g_ColorHeadCircle;
extern ImVec4 g_ColorFovCircle;
extern ImVec4 g_ColorSnapLine;
extern ImVec4 g_ColorWeaponName;
extern ImVec4 g_ColorTargetLine;
extern ImVec4 g_ColorHiddenTarget;

void SaveSettings();
void LoadSettings();

void CheckReloadSettings();