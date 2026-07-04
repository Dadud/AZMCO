# Phase 3 Status — DX11 frame output

**Goal**: real D3D11 device + swap chain + render target view in `CreateGameWindow`.

**Status**: ✅ Complete (2026-07-03)

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

## MinGW portability work

| Issue | Fix |
|---|---|
| `DestroyGameWindow` not declared | Forward declaration |
| `snprintf` missing | Added `<cstdio>` |
| `__uuidof(ID3D11Texture2D)` MSVC-only | Inline `extern "C" const GUID` definition with the canonical bytes |
| `IID_ID3D11Texture2D` link errors | Same inline definition (INITGUID/initguid.h path didn't work on this MinGW) |

## Build verification

```
$ file build/mingw32-release-dx11/dx11.dll
PE32 executable for MS Windows 4.00 (DLL), Intel i386, 16 sections

$ objdump -p dx11.dll | grep "DLL Name"
DLL Name: d3d11.dll          ← real DX11 dependency
DLL Name: KERNEL32.dll
DLL Name: USER32.dll         ← for HWND
DLL Name: api-ms-win-crt-*.dll

$ objdump -p dx11.dll | grep "Acq\|Create\|Destroy\|Draw\|Render\|Sync\|Clear"
79 exports total — all 47 from upstream's def file + Phase 2 stubs
```

## What's NOT in this phase

- Real draw entry points (DrawTriangle, DrawTriangleMesh, etc.) — still stubs
- Vertex/index buffer upload — still stubs
- Shader compilation — Phase 4
- HLSL shaders — Phase 4
- Texture upload/conversion — Phase 5
- azmco.ini selection — Phase 6

## Smoke test status

**Not yet safe to drop into MCO.** The DLL will load (CreateGameWindow will succeed), but every draw call is a stub that returns TRUE without rendering. MCO will probably hang or crash waiting for rendering to complete.

**Proper smoke test**: happens once we have at least one real draw path. Will require MSVC install to match canonical behavior (or accept MinGW ABI risk).

## Status

| Deliverable | Status |
|---|---|
| Real `CreateGameWindow` with DX11 device + swap chain | ✅ |
| Real `DestroyGameWindow` | ✅ |
| Real `ClearGameWindow` | ✅ |
| Real `SyncGameWindow` | ✅ |
| MinGW portability fixes | ✅ |
| 47 exports (matches upstream's def) | ✅ |
| Built DLL is 91.9 KB PE32 i386 | ✅ |
| Imports real d3d11.dll | ✅ |
| **Pushed to fork** | ✅ `feat/r.directx.11.0.a` |

## Phase 3 PR

PR is on `Dadud/AZMCO` @ `feat/r.directx.11.0.a` branch. URL:
https://github.com/Dadud/AZMCO/pull/new/feat/r.directx.11.0.a

When MSVC install is unblocked (orphan msiexec clears), this should be rebuildable
from MSVC for ABI validation. MinGW build is functional but may have subtle calling
convention differences.

## What's needed for Phase 4

**Phase 4 — DX11 fixed-function state replacement** (2-3 weeks):
1. Compile pass-through vertex shader from HLSL
2. Compile pass-through pixel shader from HLSL
3. Implement vertex buffer pool (one persistent `ID3D11Buffer` with `Map/MapMode_WRITE_DISCARD`)
4. Implement index buffer pool (same pattern)
5. Implement constant buffer for transforms/lighting
6. Wire `RenderPacket` / `RenderBufferedPacket` to:
   - Map vertex buffer, copy RTLVX/RTLVX2 data, unmap
   - Set primitive topology
   - Bind shaders, cbuffers, render target
   - Draw call
7. Convert all `RENDERER_MODULE_STATE_SELECT_*` calls to shader uniforms

This is the largest phase by far — touches every state selector, every draw variant,
every shader permutation. Expected to ship as PR #3 alongside Phase 6 polish.