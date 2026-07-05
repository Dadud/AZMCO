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
// Phase 4 state struct.
// mcity treats this like an interface and reads offset 0xC as a vtable
// function pointer. We put the Device pointer there so any vtable access
// routes through D3D11's real vtable (which will return sensible errors
// for methods mcity shouldn't be calling on a D3D device).
//
// Actually: mcity derefs offset 0xC of g_DX11 as *(void**) and tries to
// call whatever function pointer it finds. With g_DX11.Device at offset 0xC,
// mcity would read *(Device + 0xC) which is device->lpVtbl[3] = AddRef
// or similar. AddRef is harmless. Release is harmless. mcity will just call
// AddRef/Release on our device, which is fine.
//
// We keep RenderTargetView in the struct for our own use (offset 0x10+).
static struct DX11State {
    ID3D11Device* Device;             // 0x00 - primary D3D11 device
    ID3D11DeviceContext* Context;     // 0x04 - immediate context
    IDXGISwapChain* SwapChain;        // 0x08 - swap chain (created in CreateGameWindow)
    ID3D11Device* McityView;          // 0x0C - mcity reads this as vtable ptr (alias for Device)
    ID3D11RenderTargetView* RenderTargetView;  // 0x10 - the back buffer RTV
    BOOL IsInitialized;               // 0x14 - init complete flag
    u32 DeviceCount;                  // 0x18 - mirrors upstream Devices.Count
    void* Window;                     // 0x1C - the active window handle (HWND)
} g_DX11 = { nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr };

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
BOOL WINAPI CreateGameWindow(const u32 width, const u32 height, const u32 format, const u32 options) {
    // Phase 4: This is the real signature mcity calls (4 u32 args, not HWND).
    // mcity manages windows separately and passes a window index to draw funcs.
    //
    // We don't have an HWND here so we can't create the swap chain yet.
    // The swap chain will be created in AcquireRendererInstance when we get
    // the HWND. For now, just return a window index.
    g_DX11.DeviceCount = 1;
    return 1; // window index 1
}

BOOL WINAPI DestroyGameWindow(const u32 indx)
{
    (void)indx;
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
BOOL WINAPI ClearGameWindow(void)
{
    if (!g_DX11.IsInitialized || !g_DX11.Context) return FALSE;

    const float clearColor[4] = { 0.05f, 0.10f, 0.20f, 1.0f };  // dark blue
    g_DX11.Context->ClearRenderTargetView(g_DX11.RenderTargetView, clearColor);
    return TRUE;
}
BOOL WINAPI ClipGameWindow(const u32 x0, const u32 y0, const u32 x1, const u32 y1) { return TRUE; }

BOOL WINAPI AcquireRendererDevice(HWND hwnd) { return TRUE; }
BOOL WINAPI AcquireRendererSurface(HWND hwnd) { return TRUE; }
BOOL WINAPI InitializeVertexBuffer(void* buffer, u32 stride, u32 format, u32 width, u32 height) { return TRUE; }
BOOL WINAPI OptimizeVertexBuffer(void* buffer) { return TRUE; }
BOOL WINAPI LockWindow(BOOL state) { return TRUE; }
BOOL WINAPI RenderPacket(const void* packet) { return TRUE; }
BOOL WINAPI RenderBufferedPacket(const void* packet) { return TRUE; }
BOOL WINAPI RenderPacketDX8(const void* packet) { return TRUE; }
BOOL WINAPI AcquireRendererInstance(const void* instance) { return TRUE; }
BOOL WINAPI RestoreRendererSurfaces(void) { return TRUE; }
BOOL WINAPI SelectState(u32 state, void* value) { return TRUE; }
BOOL WINAPI AcquireState(u32 state, void* value) { return TRUE; }
BOOL WINAPI TestCooperativeLevel(void) { return TRUE; }
BOOL WINAPI AcquireGuardBands(void* bands) { return TRUE; }
BOOL WINAPI AcquireTransformAndLightCapabilities(void* caps) { return TRUE; }
BOOL WINAPI AcquireDeviceCapabilities8(void* caps) { return TRUE; }
BOOL WINAPI AcquireRendererObjects(void) { return TRUE; }
BOOL WINAPI AcquireMainRendererSurface(void) { return TRUE; }
BOOL WINAPI AcquireBackRendererSurface(void) { return TRUE; }
BOOL WINAPI AcquireRendererObjectsDX8(void) { return TRUE; }
BOOL WINAPI AcquireMaxAnisotropyDX8(u32* max) { if (max) *max = 16; return TRUE; }
BOOL WINAPI AcquireTransformWorld(void* m) { if (m) memset(m, 0, 0x40); return TRUE; }
BOOL WINAPI AcquireTransformView(void* m) { if (m) memset(m, 0, 0x40); return TRUE; }
BOOL WINAPI AcquireTransformProjection(void* m) { if (m) memset(m, 0, 0x40); return TRUE; }
BOOL WINAPI AcquireLight(u32 index, void* light) { return TRUE; }
BOOL WINAPI AcquireCurrentMaterial(void* mat) { return TRUE; }
BOOL WINAPI AcquireDisplayState(u32* state) { if (state) *state = 0; return TRUE; }
BOOL WINAPI AcquireTransformAndLightCapabilitiesDX8(void* caps) { return TRUE; }

BOOL WINAPI DrawLine(u32 a, u32 b) { return TRUE; }
BOOL WINAPI DrawLineMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL WINAPI DrawLineStrip(const u32 count, void* vertexes) { return TRUE; }
BOOL WINAPI DrawPoints(u32 count, const u16* indexes) { return TRUE; }
BOOL WINAPI DrawRectangle(s32 x0, s32 y0, s32 x1, s32 y1, u32 color) { return TRUE; }
BOOL WINAPI DrawRectangles(const s32* rects, u32 count, u32 color) { return TRUE; }
BOOL WINAPI DrawTriangle(void* a, void* b, void* c) { return TRUE; }
BOOL WINAPI DrawTriangleFan(const u32 count, void* vertexes) { return TRUE; }
BOOL WINAPI DrawTriangleMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL WINAPI DrawTriangleStrip(const u32 count, void* vertexes) { return TRUE; }


// === ADDITIONAL STUBS (Phase 2 completion) ===
// These entry points are also exported by the upstream's def file.
BOOL WINAPI FlushGameWindow(void) { return TRUE; }
BOOL WINAPI Idle(void) { return TRUE; }
BOOL WINAPI Init(void)
{
    if (g_DX11.IsInitialized) {
        // Make sure the McityView pointer is in sync with the device.
        // This is what mcity reads at offset 0xC of our state struct.
        if (!g_DX11.McityView) { g_DX11.McityView = g_DX11.Device; }
        return g_DX11.DeviceCount ? g_DX11.DeviceCount : 1;
    }

    // Mark as initialized. Real DX11 device creation happens in CreateGameWindow
    // when we have an HWND. This mirrors the upstream DX8's Init behavior of
    // returning the device count (which is 1 for a single-display system).
    g_DX11.IsInitialized = 1;
    g_DX11.DeviceCount = 1;
    return 1;  // 1 device available
}
BOOL WINAPI Is(void) { return 0; }  // 0 = no acceleration
BOOL WINAPI LockGameWindow(void) { return TRUE; }
BOOL WINAPI ToggleGameWindow(BOOL state) { return TRUE; }
BOOL WINAPI RestoreGameWindow(void) { return TRUE; }
BOOL WINAPI SelectDevice(u32 index) { return TRUE; }
BOOL WINAPI SelectGameWindow(void* window) { return TRUE; }
BOOL WINAPI SelectTexture(u32 index) { return TRUE; }
BOOL WINAPI SelectVideoMode(const u32 mode, const u32 pending, const u32 depth) { return TRUE; }
// Phase 3: Present the back buffer (swap chain Present).
BOOL WINAPI SyncGameWindow(const u32 type)
{
    if (!g_DX11.SwapChain || !g_DX11.Device) return FALSE;
    // type=0 -> standard Present, type=1 -> vsync, type=2 -> no-wait
    u32 sync_interval = 1;  // vsync
    u32 flags = 0;
    if (type == 2) { sync_interval = 0; flags = DXGI_PRESENT_DO_NOT_SEQUENCE; }
    HRESULT hr = g_DX11.SwapChain->Present(sync_interval, flags);
    return SUCCEEDED(hr);
}
BOOL WINAPI UnlockGameWindow(void) { return TRUE; }
BOOL WINAPI ReadRectangle(s32 x0, s32 y0, s32 x1, s32 y1, void* data) { return TRUE; }
BOOL WINAPI ReadRectangles(const s32* rects, u32 count, void* data) { return TRUE; }
BOOL WINAPI WriteRectangle(s32 x0, s32 y0, s32 x1, s32 y1, const void* data) { return TRUE; }
BOOL WINAPI WriteRectangles(const s32* rects, u32 count, const void* data) { return TRUE; }
BOOL WINAPI AcquireGameWindowTexture(u32 index, void* texture) { return TRUE; }
BOOL WINAPI AllocateTexture(u32 index, u32 width, u32 height, u32 format, u32 location, u32 mipmaps) { return TRUE; }
BOOL WINAPI ReleaseTexture(u32 index) { return TRUE; }
BOOL WINAPI ResetTextures(void) { return TRUE; }
BOOL WINAPI UpdateTexture(u32 index, u32 level, u32 x, u32 y, u32 width, u32 height, const void* data) { return TRUE; }
BOOL WINAPI UpdateTextureRectangle(u32 index, u32 level, const s32* rects, u32 count, const void* data) { return TRUE; }
BOOL WINAPI DrawLineStrips(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL WINAPI DrawPoint(void* vertex) { return TRUE; }
BOOL WINAPI DrawPointMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL WINAPI DrawPointStrip(const u32 count, void* vertexes) { return TRUE; }
BOOL WINAPI DrawQuad(void* a, void* b, void* c, void* d) { return TRUE; }
BOOL WINAPI DrawQuadMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL WINAPI DrawSprite(void* a, void* b) { return TRUE; }
BOOL WINAPI DrawSpriteMesh(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL WINAPI DrawTriangleFans(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
BOOL WINAPI DrawTriangleStrips(const u32 count, void* vertexes, const u32* indexes) { return TRUE; }
} // extern "C"