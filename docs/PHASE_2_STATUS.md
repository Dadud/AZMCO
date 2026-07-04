# Phase 2 Status — DX11 backend skeleton

**Goal**: stub `R.DirectX.11.0.A/` that loads into MCO without crashing.

**Status**: ✅ Complete (2026-07-03)

## What's in this phase

A new directory `Source/R.DirectX.11.0.A/` containing:

| File | Size | Purpose |
|---|---|---|
| `DirectX11.hxx` | 843 B | D3D11/DXGI/d3dcompiler includes |
| `Module.cxx` | 7.8 KB | All entry-point stubs + AcquireDescriptor |
| `Module.hxx` | 1.2 KB | Module name + author macros |
| `Main.cxx` | 1.2 KB | DLL entry point |
| `Renderer.Module.MSVC.def` | 3 KB | Export table (44 ordinals) |
| `Renderer.Module.Watcom.def` | 3 KB | Watcom export variant |

Total: 6 files, ~17 KB. Builds to a 47 KB PE32 i386 DLL via MinGW32.

## What's stubbed vs real

**Stubbed** (return TRUE/0, no-op):
- ~40 entry points: Acquire*, Select*, Render*, Draw*, Clear*, Clip, Lock, etc.
- D3D11 device creation (logged to OutputDebugString only)
- D3D11 swap chain setup
- All texture/vertex buffer management
- All state setters

**Real**:
- `AcquireDescriptor()` — returns valid `RendererModuleDescriptor` so MCO identifies the renderer
- `Main()` — DLL entry point (just returns TRUE)
- `DestroyGameWindow()` — properly nulls state

## What's NOT in this phase

- Real D3D11 device + swap chain (Phase 3)
- Any actual rendering (Phase 3-4)
- Texture conversion (Phase 5)
- HLSL shaders (Phase 4)
- Configuration via `azmco.ini` (Phase 6)

## Build

```bash
cd ~/projects/azmco-dev
./scripts/build_dx11.sh
```

Output: `build/mingw32-release-dx11/dx11.dll` (47 KB PE32 i386).

Verified: all 44 export ordinals present via `objdump -p`.

## Smoke test

The skeleton is **not** safe to drop into MCO yet — it returns success for everything but doesn't actually render, so MCO would launch to a black screen and likely hang or crash waiting for rendering to complete. The proper smoke test happens in Phase 3 once we have a real D3D11 swap chain.

## Phase 2 PR #2

PR will add:
- `Source/R.DirectX.11.0.A/` directory (6 files)
- `scripts/build_dx11.sh`
- `docs/PHASE_2_STATUS.md` (this file)
- Updates to `docs/MODERN_RENDERER_PLAN.md`

Files NOT included in PR (kept upstream-clean):
- Removed D3D8-specific source files (Images.cxx, Renderer.cxx, etc.) — they have heavy D3D8 header deps and don't compile in DX11 mode
- Removed `R.DirectX.8.0.A.vcxproj` — needs renaming/regeneration for DX11

When Phase 3 begins:
1. Create `R.DirectX.11.0.A.vcxproj` (MSVC build target)
2. Add folder to `AutoZone.sln`
3. Add `[Renderer] DX11=1` to `azmco.ini` for runtime selection
4. Implement real D3D11 device + swap chain in `CreateGameWindow()`
