#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>

#include "Imgui/imconfig.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx11.h"
#include "Imgui/imgui_impl_win32.h"
#include "Imgui/imgui_internal.h"
#include "Imgui/imstb_rectpack.h"
#include "Imgui/imstb_textedit.h"
#include "Imgui/imstb_truetype.h"

#include <string>
#include <d3d11.h>

#pragma comment(lib,"d3d11.lib")

// DirectX相关变量声明
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern UINT g_ResizeWidth, g_ResizeHeight;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern bool g_SwapChainOccluded;
extern ID3D11ShaderResourceView* g_background;

void MainWindow();