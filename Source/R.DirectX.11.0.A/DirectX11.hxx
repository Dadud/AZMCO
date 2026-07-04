// DirectX11.hxx — Phase 2 skeleton DX11 backend
//
// This file replaces DirectX.hxx in the upstream D3D8 backend. It pulls in the
// real DirectX 11 API headers so we can incrementally convert D3D8 calls to
// D3D11 equivalents. For the skeleton phase, only the includes are in place;
// the device/swap-chain/etc. creation is stubbed in Module.cxx.
//
// Header strategy: pull in <d3d11.h>, <dxgi.h>, <d3dcompiler.h>, and the DXGI
// 1.x swapchain format enum. We do NOT include <d3d8.h> — any D3D8 reference
// in this backend is a Phase 3-4 conversion TODO.

#pragma once

#define DIRECT3D_VERSION 0x0b00
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

// Convenience: cast helpers used in Module.cxx
#ifndef D3D11_NO_HELPERS
    #define SAFE_RELEASE(p) do { if (p) { (p)->Release(); (p) = nullptr; } } while (0)
#endif