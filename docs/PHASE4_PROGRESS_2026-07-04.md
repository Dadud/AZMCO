# DX11 Backend Progress (2026-07-04 - Phase 4)

## What we figured out about the Init crash

The crash at `0x0000000C` (NULL pointer + 0xC offset deref) is mcity reading
a function pointer at offset 0xC of our state struct. mcity treats the state
as an interface and reads its vtable.

**Our fix**: Put a valid `ID3D11Device*` pointer at offset 0xC of `g_DX11`
(reusing it via an alias field `McityView`). When mcity reads `*(g_DX11 + 0xC)`,
it gets our device pointer. When mcity derefs `*(Device + 0xC)`, it gets
`Device->lpVtbl[3]` which is one of the standard D3D11 methods like
`AddRef`/`Release`/`QueryInterface` — all harmless.

## Architecture

```
Offset  Field                     Purpose
0x00    g_DX11.Device             Real D3D11 device
0x04    g_DX11.Context            Immediate context
0x08    g_DX11.SwapChain          Swap chain (created in CreateGameWindow when HWND available)
0x0C    g_DX11.McityView          Alias for Device - what mcity reads at offset 0xC
0x10    g_DX11.RenderTargetView   Back buffer render target view
0x14    g_DX11.IsInitialized      Init flag
0x18    g_DX11.DeviceCount        Number of devices (mirrors upstream Devices.Count)
0x1C    g_DX11.Window             Active HWND
```

## What Init now does

1. If already initialized: keep McityView in sync with Device, return DeviceCount
2. Mark IsInitialized = 1, DeviceCount = 1
3. Return 1

The real DX11 device creation happens lazily in CreateGameWindow when
mcity provides an HWND. Currently CreateGameWindow takes `u32 width,
u32 height, u32 format, u32 options` (4 args = @16) and returns a window
index — the HWND isn't passed here so we can't create a swap chain yet.

## What's still missing

1. **Real HWND capture** — mcity creates its own windows and the HWND must
   be communicated to the renderer. The current CreateGameWindow doesn't get
   an HWND. May need an additional mcity callback or a window enumeration.

2. **Draw call implementations** — all 22 draw functions (DrawLine,
   DrawTriangle, DrawSprite, etc.) are still stubs returning TRUE.

3. **State setup** — need a real RendererModuleState-like struct matching
   the upstream DX8 layout so mcity can find all the function pointers it
   expects (Lambdas, etc.).

## Files

- `upstream/Source/R.DirectX.11.0.A/Module.cxx` — DX11 backend impl
- `upstream/Source/R.DirectX.11.0.A/Main.cxx` — DllMain entry point
- `build/mingw32-release-dx11/dx11.dll` — 50 KB MinGW build
- `scripts/build_dx11.sh` — passes .def as direct input to gcc

## Commits

- `feat/r.directx.11.0.a` (8 commits ahead of fork)
- Latest: `58d8b7e` "Phase 4: McityView alias for Device + Init/CreateGameWindow polish"

## Recovery

Original EA dx8z.dll preserved at:
`~/Backups/Motor City Online backup 20260630-222817/dx8z.preserved.original.dll`

To restore:
```
copy /Y "C:\Users\Dadud\Backups\Motor City Online backup 20260630-222817\dx8z.preserved.original.dll" "C:\Program Files (x86)\EA Games\Motor City Online\dx8z.dll"
```