# dxwrapper — D3D8→DX11 shim for Motor City Online

This directory will hold the vendored source of
[dxwrapper](https://github.com/elishacloud/dxwrapper) (by Lawrence
Wilkinson) configured to translate the D3D8 calls MCO's `mcity.exe`
makes into D3D11 calls. The resulting `d3d8.dll` is deployed next
to `mcity.exe` in the MCO install dir.

## Status (2026-07-05)

- [ ] Find and clone dxwrapper at a known-good commit
- [ ] Audit MCO's D3D8 import surface (which functions are called
      and how heavily)
- [ ] Configure dxwrapper for 32-bit MinGW static-CRT build
- [ ] Build a test `d3d8.dll` (~3-4 MB expected)
- [ ] Smoke test: drop into MCO install dir, launch mcity, capture
      rendering + log output
- [ ] Document MCO-specific ini profile and any patches needed
- [ ] Compare to existing `MCOTEST/d3d8.dll` (3.6 MB) — if our build
      renders worse, diff the two

See [`../docs/PIVOT_DXWRAPPER_2026-07-05.md`](../docs/PIVOT_DXWRAPPER_2026-07-05.md)
for the rationale behind this direction.

## Why dxwrapper and not rolling our own

- 3RASH `dx8z.dll` approach (Phase 5) failed: mcity's D3D8 self-check
  fires before any 3RASH function is called, and the 3RASH layer is
  fundamentally a D3D8 wrapper (not an alternative to D3D8).
- dxwrapper has been built and refined for years specifically for
  old D3D8/D3D9 games on modern hardware.
- Vendoring at a known commit is a one-time cost; maintaining our
  own D3D8→DX11 translation layer is a multi-year project.
- The 3.6 MB `d3d8.dll` in `C:\Users\Dadud\MCOTEST\` is a working
  dxwrapper build that the user has used before — we should match
  or exceed its rendering quality.

## MCO-specific config (sketch — to be filled in after smoke test)

```ini
[Renderer]
; D3D8 -> DX11 wrapper config for MCO (mcity.exe)
; Values below are starting points; tune after smoke testing.

Width=1920
Height=1080
Fullscreen=false
VSync=true
; MCO uses 16-bit palettized textures heavily; the wrapper should
; convert them to 32-bit BGRA on the fly.
PaletteConversion=true
; MCO uses fixed-function T&L for the 3D scene. The wrapper needs
; to either emulate FFP in DX11 or use the default vertex shader
; path.
FixedFunctionTandL=true
```
