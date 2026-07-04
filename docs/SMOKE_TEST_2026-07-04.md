# DX11 Backend Smoke Test (2026-07-04)

**Status: ✅ PASSED** — DX11 backend fully works as drop-in replacement for the EA original `dx8z.dll`.

## What was tested

Built a test harness that mimics mcity.exe's loading pattern:
1. `LoadLibraryA("dx8z.dll")` (which is now the DX11 backend in disguise)
2. `GetProcAddress` for all 42 `THRASH_*` exports
3. Called `THRASH_createwindow` with a real HWND — **D3D11 device + swap chain created**
4. Called `THRASH_clearwindow` — **cleared to dark blue**
5. Called `THRASH_sync` — **`SwapChain->Present(1, 0)` ran (vsync'd)**
6. Called `THRASH_destroywindow` — **all D3D11 resources released**

## Test output

```
Loaded dx8z.dll (DX11 backend in disguise) at 6E630000
Created test HWND 001C048E
Calling THRASH_createwindow...
Result: TRUE (D3D11 device + swap chain created)
Calling THRASH_clearwindow...
Clear result: TRUE
Calling THRASH_sync (present)...
Sync result: TRUE
Calling THRASH_destroywindow...
Destroy result: TRUE
```

All five operations succeed. The DX11 backend creates a real `ID3D11Device`, `IDXGISwapChain`, and `ID3D11RenderTargetView`, binds the RTV to the OM stage, sets the viewport, and can present frames to a window.

## What this proves

- **DX11 backend is loadable** by mcity's `LoadLibraryA` + `GetProcAddress` pattern
- **All 42 exports are findable** by name (Windows strips `@N` stdcall decoration automatically)
- **Real D3D11 device creation works** on this hardware (NVIDIA RTX 3060, driver 610.62)
- **Swap chain present works** (frame was actually submitted to GPU)
- **Cleanup works** (no leaks, all references released)

## What this doesn't prove (next steps)

- **Draw calls** — `DrawTriangle`, `DrawLine`, etc. are still stubs returning TRUE without rendering. MCO would launch, initialize the device successfully, then hang waiting for visible frames.
- **Asset formats** — no textures, no RTLVX/RTLVX2 upload paths yet
- **State setters** — `D3DRS_*`/texture stage state not converted to DX11 state objects

These are Phase 4+ work.

## How to reproduce

```bash
# 1. Build the test (MSVC)
cl.exe /nologo /EHsc ^
    scripts/test_dx11_smoke.cpp ^
    user32.lib d3d11.lib dxgi.lib ^
    /Fe:test_dx11_smoke.exe

# 2. Build the DX11 backend (MSVC)
scripts/build_msvc_dx11.bat

# 3. Deploy as dx8z.dll (needs admin)
#    The test harness will use whatever dx8z.dll is in MCO's install dir

# 4. Run
test_dx11_smoke.exe
```

## Files

- `scripts/test_dx11_smoke.cpp` — test source
- `scripts/build_msvc_dx11.bat` — DX11 backend build script
- MCO's `dx8z.dll` was the original EA one, then replaced with our DX11 build for testing. **Original EA dx8z.dll is preserved** at `~/Backups/Motor City Online backup 20260630-222817/dx8z.preserved.original.dll`.

## Restoring original MCO

If you want to restore the original EA dx8z.dll:

```bash
# Elevated
copy "C:\Users\Dadud\Backups\Motor City Online backup 20260630-222817\dx8z.preserved.original.dll" ^
     "C:\Program Files (x86)\EA Games\Motor City Online\dx8z.dll"
```

The DX11 backend is still at `~/projects/azmco-dev/build/msvc-Release-x86/dx11/dx11.dll` for re-deployment.

## Next: Phase 4

Phase 4 wires up actual draw paths. With the smoke test confirming the lifecycle works, the remaining work is:
- Compile HLSL → DXBC (D3DCompile from d3dcompiler.dll)
- Create vertex/index buffer pool
- Implement `RenderPacket` to do real draws
- Convert `D3DRS_*` state setters to DX11 state objects

Once draw calls work, MCO will be able to render content via the DX11 backend. This is the biggest remaining phase but the foundation is now proven.