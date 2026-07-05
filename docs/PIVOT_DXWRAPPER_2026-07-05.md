# AZMCO DX11 Wrapper — Project Direction (2026-07-05)

## TL;DR

This fork is now focused on building and shipping a **D3D8→DX11 shim DLL**
(deployed as `d3d8.dll` next to `mcity.exe`) that lets Motor City Online
run on modern Windows / NVIDIA hardware with hardware acceleration. The
previous direction — replacing `dx8z.dll` with a custom 3RASH-style DX11
backend — is archived.

## The pivot

The 3RASH approach tried to replace MCO's `dx8z.dll` (the 2001 EA
DirectX 8 renderer) with a custom DirectX 11 implementation that
exposed the same 3RASH export API. Five iterations of the backend got
the calling convention, the HWND plumbing, and the function signatures
right, but it ultimately can't work because:

1. **mcity performs a hard D3D8 self-check via `Direct3DCreate8` BEFORE
   it ever calls our 3RASH `Is()` sentinel.** The trace log from
   `mco-dx11-trace.log` proves this: mcity's call sequence is
   `Init() -> SelectState(39, lambda) -> AcquireDescriptor() ->
   SelectGameWindow(2)`, and after that the "Motor City has detected
   a problem" dialog appears. We never get to `Is()` or any draw
   call.

2. **The 3RASH architecture is fundamentally tied to D3D8.** mcity
   statically imports `d3d8.dll::Direct3DCreate8`. The 3RASH `dx8z.dll`
   is a wrapper layer on top of D3D8 — it doesn't replace D3D8. There's
   no way to make mcity happy without a real D3D8 device available.

3. **The trace also showed mcity's call to `SelectGameWindow(2)`**
   expects a window to already be allocated. Upstream's DX8 module
   pre-allocates windows during init; our stub waits for mcity to call
   `CreateGameWindow` first. Even fixing that wouldn't help, because
   the dialog appears regardless of whether `SelectGameWindow` returns
   TRUE or FALSE.

**The conclusion:** any approach that replaces `dx8z.dll` needs to also
provide a D3D8 device. The simplest way to do that is to ship a
**D3D8-to-DX11 translation shim** that mcity's `d3d8.dll` import
resolves to, and which translates every D3D8 call mcity makes into
D3D11 calls. mcity thinks it's talking to a real D3D8 driver; in
reality, it's talking to a D3D11 backend.

## What "dxwrapper" means in this project

**`dxwrapper`** is the name of an existing open-source project
(https://github.com/elishacloud/dxwrapper) by Lawrence Wilkinson that
does exactly this — translates D3D8 (and D3D9, and older) calls to
D3D11 / D3D12, with proper handling of fixed-function T&L, palettized
textures, and all the other D3D8 quirks that EA's 2001-era code uses.

MCO players have historically used `dxwrapper` (often pre-built and
renamed as `d3d8.dll`) to get MCO running on modern systems. The 3.6 MB
`d3d8.dll` file in `C:\Users\Dadud\MCOTEST\` is a `dxwrapper` build.

## What this fork will do

1. **Vendor the dxwrapper source** into a `dxwrapper/` subdirectory
   at a known-good commit. (We won't fork the upstream repo, just
   keep the source in-tree for reproducibility.)
2. **Configure a build for MCO's specific D3D8 usage**:
   - 32-bit WOW64 (mcity is 32-bit)
   - Static link the CRT (avoid `libgcc_s_dw2-1.dll` and
     `api-ms-win-crt-*.dll` deps)
   - D3D11 backend (not D3D12 — mcity's D3D8 patterns map more
     naturally to D3D11)
3. **Add a `mcity.ini` (or similar)** for dxwrapper with MCO-specific
   configuration (resolution, fullscreen/windowed, vsync, etc.)
4. **Build a small helper script** that drops the resulting
   `d3d8.dll` into the MCO install dir with proper admin elevation
   and backup of the original EA d3d8.dll.
5. **Smoke test** by launching MCO, confirming the title screen /
   persona select renders correctly, and verifying no functional
   regressions vs the user's previous working dxwrapper setup
   (the 3.6 MB file in `MCOTEST/`).

## What stays

- The 3RASH `dx8z.dll` work on branch `feat/r.directx.11.0.a` is
  preserved as historical reference. None of that code is used in the
  new direction.
- `AZMCO/upstream/Source/R.DirectX.11.0.A/` (the submodule) keeps
  its Phase 5 code on the `feat/r.directx.11.0.a` branch for
  posterity.
- All the helper tools we wrote:
  - `scripts/build_dx11.sh` — useful as a reference for the dxwrapper
    MinGW32 build config
  - `tools/mco-swap-dx11.bat` — useful as a template for the
    mco-swap-d3d8.bat script that will install dxwrapper
  - `tools/mco-clean-*.bat` — useful for clean-state testing
  - `tools/mco-novaserv-setup.bat` — sets the registry to talk to
    novaserv.cc (still needed regardless of the renderer)
  - `docs/PHASE_*.md` — historical record of what we tried
  - `dxdiag.txt` — useful for GPU/driver debugging

## Why the pivot is good

The 3RASH direction would have required us to reimplement the entire
upstream DX8 module (2800 lines of which maybe 2000 are actual
rendering logic) in DX11, all while guessing at the API surface
mcity expects. Even if we got it to render, the visual fidelity
would be unproven.

dxwrapper has been built, tested, and refined for years by multiple
contributors. It handles all the D3D8 quirks that EA used — fixed
function T&L, palettized textures, surface format conversions, and
dozens of edge cases. Vendoring it (or at least configuring a build
of it) and shipping the resulting DLL alongside MCO gets us a known-
good rendering pipeline in days, not months.

## Risks

- **dxwrapper maintenance:** the upstream project is maintained by
  one person. Vendoring at a known commit is essential; we can't
  depend on future upstream releases.
- **MCO-specific fixes:** dxwrapper supports many old games; MCO
  might need its own ini profile or even a small patch for some
  particular D3D8 call we haven't audited. We'll discover this
  during smoke testing.
- **Regressions vs. existing dxwrapper build:** the user has a 3.6
  MB dxwrapper build in `MCOTEST/` that worked. Our build should
  match or exceed its behavior. If it doesn't, we need to figure
  out why the existing build works and what they did differently.

## Next steps

1. **Find and clone dxwrapper source** to `dxwrapper/` subdir
2. **Audit the D3D8 entry points** mcity uses (via `dumpbin
   /imports mcity.exe | grep d3d8`)
3. **Build dxwrapper for 32-bit MinGW** with static CRT
4. **Smoke test** with MCO installed, capture state, iterate
5. **Document the build** in `dxwrapper/README.md`