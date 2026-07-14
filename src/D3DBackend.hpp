#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>

#include "imgui.h"
#include "imgui_impl_win32.h"

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;

extern IDXGISwapChain* g_SettingsSwap;
extern ID3D11RenderTargetView* g_SettingsRTV;
extern IDXGISwapChain* g_OverlaySwap;
extern ID3D11RenderTargetView* g_OverlayRTV;

extern ImGuiContext* g_SettingsCtx;
extern ImGuiContext* g_OverlayCtx;

extern bool g_Running;

extern HWND g_Settings;
extern HWND g_Overlay;
extern HWND g_GameWindow;

bool CreateD3DDevice();
void CleanupD3DDevice();
bool CreateSwapChainForHwnd(HWND hWnd, IDXGISwapChain** outSwap, ID3D11RenderTargetView** outRTV);
void CleanupSwapChain(IDXGISwapChain** swap, ID3D11RenderTargetView** rtv);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
