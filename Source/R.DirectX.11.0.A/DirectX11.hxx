// DirectX11.hxx — Phase 2 skeleton DX11 backend

#pragma once

// WORKAROUND: Windows SDK 10.0.26100.0 has a bug where `__ptr64` isn't
// defined anywhere in the headers, and `winnt.h` line 451 fails to compile
// when WIN32_LEAN_AND_MEAN is set. basetsd.h's `POINTER_64` macro then
// refers to undefined `__ptr64`.
//
// Pre-define both POINTER_64 and __ptr64 as no-ops (they're only markers on
// Itanium/old MIPS64, not relevant to x86/x64 modern targets).
#ifndef __ptr64
    #define __ptr64
#endif

#ifndef POINTER_64
    #if defined(_WIN64) || defined(_M_X64) || defined(_M_ARM64)
        #define POINTER_64 __ptr64
    #else
        #define POINTER_64
    #endif
#endif

// Now safe to include Windows headers
#include <windows.h>

// D3D11 requires _WIN32_WINNT >= 0x0601
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0601
#endif
#include <sdkddkver.h>

// D3D11 headers
#define DIRECT3D_VERSION 0x0b00
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#ifndef D3D11_NO_HELPERS
    #define SAFE_RELEASE(p) do { if (p) { (p)->Release(); (p) = nullptr; } } while (0)
#endif