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

BOOL CreateGameWindow(HWND hwnd)
{
    OutputDebugStringA("[R.DirectX.11.0.A] CreateGameWindow (skeleton stub)\n");
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

BOOL ClearGameWindow(void) { return TRUE; }
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

} // extern "C"