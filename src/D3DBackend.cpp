#include "D3DBackend.hpp"

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

IDXGISwapChain* g_SettingsSwap = nullptr;
ID3D11RenderTargetView* g_SettingsRTV = nullptr;
IDXGISwapChain* g_OverlaySwap = nullptr;
ID3D11RenderTargetView* g_OverlayRTV = nullptr;

ImGuiContext* g_SettingsCtx = nullptr;
ImGuiContext* g_OverlayCtx = nullptr;

bool g_Running = true;

HWND g_Settings = nullptr;
HWND g_Overlay = nullptr;
HWND g_GameWindow = nullptr;

bool CreateD3DDevice() {
	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	HRESULT res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
		featureLevelArray, 2, D3D11_SDK_VERSION,
		&g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) {
		res = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
			featureLevelArray, 2, D3D11_SDK_VERSION,
			&g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	}
	return SUCCEEDED(res);
}

void CleanupD3DDevice() {
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice = nullptr; }
}

bool CreateSwapChainForHwnd(HWND hWnd, IDXGISwapChain** outSwap, ID3D11RenderTargetView** outRTV) {
	IDXGIDevice* dxgiDevice = nullptr;
	if (FAILED(g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice))) return false;
	IDXGIAdapter* adapter = nullptr;
	if (FAILED(dxgiDevice->GetAdapter(&adapter))) { dxgiDevice->Release(); return false; }
	IDXGIFactory* factory = nullptr;
	if (FAILED(adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory))) {
		adapter->Release(); dxgiDevice->Release(); return false;
	}

	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 2;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	HRESULT hr = factory->CreateSwapChain(g_pd3dDevice, &sd, outSwap);
	factory->Release(); adapter->Release(); dxgiDevice->Release();
	if (FAILED(hr)) return false;

	ID3D11Texture2D* backBuffer = nullptr;
	(*outSwap)->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	g_pd3dDevice->CreateRenderTargetView(backBuffer, nullptr, outRTV);
	backBuffer->Release();
	return true;
}

void CleanupSwapChain(IDXGISwapChain** swap, ID3D11RenderTargetView** rtv) {
	if (rtv && *rtv)   { (*rtv)->Release();  *rtv = nullptr; }
	if (swap && *swap) { (*swap)->Release(); *swap = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ImGuiContext* ctxForThisHwnd = nullptr;
	IDXGISwapChain** swapForThisHwnd = nullptr;
	ID3D11RenderTargetView** rtvForThisHwnd = nullptr;
	if (hWnd == g_Settings) {
		ctxForThisHwnd = g_SettingsCtx;
		swapForThisHwnd = &g_SettingsSwap;
		rtvForThisHwnd = &g_SettingsRTV;
	} else if (hWnd == g_Overlay) {
		ctxForThisHwnd = g_OverlayCtx;
		swapForThisHwnd = &g_OverlaySwap;
		rtvForThisHwnd = &g_OverlayRTV;
	}

	if (ctxForThisHwnd) {
		ImGuiContext* prev = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(ctxForThisHwnd);
		LRESULT handled = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		ImGui::SetCurrentContext(prev);
		if (handled) return handled;
	}

	switch (msg) {
	case WM_SIZE:
		if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED && swapForThisHwnd && rtvForThisHwnd && *swapForThisHwnd) {
			if (*rtvForThisHwnd) { (*rtvForThisHwnd)->Release(); *rtvForThisHwnd = nullptr; }
			(*swapForThisHwnd)->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			ID3D11Texture2D* bb = nullptr;
			(*swapForThisHwnd)->GetBuffer(0, IID_PPV_ARGS(&bb));
			g_pd3dDevice->CreateRenderTargetView(bb, nullptr, rtvForThisHwnd);
			bb->Release();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
		break;
	case WM_CLOSE:
		g_Running = false;
		return 0;
	case WM_DESTROY:
		if (hWnd == g_Settings) PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}
