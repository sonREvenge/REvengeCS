#include "Hotkeys.hpp"

#include <windows.h>
#include <thread>
#include <chrono>

#include "D3DBackend.hpp"

#include "Settings.hpp"

namespace {
	std::thread s_thread;

	void PollLoop() {
		while (g_Running) {
			if (GetAsyncKeyState(VK_END) & 1) {
				g_Running = false;
			}
			if (g_AimbotHotkey != 0 && (GetAsyncKeyState(g_AimbotHotkey) & 1)) {
				g_AimEnabled = !g_AimEnabled;
			}
			if (g_VisualsHotkey != 0 && (GetAsyncKeyState(g_VisualsHotkey) & 1)) {
				g_VisualsEnabled = !g_VisualsEnabled;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

namespace Hotkeys
{
	void Start() {
		s_thread = std::thread(PollLoop);
	}

	void Stop() {
		if (s_thread.joinable()) s_thread.join();
	}
}