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
//
// Phase 5: One global interface struct + per-window slots. mcity can ask for
// up to MAX_WINDOWS windows via CreateGameWindow and switch between them via
// SelectGameWindow. Each slot holds its own DX11 device + swap chain + RTV.
#define MAX_WINDOWS 4
static struct DX11WindowSlot {
    ID3D11Device* Device;
    ID3D11DeviceContext* Context;
    IDXGISwapChain* SwapChain;
    ID3D11RenderTargetView* RenderTargetView;
    void* HWND;                    // captured via SelectState(SELECT_WINDOW=25, hwnd)
    u32 Width;
    u32 Height;
    u32 Format;
    u32 Options;
    u32 IsAllocated;               // CreateGameWindow returned this index
} g_Windows[MAX_WINDOWS] = {};

static struct DX11State {
    ID3D11Device* Device;             // 0x00 - primary D3D11 device (alias for active slot)
    ID3D11DeviceContext* Context;     // 0x04 - immediate context
    IDXGISwapChain* SwapChain;        // 0x08 - swap chain (alias for active slot)
    ID3D11Device* McityView;          // 0x0C - mcity reads this as vtable ptr (alias for Device)
    ID3D11RenderTargetView* RenderTargetView;  // 0x10 - the back buffer RTV (alias)
    BOOL IsInitialized;               // 0x14 - init complete flag
    u32 DeviceCount;                  // 0x18 - mirrors upstream Devices.Count
    void* Window;                     // 0x1C - the active window handle (HWND)
    u32 ActiveSlot;                   // index into g_Windows, set by SelectGameWindow
} g_DX11 = { nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, 0 };

// Returns the active window slot, or nullptr if no slot is active.
static DX11WindowSlot* ActiveSlot() {
    if (g_DX11.ActiveSlot >= MAX_WINDOWS) return nullptr;
    if (!g_Windows[g_DX11.ActiveSlot].IsAllocated) return nullptr;
    return &g_Windows[g_DX11.ActiveSlot];
}

// Syncs the global g_DX11 interface struct's pointer aliases to point at the
// currently active slot's resources. mcity reads offsets 0x00/0x04/0x08/0x10
// as vtable/etc. pointers so the aliases must match the active slot.
static void SyncInterfaceFromActive() {
    DX11WindowSlot* s = ActiveSlot();
    g_DX11.Device = s ? s->Device : nullptr;
    g_DX11.Context = s ? s->Context : nullptr;
    g_DX11.SwapChain = s ? s->SwapChain : nullptr;
    g_DX11.RenderTargetView = s ? s->RenderTargetView : nullptr;
    g_DX11.Window = s ? s->HWND : nullptr;
    g_DX11.McityView = g_DX11.Device;  // alias
}

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

// Phase 5: state constants. We don't pull in AZX/RendererModule.Basic.hxx
// (it's D3D8-specific) so we hardcode the state IDs we care about.
#define RENDERER_MODULE_STATE_SELECT_WINDOW 25

extern "C" {

// Phase 5: Allocate a new DX11 device + swap chain for a game window.
//
// mcity calls this with (width, height, format, options) and expects back
// a window index (u32, 0-based in upstream). We find the next free slot
// in g_Windows, create a D3D11 device + swap chain bound to the HWND that
// was captured via SelectState(SELECT_WINDOW=25, hwnd), and return the slot
// index. If we don't have an HWND yet, we still return a slot index but
// flag it as needing HWND (the slot's HWND field stays nullptr until
// SelectState fires).
//
// Returns: window index on success, 0 on failure (0 is reserved/unused in
// the upstream's MIN_WINDOW_INDEX scheme but we'll use 0 as our "no slots"
// return value; the upstream returns RENDERER_MODULE_FAILURE on failure).
u32 WINAPI CreateGameWindow(const u32 width, const u32 height, const u32 format, const u32 options)
{
    // Find a free slot.
    u32 slot = MAX_WINDOWS;
    for (u32 i = 0; i < MAX_WINDOWS; i++) {
        if (!g_Windows[i].IsAllocated) { slot = i; break; }
    }
    if (slot >= MAX_WINDOWS) {
        // Out of slots. Upstream would return RENDERER_MODULE_FAILURE.
        // Returning 0 lets mcity distinguish from valid indices (1..MAX_WINDOWS).
        return 0;
    }

    DX11WindowSlot* s = &g_Windows[slot];
    s->Width = width;
    s->Height = height;
    s->Format = format;
    s->Options = options;
    s->IsAllocated = 1;
    g_DX11.DeviceCount++;

    // If we already have an HWND (from an earlier SelectState call), create
    // the device + swap chain now. Otherwise we mark the slot as pending and
    // create the device when SelectState fires.
    if (s->HWND != nullptr) {
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 1;
        scd.BufferDesc.Width = width ? width : 800;
        scd.BufferDesc.Height = height ? height : 600;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferDesc.RefreshRate.Numerator = 60;
        scd.BufferDesc.RefreshRate.Denominator = 1;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = (HWND)s->HWND;
        scd.SampleDesc.Count = 1;
        scd.SampleDesc.Quality = 0;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            &featureLevel, 1, D3D11_SDK_VERSION,
            &scd, &s->SwapChain, &s->Device, nullptr, &s->Context);
        if (SUCCEEDED(hr)) {
            ID3D11Texture2D* backBuffer = nullptr;
            hr = s->SwapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
            if (SUCCEEDED(hr)) {
                s->Device->CreateRenderTargetView(backBuffer, nullptr, &s->RenderTargetView);
                backBuffer->Release();
                s->Context->OMSetRenderTargets(1, &s->RenderTargetView, nullptr);

                D3D11_VIEWPORT vp = {};
                vp.Width = (float)(width ? width : 800);
                vp.Height = (float)(height ? height : 600);
                vp.MinDepth = 0.0f;
                vp.MaxDepth = 1.0f;
                s->Context->RSSetViewports(1, &vp);
            }
        }
        // If creation failed, the slot still has its IsAllocated=1 but
        // no device. Subsequent operations will no-op until HWND changes.
    }

    // Make this the active slot by default so subsequent draws work.
    g_DX11.ActiveSlot = slot;
    SyncInterfaceFromActive();

    // Return 1-based index to match upstream's MIN_WINDOW_INDEX scheme.
    return slot + 1;
}

BOOL WINAPI DestroyGameWindow(const u32 indx)
{
    // indx is 1-based (returned from CreateGameWindow as slot+1).
    if (indx == 0 || indx > MAX_WINDOWS) return FALSE;
    DX11WindowSlot* s = &g_Windows[indx - 1];
    if (!s->IsAllocated) return FALSE;

    if (s->RenderTargetView) { s->RenderTargetView->Release(); s->RenderTargetView = nullptr; }
    if (s->SwapChain) { s->SwapChain->Release(); s->SwapChain = nullptr; }
    if (s->Context) { s->Context->Release(); s->Context = nullptr; }
    if (s->Device) { s->Device->Release(); s->Device = nullptr; }
    s->HWND = nullptr;
    s->IsAllocated = 0;
    if (g_DX11.DeviceCount > 0) g_DX11.DeviceCount--;

    // If we just destroyed the active slot, clear the active pointer.
    if (g_DX11.ActiveSlot == indx - 1) {
        g_DX11.ActiveSlot = MAX_WINDOWS;  // invalid
        SyncInterfaceFromActive();
    }
    return TRUE;
}

// Phase 5: Clear the render target to a known color.
// Uses the active window slot's RTV. If no slot is active, no-op.
BOOL WINAPI ClearGameWindow(void)
{
    DX11WindowSlot* s = ActiveSlot();
    if (!s || !s->Context || !s->RenderTargetView) return FALSE;

    const float clearColor[4] = { 0.05f, 0.10f, 0.20f, 1.0f };  // dark blue
    s->Context->ClearRenderTargetView(s->RenderTargetView, clearColor);
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
// Phase 5: SelectState intercepts the HWND when mcity calls with
// state == SELECT_WINDOW (=25). All other state calls are no-ops for now.
// On HWND change, if a slot is already allocated but missing a device
// (HWND came after CreateGameWindow), we lazily create the device.
BOOL WINAPI SelectState(u32 state, void* value)
{
    if (state == RENDERER_MODULE_STATE_SELECT_WINDOW && value != nullptr) {
        // Store HWND in the active slot. If no slot is active yet, store in
        // slot 0 so a future CreateGameWindow call can pick it up.
        DX11WindowSlot* s = ActiveSlot();
        if (!s) s = &g_Windows[0];
        s->HWND = value;
        g_DX11.Window = value;

        // If the slot is allocated but has no device yet, create one now.
        if (s->IsAllocated && s->Device == nullptr) {
            DXGI_SWAP_CHAIN_DESC scd = {};
            scd.BufferCount = 1;
            scd.BufferDesc.Width = s->Width ? s->Width : 800;
            scd.BufferDesc.Height = s->Height ? s->Height : 600;
            scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            scd.BufferDesc.RefreshRate.Numerator = 60;
            scd.BufferDesc.RefreshRate.Denominator = 1;
            scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            scd.OutputWindow = (HWND)s->HWND;
            scd.SampleDesc.Count = 1;
            scd.SampleDesc.Quality = 0;
            scd.Windowed = TRUE;
            scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
            HRESULT hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                &featureLevel, 1, D3D11_SDK_VERSION,
                &scd, &s->SwapChain, &s->Device, nullptr, &s->Context);
            if (SUCCEEDED(hr)) {
                ID3D11Texture2D* backBuffer = nullptr;
                hr = s->SwapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
                if (SUCCEEDED(hr)) {
                    s->Device->CreateRenderTargetView(backBuffer, nullptr, &s->RenderTargetView);
                    backBuffer->Release();
                    s->Context->OMSetRenderTargets(1, &s->RenderTargetView, nullptr);

                    D3D11_VIEWPORT vp = {};
                    vp.Width = (float)(s->Width ? s->Width : 800);
                    vp.Height = (float)(s->Height ? s->Height : 600);
                    vp.MinDepth = 0.0f;
                    vp.MaxDepth = 1.0f;
                    s->Context->RSSetViewports(1, &vp);
                }
            }
        }
        SyncInterfaceFromActive();
        return TRUE;
    }
    // Unknown state — accept silently. Returning TRUE keeps mcity happy
    // even though we don't act on the state.
    return TRUE;
}
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
// Phase 5: Init marks the module as loaded. Real D3D11 work happens in
// CreateGameWindow + SelectState(SELECT_WINDOW, hwnd). Returns the device
// count on subsequent calls (mcity may call Init multiple times).
BOOL WINAPI Init(void)
{
    g_DX11.IsInitialized = 1;
    // Count currently allocated windows (slot-based).
    u32 count = 0;
    for (u32 i = 0; i < MAX_WINDOWS; i++) {
        if (g_Windows[i].IsAllocated) count++;
    }
    if (count == 0) count = 1;  // claim at least 1 device exists
    g_DX11.DeviceCount = count;
    return count;
}
// Phase 5: Return acceleration-available sentinel value. mcity calls this
// after Init to check whether the renderer is usable. Returning 0 is treated
// as "no acceleration" and triggers the "Motor City has detected a problem
// with the hardware configuration" error dialog. The upstream DX8 returns
// RENDERER_MODULE_DX8_ACCELERATION_AVAILABLE (100) when a real D3D8 device
// is ready. We mirror that value here even though our device is DX11 —
// the value is just a sentinel meaning "yes, hardware accel is available".
#define RENDERER_MODULE_DX8_ACCELERATION_AVAILABLE 100
BOOL WINAPI Is(void) { return (BOOL)RENDERER_MODULE_DX8_ACCELERATION_AVAILABLE; }
BOOL WINAPI LockGameWindow(void) { return TRUE; }
BOOL WINAPI ToggleGameWindow(BOOL state) { return TRUE; }
BOOL WINAPI RestoreGameWindow(void) { return TRUE; }
BOOL WINAPI SelectDevice(u32 index) { return TRUE; }
// Phase 5: indx is 1-based window index (returned from CreateGameWindow).
// Switches the active slot so subsequent draws target the new window's
// device + swap chain.
BOOL WINAPI SelectGameWindow(u32 indx)
{
    if (indx == 0 || indx > MAX_WINDOWS) return FALSE;
    u32 slot = indx - 1;
    if (!g_Windows[slot].IsAllocated) return FALSE;
    g_DX11.ActiveSlot = slot;
    SyncInterfaceFromActive();
    return TRUE;
}
BOOL WINAPI SelectTexture(u32 index) { return TRUE; }
BOOL WINAPI SelectVideoMode(const u32 mode, const u32 pending, const u32 depth) { return TRUE; }
// Phase 5: Present the back buffer (swap chain Present) for the active slot.
BOOL WINAPI SyncGameWindow(const u32 type)
{
    DX11WindowSlot* s = ActiveSlot();
    if (!s || !s->SwapChain || !s->Device) return FALSE;
    // type=0 -> standard Present, type=1 -> vsync, type=2 -> no-wait
    u32 sync_interval = 1;  // vsync
    u32 flags = 0;
    if (type == 2) { sync_interval = 0; flags = DXGI_PRESENT_DO_NOT_SEQUENCE; }
    HRESULT hr = s->SwapChain->Present(sync_interval, flags);
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