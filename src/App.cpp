#include "App.hpp"

#include <windows.h>
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "D3DBackend.hpp"
#include "GameHack.hpp"
#include "Hotkeys.hpp"
#include "OffsetScanner.hpp"
#include "Settings.hpp"
#include "SettingsUI.hpp"
#include "VisibleCheck.hpp"

namespace {

	HINSTANCE s_hInstance = nullptr;
	int       s_activeTab = 0;

	constexpr wchar_t kSettingsClass[] = L"REVENGE_CS_Settings";
	constexpr wchar_t kOverlayClass[]  = L"REVENGE_CS_Overlay";
	constexpr wchar_t kGameTitle[]     = L"Counter-Strike 2";

	void AttachAndScan() {
		if (mem.Attach(L"cs2.exe")) {
			ModuleInfo mi = mem.GetModuleInfo(L"client.dll");
			module_base = mi.base;
			module_size = mi.size;
			RefreshOffsets(module_base, module_size);
			InitVisibleCheck(module_base, module_size);
		}
	}

	void RegisterWindowClasses(HINSTANCE hInst) {
		WNDCLASSEXW wcSettings = { sizeof(wcSettings), CS_CLASSDC, WndProc, 0L, 0L, hInst,
			LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW),
			(HBRUSH)(COLOR_WINDOW + 1), nullptr, kSettingsClass, nullptr };
		RegisterClassExW(&wcSettings);

		WNDCLASSEXW wcOverlay = { sizeof(wcOverlay), CS_CLASSDC, WndProc, 0L, 0L, hInst,
			nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, kOverlayClass, nullptr };
		RegisterClassExW(&wcOverlay);
	}

	void CreateSettingsWindow(HINSTANCE hInst) {
		const int w = 420, h = 320;
		int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
		int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
		g_Settings = CreateWindowExW(0, kSettingsClass, L"REVENGE CS",
			WS_OVERLAPPEDWINDOW, x, y, w, h,
			nullptr, nullptr, hInst, nullptr);
	}

	void CreateOverlayWindow(HINSTANCE hInst) {
		g_Overlay = CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
			kOverlayClass, L"REVENGE CS Overlay", WS_POPUP,
			0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
			nullptr, nullptr, hInst, nullptr);
		SetLayeredWindowAttributes(g_Overlay, RGB(0, 0, 0), 255, LWA_ALPHA);
		MARGINS margins = { -1 };
		DwmExtendFrameIntoClientArea(g_Overlay, &margins);
	}

	bool InitD3DForBothWindows() {
		if (!CreateD3DDevice()) {
			MessageBoxA(nullptr, "Failed to create the Direct3D 11 device.", "REVENGE CS", MB_OK | MB_ICONERROR);
			return false;
		}
		if (!CreateSwapChainForHwnd(g_Settings, &g_SettingsSwap, &g_SettingsRTV)) {
			MessageBoxA(nullptr, "Failed to create the settings window swap chain.", "REVENGE CS", MB_OK | MB_ICONERROR);
			return false;
		}
		if (!CreateSwapChainForHwnd(g_Overlay, &g_OverlaySwap, &g_OverlayRTV)) {
			MessageBoxA(nullptr, "Failed to create the overlay swap chain.", "REVENGE CS", MB_OK | MB_ICONERROR);
			return false;
		}
		return true;
	}

	void InitImGuiContexts() {
		IMGUI_CHECKVERSION();

		g_SettingsCtx = ImGui::CreateContext();
		ImGui::SetCurrentContext(g_SettingsCtx);
		ImGui::StyleColorsDark();
		ApplyCustomDarkTheme();
		ImGui_ImplWin32_Init(g_Settings);
		ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

		g_OverlayCtx = ImGui::CreateContext();
		ImGui::SetCurrentContext(g_OverlayCtx);
		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(g_Overlay);
		ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
	}

	void PumpWindowMessages() {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) g_Running = false;
		}
	}

	void MaybeReattach() {
		if (mem.IsAttached() && module_base != 0) return;
		static int frame = 0;
		if ((++frame % 120) == 0) AttachAndScan();
	}

	void MaybeReloadSettings() {
		static int frame = 0;
		if ((++frame % 60) == 0) CheckReloadSettings();
	}

		void RenderSettingsWindow() {
			const float clearOpaque[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

		ImGui::SetCurrentContext(g_SettingsCtx);
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		BuildSettingsUI(s_activeTab);
		ImGui::Render();
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_SettingsRTV, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_SettingsRTV, clearOpaque);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		g_SettingsSwap->Present(1, 0);
	}

	void RenderOverlayWindow() {
		const float clearTransparent[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

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

	void ShutdownImGuiContext(ImGuiContext*& ctx) {
		if (!ctx) return;
		ImGui::SetCurrentContext(ctx);
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext(ctx);
		ctx = nullptr;
	}

}

	namespace App
	{
		bool Init() {
			SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
			LoadSettings();
			AttachAndScan();

		s_hInstance = GetModuleHandle(nullptr);
		RegisterWindowClasses(s_hInstance);
		CreateSettingsWindow(s_hInstance);
		CreateOverlayWindow(s_hInstance);
		g_GameWindow = FindWindowW(nullptr, kGameTitle);

		if (!InitD3DForBothWindows()) return false;

		ShowWindow(g_Settings, SW_SHOWDEFAULT); UpdateWindow(g_Settings);
		ShowWindow(g_Overlay,  SW_SHOWDEFAULT); UpdateWindow(g_Overlay);

		InitImGuiContexts();
		Hotkeys::Start();
		return true;
	}

	void Run() {
		while (g_Running) {
			PumpWindowMessages();
			if (!g_Running) break;

			if (!g_GameWindow) g_GameWindow = FindWindowW(nullptr, kGameTitle);
			SetWindowPos(g_Overlay, HWND_TOPMOST, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

			MaybeReattach();
			MaybeReloadSettings();

			RenderSettingsWindow();
			RenderOverlayWindow();
		}
	}

		void Shutdown() {
			SaveSettings();
			ShutdownVisibleCheck();
			Hotkeys::Stop();

		if (g_Overlay)  { DestroyWindow(g_Overlay);  g_Overlay  = nullptr; }
		if (g_Settings) { DestroyWindow(g_Settings); g_Settings = nullptr; }

		ShutdownImGuiContext(g_OverlayCtx);
		ShutdownImGuiContext(g_SettingsCtx);

		CleanupSwapChain(&g_OverlaySwap,  &g_OverlayRTV);
		CleanupSwapChain(&g_SettingsSwap, &g_SettingsRTV);
		CleanupD3DDevice();

		UnregisterClassW(kOverlayClass,  s_hInstance);
		UnregisterClassW(kSettingsClass, s_hInstance);
	}
}