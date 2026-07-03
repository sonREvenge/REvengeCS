#pragma once

// Reads the game world, resolves the closest visible enemy in the aimbot's
// FOV, and draws the ESP overlay for the current frame. Safe to call every
// frame; no-ops internally if required pointers can't be read.
void HackGame();
