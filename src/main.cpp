#include <windows.h>
#include <shellscalingapi.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "Shcore.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "D3DBackend.hpp"
#include "Settings.hpp"
#include "OffsetScanner.hpp"
#include "GameHack.hpp"
#include "SettingsUI.hpp"

static void KeyHandler() {
	while (g_Running) {
		if (GetAsyncKeyState(VK_END) & 1) {
			g_Running = false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

static void AttachAndScan() {
	if (mem.Attach(L"cs2.exe")) {
		ModuleInfo mi = mem.GetModuleInfo(L"client.dll");
		module_base = mi.base;
		module_size = mi.size;
		RefreshOffsets(module_base, module_size);
	}
}

int main() {
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	LoadSettings();
	AttachAndScan();

	HINSTANCE hInst = GetModuleHandle(NULL);

	WNDCLASSEXW wcSettings = { sizeof(wcSettings), CS_CLASSDC, WndProc, 0L, 0L, hInst,
		LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1), nullptr, L"REVENGE_CS_Settings", nullptr };
	RegisterClassExW(&wcSettings);

	WNDCLASSEXW wcOverlay = { sizeof(wcOverlay), CS_CLASSDC, WndProc, 0L, 0L, hInst,
		nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, L"REVENGE_CS_Overlay", nullptr };
	RegisterClassExW(&wcOverlay);

	const int sw = 420, sh = 320;
	int sx = (GetSystemMetrics(SM_CXSCREEN) - sw) / 2;
	int sy = (GetSystemMetrics(SM_CYSCREEN) - sh) / 2;
	g_Settings = CreateWindowExW(
		0,
		wcSettings.lpszClassName, L"REVENGE CS",
		WS_OVERLAPPEDWINDOW,
		sx, sy, sw, sh,
		nullptr, nullptr, hInst, nullptr
	);

	g_Overlay = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		wcOverlay.lpszClassName, L"REVENGE CS Overlay",
		WS_POPUP,
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		nullptr, nullptr, hInst, nullptr
	);
	SetLayeredWindowAttributes(g_Overlay, RGB(0, 0, 0), 255, LWA_ALPHA);
	MARGINS margins = { -1 };
	DwmExtendFrameIntoClientArea(g_Overlay, &margins);

	g_GameWindow = FindWindowW(nullptr, L"Counter-Strike 2");

	if (!CreateD3DDevice()) {
		MessageBoxA(nullptr, "Failed to create the Direct3D 11 device.", "REVENGE CS", MB_OK | MB_ICONERROR);
		return 1;
	}
	if (!CreateSwapChainForHwnd(g_Settings, &g_SettingsSwap, &g_SettingsRTV)) {
		MessageBoxA(nullptr, "Failed to create the settings window swap chain.", "REVENGE CS", MB_OK | MB_ICONERROR);
		return 1;
	}
	if (!CreateSwapChainForHwnd(g_Overlay, &g_OverlaySwap, &g_OverlayRTV)) {
		MessageBoxA(nullptr, "Failed to create the overlay swap chain.", "REVENGE CS", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(g_Settings, SW_SHOWDEFAULT); UpdateWindow(g_Settings);
	ShowWindow(g_Overlay,  SW_SHOWDEFAULT); UpdateWindow(g_Overlay);

	IMGUI_CHECKVERSION();
	g_SettingsCtx = ImGui::CreateContext();
	ImGui::SetCurrentContext(g_SettingsCtx);
	ImGui::StyleColorsDark(); ApplyCustomDarkTheme();
	ImGui_ImplWin32_Init(g_Settings);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	g_OverlayCtx = ImGui::CreateContext();
	ImGui::SetCurrentContext(g_OverlayCtx);
	ImGui::StyleColorsDark(); // overlay has no widgets but ImGui needs a style
	ImGui_ImplWin32_Init(g_Overlay);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	std::thread hotkeyThread(KeyHandler);

	int activeTab = 0;

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (g_Running) {
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) g_Running = false;
		}
		if (!g_Running) break;

		if (!g_GameWindow) g_GameWindow = FindWindowW(nullptr, L"Counter-Strike 2");

		SetWindowPos(g_Overlay, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		if (!mem.IsAttached() || module_base == 0) {
			static int s_RetryFrame = 0;
			if ((++s_RetryFrame % 120) == 0) {
				AttachAndScan();
			}
		}

		{
			static int s_CfgPollFrame = 0;
			if ((++s_CfgPollFrame % 60) == 0) CheckReloadSettings();
		}

		const float clearTransparent[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		const float clearOpaque[4] = { 0.10f, 0.10f, 0.14f, 1.0f };

		ImGui::SetCurrentContext(g_SettingsCtx);
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		BuildSettingsUI(activeTab);
		ImGui::Render();
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_SettingsRTV, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_SettingsRTV, clearOpaque);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		g_SettingsSwap->Present(1, 0);

		ImGui::SetCurrentContext(g_OverlayCtx);
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		HWND foregroundNow = GetForegroundWindow();
		bool cs2Active = (g_GameWindow && foregroundNow == g_GameWindow);
		if (mem.IsAttached() && module_base != 0 && cs2Active) {
			HackGame();
		}
		ImGui::Render();
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_OverlayRTV, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_OverlayRTV, clearTransparent);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		g_OverlaySwap->Present(1, 0);
	}

	SaveSettings();
	if (hotkeyThread.joinable()) hotkeyThread.join();

	if (g_Overlay)  { DestroyWindow(g_Overlay);  g_Overlay  = nullptr; }
	if (g_Settings) { DestroyWindow(g_Settings); g_Settings = nullptr; }

	if (g_OverlayCtx) {
		ImGui::SetCurrentContext(g_OverlayCtx);
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(g_OverlayCtx);
		g_OverlayCtx = nullptr;
	}
	if (g_SettingsCtx) {
		ImGui::SetCurrentContext(g_SettingsCtx);
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(g_SettingsCtx);
		g_SettingsCtx = nullptr;
	}

	CleanupSwapChain(&g_OverlaySwap,  &g_OverlayRTV);
	CleanupSwapChain(&g_SettingsSwap, &g_SettingsRTV);
	CleanupD3DDevice();

	UnregisterClassW(wcOverlay.lpszClassName,  hInst);
	UnregisterClassW(wcSettings.lpszClassName, hInst);

	return 0;
}
