// msvc_compat.hxx — MinGW portability shim for AZMCO
//
// This header makes MinGW-w64 look enough like MSVC that AZMCO's
// existing `_MSC_VER` guards work as intended. Force-included via
// gcc -include flag before any AZMCO source compiles.
//
// What's shimmed:
//   - _MSC_VER: set to 1300 (VC7) so newer-MSVC branches activate
//     (the `#if _MSC_VER <= 1200` guard falls through, picking the
//     `long long` portable typedefs instead of `__int64`)
//   - _MSC_EXTENSIONS: defined so MinGW's _mingw.h doesn't auto-define
//     NONAMELESSUNION (which would force d3d8types.h to use named
//     union/struct members and break AZMCO's m._11 access pattern)
//   - NONAMELESSUNION: undef'd as a belt-and-braces measure
//   - NOMINMAX: undef'd so AZMCO's raw max()/min() calls work
//   - max/min: replaced with templates that allow mixed-type args
//     (u32 + s32) like MSVC's windows.h `max(a,b)` macro did
//
// What's NOT shimmed:
//   - `__declspec(dllexport)` / `__declspec(dllimport)` — these need
//     proper MinGW attribute syntax (`__attribute__((visibility))`)
//     which is a deeper change to AZX/RendererModule.Export.hxx
//   - `#pragma optimize("", off)` — MSVC-specific, no-op on MinGW
//     (only activated by `#ifdef NOOPTIMIZATION` so harmless)
//
// Once AZMCO has built-in MinGW portability (or once MSVC becomes
// the primary build), this shim becomes unnecessary.

#pragma once

#ifndef _MSC_VER
    #define _MSC_VER 1300
#endif

// CRITICAL: defining _MSC_EXTENSIONS prevents MinGW's _mingw.h from
// auto-defining NONAMELESSUNION, which would break m._11 access in
// D3DMATRIX (and other SDK structs with anonymous unions).
#ifndef _MSC_EXTENSIONS
    #define _MSC_EXTENSIONS
#endif

#ifndef __declspec
    #define __declspec(x)
#endif

// Suppress "unused parameter" warnings in headers
#ifdef __GNUC__
    #define UNUSED(x) x __attribute__((unused))
#else
    #define UNUSED(x) x
#endif

// Belt-and-braces: in case anything else defined NONAMELESSUNION before us
#ifdef NONAMELESSUNION
    #undef NONAMELESSUNION
#endif
#ifdef NONAMELESSSTRUCT
    #undef NONAMELESSSTRUCT
#endif

// MinGW's c++config.h defines NOMINMAX, which prevents windows.h's
// `max`/`min` macros from being usable. AZMCO source uses raw max()/min()
// with mixed integer types (e.g. max(u32, s32) at Images.cxx:1887) which
// worked under MSVC's macro but fails under std::max template deduction
// (which requires both args to be the same type).
//
// Solution: undef NOMINMAX before windows.h pulls in, then define our own
// max/min templates that allow mixed types via std::common_type.
#ifdef __GNUC__
    #ifdef NOMINMAX
        #undef NOMINMAX
    #endif
    #include <algorithm>
    #include <type_traits>
    template<typename A, typename B>
    static inline auto max(A a, B b) -> typename std::common_type<A, B>::type {
        return a > b ? a : b;
    }
    template<typename A, typename B>
    static inline auto min(A a, B b) -> typename std::common_type<A, B>::type {
        return a < b ? a : b;
    }
#endif