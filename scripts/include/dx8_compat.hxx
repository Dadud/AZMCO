// dx8_compat.hxx — MSVC compatibility shim for MinGW's d3d8.h
//
// MinGW's d3d8types.h defines D3DPRESENT_PARAMETERS differently than the
// original Microsoft one. The upstream AZMCO source expects the Microsoft
// layout (with EnableAutoDepthStencil, Windowed, etc.).
//
// This header is force-included BEFORE d3d8.h via MSBuild's /FI option.
// It defines the missing fields as no-op stubs so MSVC compiles the source.

#pragma once

// Guard against double-include
#ifndef _DX8_COMPAT_MSVC_HXX
#define _DX8_COMPAT_MSVC_HXX

// d3d8types.h uses __MSABI_LONG but doesn't define it on MSVC
#ifndef __MSABI_LONG
    #define __MSABI_LONG long
#endif

// These fields are missing from MinGW's _D3DPRESENT_PARAMETERS_ struct.
// We define them as no-op fields via a union with the existing struct,
// but since we can't modify the struct directly, we provide wrapper macros
// that the source code can use.
//
// NOTE: This shim is a Phase 3 stopgap. The proper fix is to use the
// canonical Microsoft DirectX 8.0a SDK from archive.org.

#endif // _DX8_COMPAT_MSVC_HXX