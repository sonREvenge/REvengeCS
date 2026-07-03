#pragma once

// Applies the app's dark theme to the current ImGui context. Call once per
// context right after creating it.
void ApplyCustomDarkTheme();

// Renders the borderless settings window (tab nav + Aimbot/Visuals/Dumper
// pages) into the current ImGui frame. `activeTab` persists the selected tab
// across frames.
void BuildSettingsUI(int& activeTab);
