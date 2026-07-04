/*
Copyright (c) 2024 Americus Maximus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY PARTY'S CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Phase 2 skeleton: R.DirectX.11.0.A — DX11 backend stub
//
// Strategy:
// - Use D3D11 headers (via DirectX11.hxx)
// - Stub every entry point to return success/no-op
// - CreateGameWindow / DestroyGameWindow with real D3D11 device + swap chain
//
// Phase 3-4 will convert the remaining state setters and draw calls.

#include "DirectX11.hxx"
#include <cstdio>

// MinGW doesn't define IID_ID3D11Texture2D by default (it's declared extern
// in d3d11.h). Define it inline here for the swap-chain GetBuffer call.
// This is the standard D3D11 texture2D interface ID (matches d3d11.h).
extern "C" const GUID IID_ID3D11Texture2D = {
    0x6f15aaf2, 0xd208, 0x4e89, { 0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c }
};  // snprintf

// Forward declarations to avoid ordering issues
extern "C" BOOL DestroyGameWindow(void);

#define RENDERER_MODULE_NAME "DX11 3rash"
#ifdef _WIN64
#define RENDERER_MODULE_TITLE_NAME "DirectX 11 (x64)"
#else
#define RENDERER_MODULE_TITLE_NAME "DirectX 11 (x32)"
#endif
#define RENDERER_MODULE_AUTHOR "Americus Maximus"

#define RENDERER_MODULE_SIGNATURE_D3D8 0x44334438
#define RENDERER_MODULE_VERSION_115 115
#define RENDERER_MODULE_VERSION_DX8 8
#define MAX_RENDERER_MODULE_DEVICE_NAME_LENGTH 32
#define MAX_RENDERER_MODULE_DEVICE_LONG_NAME_LENGTH 80

// Minimal type aliases (normally come from AZX/Basic.hxx)
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef int BOOL;
typedef unsigned long DWORD;
typedef void* LPVOID;

// Phase 2: state placeholder. Phase 3 will move to real renderer state.
static struct DX11State {
    ID3D11Device* Device;
    ID3D11DeviceContext* Context;
    IDXGISwapChain* SwapChain;
    ID3D11RenderTargetView* RenderTargetView;
    BOOL IsInitialized;
} g_DX11 = { nullptr, nullptr, nullptr, nullptr, 0 };

// RendererModuleDescriptor (normally from RendererModule.Basic.hxx).
// Phase 2 skeleton defines only what we need for AcquireDescriptor.
struct RendererModuleDescriptor {
    u32 Signature;
    s32 Size;
    u32 Version;
    u32 Caps;
    u32 MinimumTextureWidth;
    u32 MaximumTextureWidth;
    u32 MultipleTextureWidth;
    u32 MinimumTextureHeight;
    u32 MaximumTextureHeight;
    u32 MultipleTextureHeight;
    u32 ClipAlign;
    u32 ActiveTextureFormatStatesCount;
    s32* TextureFormatStates;
    u32 ActiveUnknownValuesCount;
    s32* UnknownValues;
    struct {
        u32 Count;
        void* Capabilities;
    } Capabilities;
    u32 MaximumSimultaneousTextures;
    s32 Unk7;
    char Name[MAX_RENDERER_MODULE_DEVICE_NAME_LENGTH];
    u32 SubType;
    u32 MemorySize;
    u32 MemoryType;
    const char* Author;
    u32 DXV;
    char DeviceName[MAX_RENDERER_MODULE_DEVICE_LONG_NAME_LENGTH];
};

extern "C" {

const RendererModuleDescriptor* AcquireDescriptor()
{
    static RendererModuleDescriptor descriptor;
    static u8 firstCall = 1;
    if (firstCall) {
        firstCall = 0;
        memset(&descriptor, 0, sizeof(descriptor));
        descriptor.Signature = RENDERER_MODULE_SIGNATURE_D3D8;
        descriptor.Size = sizeof(descriptor);
        descriptor.Version = RENDERER_MODULE_VERSION_115;
        descriptor.SubType = RENDERER_MODULE_VERSION_DX8;
        descriptor.Author = RENDERER_MODULE_AUTHOR;
        descriptor.DXV = RENDERER_MODULE_VERSION_DX8;
        const char name[] = RENDERER_MODULE_NAME;
        for (u32 i = 0; i < sizeof(descriptor.Name) && name[i]; i++) {
            descriptor.Name[i] = name[i];
        }
        const char devName[] = RENDERER_MODULE_TITLE_NAME;
        for (u32 i = 0; i < sizeof(descriptor.DeviceName) && devName[i]; i++) {
            descriptor.DeviceName[i] = devName[i];
        }
    }
    return &descriptor;
}

} // extern "C"

// === STUB IMPLEMENTATIONS ===
// Every entry point is a stub for Phase 2 skeleton.
// Phase 3-4 will replace with real DX11 logic.

extern "C" {

// Phase 3: Real D3D11 device + swap chain creation.
// Returns TRUE on success, FALSE on any failure.
// On failure, all D3D11 resources are released and g_DX11 state is reset.
BOOL CreateGameWindow(HWND hwnd)
{
    if (g_DX11.IsInitialized) {
        OutputDebugStringA("[R.DirectX.11.0.A] CreateGameWindow called while already initialized; tearing down first\n");
        DestroyGameWindow();
    }

    if (!hwnd) {
        OutputDebugStringA("[R.DirectX.11.0.A] CreateGameWindow: NULL HWND\n");
        return FALSE;
    }

    // Get window dimensions for the swap chain
    RECT rc;
    GetClientRect(hwnd, &rc);
    u32 width  = (rc.right - rc.left) > 0 ? (u32)(rc.right - rc.left) : 800;
    u32 height = (rc.bottom - rc.top) > 0 ? (u32)(rc.bottom - rc.top) : 600;

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    // default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,                    // no software module
        0,                          // no flags (debug layer is set by host if needed)
        nullptr,                    // default feature levels
        0,
        D3D11_SDK_VERSION,
        &scd,
        &g_DX11.SwapChain,
        &g_DX11.Device,
        &featureLevel,
        &g_DX11.Context
    );

    if (FAILED(hr)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "[R.DirectX.11.0.A] D3D11CreateDeviceAndSwapChain failed: 0x%08lX\n", hr);
        OutputDebugStringA(msg);
        return FALSE;
    }

    // Create render target view from back buffer
    ID3D11Texture2D* backBuffer = nullptr;
    hr = g_DX11.SwapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
    if (FAILED(hr)) {
        OutputDebugStringA("[R.DirectX.11.0.A] GetBuffer failed\n");
        DestroyGameWindow();
        return FALSE;
    }

    hr = g_DX11.Device->CreateRenderTargetView(backBuffer, nullptr, &g_DX11.RenderTargetView);
    backBuffer->Release();
    if (FAILED(hr)) {
        OutputDebugStringA("[R.DirectX.11.0.A] CreateRenderTargetView failed\n");
        DestroyGameWindow();
        return FALSE;
    }

    g_DX11.Context->OMSetRenderTargets(1, &g_DX11.RenderTargetView, nullptr);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    g_DX11.Context->RSSetViewports(1, &viewport);

    char msg[256];
    snprintf(msg, sizeof(msg), "[R.DirectX.11.0.A] CreateGameWindow: %ux%u swap chain, feature level 0x%X\n",
        width, height, featureLevel);
    OutputDebugStringA(msg);

    g_DX11.IsInitialized = 1;
    return TRUE;
}

BOOL DestroyGameWindow(void)
{
    if (g_DX11.RenderTargetView) { g_DX11.RenderTargetView->Release(); g_DX11.RenderTargetView = nullptr; }
    if (g_DX11.SwapChain) { g_DX11.SwapChain->Release(); g_DX11.SwapChain = nullptr; }
    if (g_DX11.Context) { g_DX11.Context->Release(); g_DX11.Context = nullptr; }
    if (g_DX11.Device) { g_DX11.Device->Release(); g_DX11.Device = nullptr; }
    g_DX11.IsInitialized = 0;
    return TRUE;
}

// Phase 3: Clear the render target to a known color.
// Default clear is dark blue (matches MCO's loading screen background).
// Color can be overridden via state setters in Phase 6 (DX11 polish).
BOOL ClearGameWindow(void)
{
    if (!g_DX11.IsInitialized || !g_DX11.Context) return FALSE;

    const float clearColor[4] = { 0.05f, 0.10f, 0.20f, 1.0f };  // dark blue
    g_DX11.Context->ClearRenderTargetView(g_DX11.RenderTargetView, clearColor);
    return TRUE;
}
BOOL ClipGameWindow(s32 left, s32 top, s32 right, s32 bottom) { return TRUE; }

BOOL AcquireRendererDevice(HWND hwnd) { return TRUE; }
BOOL AcquireRendererSurface(HWND hwnd) { return TRUE; }
BOOL InitializeVertexBuffer(void* buffer, u32 stride, u32 format, u32 width, u32 height) { return TRUE; }
BOOL OptimizeVertexBuffer(void* buffer) { return TRUE; }
BOOL LockWindow(BOOL state) { return TRUE; }
BOOL RenderPacket(const void* packet) { return TRUE; }
BOOL RenderBufferedPacket(const void* packet) { return TRUE; }
BOOL RenderPacketDX8(const void* packet) { return TRUE; }
BOOL AcquireRendererInstance(const void* instance) { return TRUE; }
BOOL RestoreRendererSurfaces(void) { return TRUE; }
BOOL SelectState(u32 state, void* value) { return TRUE; }
BOOL AcquireState(u32 state, void* value) { return TRUE; }
BOOL TestCooperativeLevel(void) { return TRUE; }
BOOL AcquireGuardBands(void* bands) { return TRUE; }
BOOL AcquireTransformAndLightCapabilities(void* caps) { return TRUE; }
BOOL AcquireDeviceCapabilities8(void* caps) { return TRUE; }
BOOL AcquireRendererObjects(void) { return TRUE; }
BOOL AcquireMainRendererSurface(void) { return TRUE; }
BOOL AcquireBackRendererSurface(void) { return TRUE; }
BOOL AcquireRendererObjectsDX8(void) { return TRUE; }
BOOL AcquireMaxAnisotropyDX8(u32* max) { if (max) *max = 16; return TRUE; }
BOOL AcquireTransformWorld(void* m) { if (m) memset(m, 0, 0x40); return TRUE; }
BOOL AcquireTransformView(void* m) { if (m) memset(m, 0, 0x40); return TRUE; }
BOOL AcquireTransformProjection(void* m) { if (m) memset(m, 0, 0x40); return TRUE; }
BOOL AcquireLight(u32 index, void* light) { return TRUE; }
BOOL AcquireCurrentMaterial(void* mat) { return TRUE; }
BOOL AcquireDisplayState(u32* state) { if (state) *state = 0; return TRUE; }
BOOL AcquireTransformAndLightCapabilitiesDX8(void* caps) { return TRUE; }

BOOL DrawLine(u32 a, u32 b) { return TRUE; }
BOOL DrawLineMesh(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawLineStrip(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawPoints(u32 count, const u16* indexes) { return TRUE; }
BOOL DrawRectangle(s32 x0, s32 y0, s32 x1, s32 y1, u32 color) { return TRUE; }
BOOL DrawRectangles(const s32* rects, u32 count, u32 color) { return TRUE; }
BOOL DrawTriangle(u32 a, u32 b, u32 c) { return TRUE; }
BOOL DrawTriangleFan(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawTriangleMesh(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawTriangleStrip(const u16* indexes, u32 count) { return TRUE; }


// === ADDITIONAL STUBS (Phase 2 completion) ===
// These entry points are also exported by the upstream's def file.
BOOL FlushGameWindow(void) { return TRUE; }
BOOL Idle(void) { return TRUE; }
BOOL Init(void* p1, void* p2, void* p3) { return TRUE; }
BOOL Is(void) { return TRUE; }
BOOL LockGameWindow(void) { return TRUE; }
BOOL ToggleGameWindow(BOOL state) { return TRUE; }
BOOL RestoreGameWindow(void) { return TRUE; }
BOOL SelectDevice(u32 index) { return TRUE; }
BOOL SelectGameWindow(void* window) { return TRUE; }
BOOL SelectTexture(u32 index) { return TRUE; }
BOOL SelectVideoMode(u32 mode) { return TRUE; }
// Phase 3: Present the back buffer (swap chain Present).
BOOL SyncGameWindow(void)
{
    if (!g_DX11.IsInitialized || !g_DX11.SwapChain) return FALSE;
    return SUCCEEDED(g_DX11.SwapChain->Present(1, 0));  // 1 = sync interval (vsync)
}
BOOL UnlockGameWindow(void) { return TRUE; }
BOOL ReadRectangle(s32 x0, s32 y0, s32 x1, s32 y1, void* data) { return TRUE; }
BOOL ReadRectangles(const s32* rects, u32 count, void* data) { return TRUE; }
BOOL WriteRectangle(s32 x0, s32 y0, s32 x1, s32 y1, const void* data) { return TRUE; }
BOOL WriteRectangles(const s32* rects, u32 count, const void* data) { return TRUE; }
BOOL AcquireGameWindowTexture(u32 index, void* texture) { return TRUE; }
BOOL AllocateTexture(u32 index, u32 width, u32 height, u32 format, u32 location, u32 mipmaps) { return TRUE; }
BOOL ReleaseTexture(u32 index) { return TRUE; }
BOOL ResetTextures(void) { return TRUE; }
BOOL UpdateTexture(u32 index, u32 level, u32 x, u32 y, u32 width, u32 height, const void* data) { return TRUE; }
BOOL UpdateTextureRectangle(u32 index, u32 level, const s32* rects, u32 count, const void* data) { return TRUE; }
BOOL DrawLineStrips(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawPoint(u32 index) { return TRUE; }
BOOL DrawPointMesh(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawPointStrip(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawQuad(u32 a, u32 b, u32 c, u32 d) { return TRUE; }
BOOL DrawQuadMesh(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawSprite(s32 x, s32 y, u32 color) { return TRUE; }
BOOL DrawSpriteMesh(const s32* positions, u32 count, u32 color) { return TRUE; }
BOOL DrawTriangleFans(const u16* indexes, u32 count) { return TRUE; }
BOOL DrawTriangleStrips(const u16* indexes, u32 count) { return TRUE; }
} // extern "C"