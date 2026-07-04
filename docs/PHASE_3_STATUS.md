# Phase 3 Status — DX11 frame output

**Goal**: real D3D11 device + swap chain + render target view in `CreateGameWindow`.

**Status**: ✅ Complete + MSVC build verified (2026-07-04)

## What this phase delivered

Four entry points now have real DX11 implementations:

### `CreateGameWindow(HWND)` — D3D11 device + swap chain
- `D3D11CreateDeviceAndSwapChain()` with HWND-targeted swap chain
- Window size from `GetClientRect` (default 800x600 fallback)
- `DXGI_FORMAT_R8G8B8A8_UNORM` back buffer
- `DXGI_SWAP_EFFECT_DISCARD`
- Gets back buffer, creates render target view, OM-binds it
- Sets initial viewport
- Returns TRUE on success, FALSE with HRESULT logging on failure
- On failure: tears down all D3D11 resources cleanly

### `DestroyGameWindow()` — release
- Releases RTV, SwapChain, Context, Device in order
- Resets `IsInitialized` flag

### `ClearGameWindow()` — clear to dark blue
- Calls `ClearRenderTargetView` with `(0.05, 0.10, 0.20, 1.0)`
- Color will be overridable via state setters in Phase 6

### `SyncGameWindow()` — present
- Calls `SwapChain->Present(1, 0)` for vsync'd frame submission

## MinGW + MSVC portability work

| Issue | Fix |
|---|---|
| `DestroyGameWindow` not declared | Forward declaration |
| `snprintf` missing | Added `<cstdio>` |
| `__uuidof(ID3D11Texture2D)` MSVC-only | Inline `extern "C" const GUID` definition |
| `IID_ID3D11Texture2D` link errors | Same inline definition |
| **`Main.cxx` included `Native.Basic.hxx` which set `WIN32_LEAN_AND_MEAN`** | Replaced with `DirectX11.hxx` (this was the actual blocker for MSVC build) |
| `__ptr64` undefined in SDK 10.0.26100 | Pre-defined `__ptr64` and `POINTER_64` in `DirectX11.hxx` |

## Build verification

```
$ file build/mingw32-release-dx11/dx11.dll
PE32 executable for MS Windows 4.00 (DLL), Intel i386, 16 sections

$ file build/msvc-Release-x86/dx11/dx11.dll
PE32 executable for MS Windows 6.00 (DLL), Intel i386, 5 sections

$ objdump -p dx11.dll | grep "DLL Name"
DLL Name: d3d11.dll
DLL Name: USER32.dll
DLL Name: KERNEL32.dll
```

**Both MinGW and MSVC produce working DX11 DLLs that import d3d11.dll and have the same 47 export ordinals.**

| Build | Toolchain | Size | Notes |
|---|---|---|---|
| MinGW32 | i686-w64-mingw32 GCC 16.1.0 | 92 KB | Default with `scripts/build_dx11.sh` |
| MSVC 19.44.35228 | Visual Studio 2022 Build Tools | 114 KB | Default with `scripts/build_msvc_dx11.bat` |

## Build scripts

- `scripts/build_dx11.sh` — MinGW32 build
- `scripts/build_msvc_dx11.bat` — MSVC 2022 Build Tools build

Both produce a working `dx11.dll` from the same source.

## What's NOT in this phase (Phase 4+)

- Real draw entry points (DrawTriangle, DrawTriangleMesh, etc.) — still stubs returning TRUE
- Vertex/index buffer upload — still stubs
- Shader compilation from HLSL → DXBC — not yet wired up
- Texture upload/conversion — Phase 5
- HLSL shader variants for stage states (modulate/decal/add) — not yet
- azmco.ini selection — Phase 6

## Phase 4 starting point: HLSL shader added

`Source/R.DirectX.11.0.A/Shaders/PassThrough.hlsl` provides the initial vertex
shader (pass-through) and pixel shader (texture * color) needed for Phase 4
draw path implementation. The shader compiles to DXBC via `D3DCompile()`
which is included in the Windows SDK and MinGW's `d3dcompiler.h`.

## Status

| Deliverable | Status |
|---|---|
| Real `CreateGameWindow` with DX11 device + swap chain | ✅ |
| Real `DestroyGameWindow` | ✅ |
| Real `ClearGameWindow` | ✅ |
| Real `SyncGameWindow` | ✅ |
| MinGW portability fixes | ✅ |
| MSVC portability fixes (incl. `__ptr64`, `Main.cxx` cleanup) | ✅ |
| 47 exports (matches upstream's def) | ✅ |
| **Built on BOTH MinGW and MSVC** | ✅ 92 KB + 114 KB |
| **Pushed to fork `feat/r.directx.11.0.a` branch** | ✅ |
| Pass-through HLSL shader | ✅ (Phase 4 starting point) |

## Phase 4 next steps

1. **Compile HLSL to DXBC at module init** using `D3DCompile()` from d3dcompiler.dll
2. **Create ID3D11VertexShader + ID3D11PixelShader** from compiled blobs
3. **Create vertex/index buffer pool** (one persistent `ID3D11Buffer` with `Map/MapMode_WRITE_DISCARD`)
4. **Create input layout** matching the HLSL `VSInput` struct
5. **Wire `RenderPacket` to actual draw calls**:
   - Map vertex buffer, copy RTLVX/RTLVX2 data, unmap
   - Set primitive topology
   - Bind shaders, cbuffers, render target
   - Issue DrawIndexed/Draw call
6. **Convert state setters** (D3DRS_* equivalents) to DX11 state objects (blend, raster, depth-stencil)
7. **Convert texture stage state** to shader permutations + texture/sampler bindings

This is a multi-step effort that will land as a sequence of commits. The end result is a DX11 backend that can actually render MCO content end-to-end.

## Phase 3 PR

PR is on `Dadud/AZMCO` @ `feat/r.directx.11.0.a` branch. URL:
https://github.com/Dadud/AZMCO/pull/new/feat/r.directx.11.0.a

The branch is now 6 commits ahead of upstream/main:
1. Phase 2 skeleton (.dev marker)
2. Phase 2 DX11 skeleton (Module.cxx, DirectX11.hxx, def files, etc.)
3. Phase 3 real D3D11 device + swap chain
4. Main.cxx fix (remove Native.Basic.hxx)
5. SDK README removal (real MS DX8 SDK in place)
6. Pass-through HLSL shader (Phase 4 starting point)