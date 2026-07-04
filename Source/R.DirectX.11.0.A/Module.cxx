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

// Minimal Windows SDK include. Use Windows SDK types directly since
// the AZX/Basic.hxx header pulls in D3D8-specific stuff that doesn't
// belong in the DX11 backend.
#include <windows.h>

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
        // Match the original EA dx8z.dll's descriptor so mcity's
        // "is this a 3rash DLL?" check passes.
        descriptor.SubType = 0;  // RENDERER_MODULE_VERSION_104
        descriptor.DXV = 8;
        descriptor.Author = "Mike Ockenden, Thursday 05:09PM Aug 30, 2001";
        const char name[] = "DX8 3rash";
        for (u32 i = 0; i < sizeof(descriptor.Name) && name[i]; i++) {
            descriptor.Name[i] = name[i];
        }
        // DeviceName left empty to match original
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
BOOL CreateGameWindow(const u32 width, const u32 height, const u32 format, const u32 options) {
    // Phase 3: This is the REAL signature mcity expects (4 u32 args = 16 bytes).
    // The HWND is NOT passed here - mcity manages windows separately and passes
    // window indices to the draw functions.
    //
    // For now, return a fake window index. Phase 4 will create real DX11 surfaces.
    return 1; // Fake window index 1
}

BOOL DestroyGameWindow(const u32 indx)
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
BOOL ClipGameWindow(const u32 x0, const u32 y0, const u32 x1, const u32 y1) { return TRUE; }

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
BOOL DrawLineMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL DrawLineStrip(const u32 count, void* vertexes) { return TRUE; }
BOOL DrawPoints(u32 count, const u16* indexes) { return TRUE; }
BOOL DrawRectangle(s32 x0, s32 y0, s32 x1, s32 y1, u32 color) { return TRUE; }
BOOL DrawRectangles(const s32* rects, u32 count, u32 color) { return TRUE; }
BOOL DrawTriangle(void* a, void* b, void* c) { return TRUE; }
BOOL DrawTriangleFan(const u32 count, void* vertexes) { return TRUE; }
BOOL DrawTriangleMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL DrawTriangleStrip(const u32 count, void* vertexes) { return TRUE; }


// === ADDITIONAL STUBS (Phase 2 completion) ===
// These entry points are also exported by the upstream's def file.
BOOL FlushGameWindow(void) { return TRUE; }
BOOL Idle(void) { return TRUE; }
BOOL Init(void) { return 0; }  // 0 = no renderer (NULL instance pointer)
BOOL Is(void) { return 0; }  // 0 = no acceleration
BOOL LockGameWindow(void) { return TRUE; }
BOOL ToggleGameWindow(BOOL state) { return TRUE; }
BOOL RestoreGameWindow(void) { return TRUE; }
BOOL SelectDevice(u32 index) { return TRUE; }
BOOL SelectGameWindow(void* window) { return TRUE; }
BOOL SelectTexture(u32 index) { return TRUE; }
BOOL SelectVideoMode(const u32 mode, const u32 pending, const u32 depth) { return TRUE; }
// Phase 3: Present the back buffer (swap chain Present).
BOOL SyncGameWindow(const u32 type)
{
    if (!g_DX11.SwapChain) return FALSE;
    // type=0 -> standard Present, type=1 -> vsync, type=2 -> no-wait
    u32 sync_interval = 1;  // vsync
    u32 flags = 0;
    if (type == 2) { sync_interval = 0; flags = DXGI_PRESENT_DO_NOT_SEQUENCE; }
    HRESULT hr = g_DX11.SwapChain->Present(sync_interval, flags);
    return SUCCEEDED(hr);
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
BOOL DrawLineStrips(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL DrawPoint(void* vertex) { return TRUE; }
BOOL DrawPointMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL DrawPointStrip(const u32 count, void* vertexes) { return TRUE; }
BOOL DrawQuad(void* a, void* b, void* c, void* d) { return TRUE; }
BOOL DrawQuadMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL DrawSprite(void* a, void* b) { return TRUE; }
BOOL DrawSpriteMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL DrawTriangleFans(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL DrawTriangleStrips(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
} // extern "C"