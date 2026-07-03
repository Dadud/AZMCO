# Multi-Toolchain Build Plan

The upstream AZMCO ships as MSVC vcxproj + Watcom wpj + MSVC6 dsw. We need to support multiple build methods so we're not locked into one toolchain.

## Toolchain matrix

| Toolchain | Status | Target | Use case |
|---|---|---|---|
| **MSVC 2022 Build Tools** | ⏳ Installing | Win32 + x64, modern | Primary — matches upstream `.vcxproj` |
| **MinGW32 i686** | ✅ Installed | Win32 only | Smoke builds, cross-platform portability checks |
| **Open Watcom V2.0** | ❌ Not installed | Win32 + DOS | Legacy XP builds, original toolchain lineage |
| **clang-cl** | ❌ Not installed | Win32 + x64 | Cross-vendor verification, sanitizers |
| **CMake** | ❌ Not installed | Toolchain-agnostic | Future per-renderer CMakeLists |

## Per-backend build matrix

Each backend in `Source/R.<API>.<Version>.A/` will need:

| Backend | MSVC | MinGW | Watcom | clang-cl |
|---|---|---|---|---|
| `R.DirectX.7.0.A` | ✅ upstream | ⚠️ try | ✅ upstream | ⚠️ try |
| `R.DirectX.8.0.A` | ✅ upstream | ⚠️ MSVC-isms block | ✅ upstream | ⚠️ try |
| `R.DirectX.8.0.M` | ✅ upstream | ⚠️ MSVC-isms block | ⚪ no | ⚠️ try |
| `R.Software.A` | ✅ upstream | ⚠️ try | ⚪ no | ⚠️ try |
| `R.DirectX.9.0.A` | ❌ TODO | ⚠️ TODO | ⚪ skip | ⚪ skip |
| `R.DirectX.11.0.A` | ❌ TODO | ⚪ skip | ⚪ skip | ⚠️ TODO |
| `R.Vulkan.1.0.A` | ❌ TODO | ❌ TODO | ⚪ skip | ⚠️ TODO |

Legend: ✅ works · ⚠️ needs work · ❌ TODO · ⚪ not applicable

## Known portability issues

From smoke-build attempt with MinGW32:

1. **`Source/AZX/Basic.hxx`** uses MSVC `__int64` instead of standard `long long`
   ```c
   typedef unsigned __int64 u64;  // MSVC
   typedef unsigned long long u64; // portable
   typedef __int64 s64;          // MSVC
   typedef long long s64;        // portable
   ```

2. **`NOMINMAX` redefinition** — MinGW already defines it via c++config.h

3. **`#pragma once`** is fine in both, but check for MSVC-specific pragmas

4. **`__declspec(dllexport)`** vs `__attribute__((visibility("default")))` — linker exports need different syntax

5. **Watcom `.def` files** for exports — different from MSVC linker syntax

## Build script architecture

```
scripts/
├── build_msvc.bat           ← MSBuild entry point
├── build_mingw32.sh         ← gcc + DX8 SDK from upstream
├── build_watcom.bat         ← Open Watcom wcl + wlink
├── build_clang.bat          ← clang-cl frontend, MSVC backend
├── build_cmake.bat          ← future: toolchain-agnostic
└── toolchain/
    ├── detect.sh            ← picks best available toolchain
    ├── msvc.env             ← exports MSVC vars
    ├── mingw.env            ← exports MinGW vars
    ├── watcom.env           ← exports WATCOM vars
    └── clang.env            ← exports clang-cl vars
```

Each `build_*.bat/.sh` calls `toolchain/detect.sh` first, then sources the right `.env`, then invokes the actual compiler.

## First concrete build test (when MSVC lands)

```bash
cd ~/projects/azmco-dev
./scripts/build_msvc.bat Release Win32
```

Expected output:
- `build/Release-Win32/R.DirectX.8.0.A/dx8z.dll`
- `build/Release-Win32/AZX/AZX.dll` (or whatever AZX builds to)
- `build/Release-Win32/Launcher/Launcher.exe`

Drop `dx8z.dll` into `C:\Program Files (x86)\EA Games\Motor City Online\` (backup the original first), launch via NPS, see if MCO still works.

## Modernization roadmap — updated

Per `Source/AZX/Renderer.Basic.hxx` interface, each new backend:

1. **Mirror file layout** of `R.DirectX.8.0.A/`
2. **Implement renderer interface** declared in `Renderer.Basic.hxx`
3. **Hook into renderer registry** (search for `R.DirectX.8.0.A` string in `Source/AZX/`)
4. **Add to `AutoZone.sln`**
5. **Tests in `Tests/R.<API>.<Version>.A/`**
6. **azmco.ini** entry for runtime selection

## Build verification matrix

After each backend is added:

```
Renderer     | MSVC | MinGW | Watcom | clang-cl | x64 | Win32 | Notes
--------------|------|-------|--------|----------|------|-------|--------
DX7 upstream  |  ✓   |   ?   |   ✓    |    ?     |  ?   |   ✓   | Reference
DX8 A upstream|  ✓   |   ✗   |   ✓    |    ?     |  ?   |   ✓   | 2-byte patch site
DX8 M upstream|  ✓   |   ✗   |   -    |    ?     |  ?   |   ✓   |
Software      |  ✓   |   ?   |   -    |    ?     |  ?   |   ✓   |
DX9 (TODO)    |  ✓   |   -   |   -    |    ?     |  ✓   |   ✓   | Quick path
DX11 (TODO)   |  ✓   |   -   |   -    |    ?     |  ✓   |   ✓   |
Vulkan (TODO) |  ✓   |   ✗   |   -    |    ?     |  ✓   |   ✓   | The endgame
```

## Status legend

- ✓ Works on this toolchain
- ? Untested (needs build attempt)
- ✗ Known broken (MSVC-isms in source)
- - Not applicable / not yet started
- ⏳ Pending install

## Next steps

1. ⏳ MSVC 2022 Build Tools install (background)
2. After install: detect, smoke build, validate `dx8z.dll` replaces original
3. Patch `Source/AZX/Basic.hxx` for MinGW portability (upstreamable as PR)
4. Install Open Watcom for legacy build verification
5. Investigate clang-cl path for cross-vendor builds