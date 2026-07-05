# Phase 5 Results — DX11 backend attempt (2026-07-04 to 2026-07-05)

## TL;DR

The DX11 backend approach (replacing `dx8z.dll` with a custom DirectX 11
implementation that mimics the 3RASH API) **does not work** because mcity
performs a hard D3D8 self-check before it even calls our `Is()` sentinel.
The 3RASH architecture from 2001 is fundamentally tied to D3D8 — there
is no way to present a DX11 backend as a 3RASH renderer without also
providing a D3D8 device via a shim DLL.

**Recommendation: pivot to `dxwrapper` (D3D8→DX11/D3D12 shim) deployed as
`d3d8.dll` next to mcity.** This is the path the user has already been
on (the 3.6 MB `d3d8.dll` in `MCOTEST/` was a dxwrapper build). Building
from upstream source gives a known-good D3D8→DX11 translation layer
without re-implementing 2800 lines of upstream DX8 module code.

## What we proved worked (committed)

### Step 0: Calling convention fix (`ecf373d`)
- Added `WINAPI` (`__stdcall`) to all 47 exports in `Module.cxx`
- Completed the `.def` file's `@N` stack-size decorations
- Verified `dumpbin /exports` of our `dx11.dll` matches EA's `dx8z.dll`
  byte-for-byte on all 47 ordinals / decorated names
- mcity's `GetProcAddress("_THRASH_xxx@N")` lookups resolve correctly

### Step 1: HWND plumbing (`ae9a48f`)
- g_Windows[4]: per-slot DX11 device + swap chain + RTV
- g_DX11.ActiveSlot: tracks which slot is current
- ActiveSlot() / SyncInterfaceFromActive() helpers
- CreateGameWindow returns u32 (window index) per upstream convention
- DestroyGameWindow releases slot's resources
- SelectState intercepts state 25 (HWND) and creates device lazily
- SelectGameWindow switches active slot
- ClearGameWindow / SyncGameWindow use active slot

### Step 1.1: Is() sentinel (`2f2d5e2`)
- Returns `RENDERER_MODULE_DX8_ACCELERATION_AVAILABLE` (100)
- Did NOT fix the "Motor City has detected a problem" dialog

### Step 2: File-based trace logging (`3032285`)
- trace_log() helper writes to `C:\Users\Dadud\tools\mco-dx11-trace.log`
- trace_clear() on Init() so each test run starts fresh
- Instrumented: AcquireDescriptor, Init, CreateGameWindow, DestroyGameWindow,
  ClearGameWindow, SyncGameWindow, SelectState, SelectDevice, SelectGameWindow,
  Is, DrawRectangle, DrawSprite

### Step 2.1: Static linking (`9ef2f8c`, master branch)
- `-static-libgcc -static-libstdc++` eliminates `libgcc_s_dw2-1.dll` dep
- mco-swap-dx11.bat copies 8 UCRT api-ms-win-crt forwarder DLLs into install dir
  on BACKUP, removes on RESTORE
- Without this, mcity got "A required DLL was not found. (dx8z.dll)"

### Step 2.2: Lazy SelectGameWindow (`7925fb5`)
- Trace revealed mcity flow: `Init -> SelectState(39, lambda) -> AcquireDescriptor
  -> SelectGameWindow(2)` and then the dialog
- State 39 = RENDERER_MODULE_STATE_SELECT_SELECT_STATE_LAMBDA (callback
  registration, not HWND)
- SelectGameWindow now lazily allocates the requested slot if not yet allocated
- Did NOT fix the dialog (mcity still gets to the same decision point)

## The actual problem (post-mortem)

mcity's actual call sequence is short:

```
Init() -> SelectState(39, lambda) -> AcquireDescriptor() -> SelectGameWindow(2)
```

After `SelectGameWindow(2)` returns, mcity does **not** call our `Is()` or
`CreateGameWindow` — instead it shows the "Motor City has detected a
problem with the hardware configuration" dialog. The trace ends right
at that point.

**Why the dialog:** mcity performs a hard D3D8 self-check via
`Direct3DCreate8` (which it imports statically from `d3d8.dll`) BEFORE
it ever asks our backend whether acceleration is available. When that
self-check fails — or when the resulting device can't render in the way
mcity expects — mcity shows the dialog as a fatal fallback.

The 3RASH `Is()=100` sentinel is checked AFTER the D3D8 self-check
passes. We never get to that point because mcity aborts first.

**Additional evidence:** the dialog was unclickable on the second test,
suggesting mcity is in a busy state (probably a child thread waiting on
an event the main thread never signals) which prevents the message pump
from servicing the dialog. This is consistent with the EA 2001
architecture being single-threaded with synchronous initialization.

## Backup chain (in `C:\Users\Dadud\Backups\Motor City Online backup 20260630-222817\`)

- `dx8z.preserved.original.dll` — EA original (135168 bytes, SHA 327938d5...)
- `dx8z.pre-dx11-test.dll` — user's prior version
- `dx8z.live-20260705-002704.dll` — our first Is()=0 test build
- `dx8z.live-20260705-003059.dll` — our second Is()=0 test build
- `dx8z.live-20260705-012710.dll` — our trace build (95KB, libgcc dep)
- `dx8z.live-20260705-013942.dll` — our static-linked build (107KB, UCRT deps)
- `dx8z.live-20260705-014254.dll` — our static-linked + lazy SelectGameWindow build

## Commits on `feat/r.directx.11.0.a` (in upstream submodule)

- `ecf373d` — Phase 5 step 0: WINAPI/stdcall + .def complete
- `ae9a48f` — Phase 5 step 1: HWND plumbing
- `2f2d5e2` — Phase 5 step 1.1: Is() = 100
- `3032285` — Phase 5 step 2: trace logging
- `7925fb5` — Phase 5 step 2.2: lazy SelectGameWindow

Plus `9ef2f8c` on master (parent repo): build script changes for static linking.

## What I'd do next: dxwrapper route

1. Find dxwrapper source on GitHub (elishacloud/dxwrapper or similar fork)
2. Audit it for the D3D8 calls mcity makes (Direct3DCreate8, IDirect3D8::CreateDevice,
   IDirect3DDevice8::Reset, etc.)
3. Build for 32-bit WOW64
4. Drop in as `d3d8.dll` next to mcity
5. Keep our `dx8z.dll` as-is OR revert to EA's `dx8z.dll` (doesn't matter —
   mcity will call into dxwrapper for D3D8 anyway, and our dx8z never gets
   meaningful calls since mcity aborts before using it)

This would actually get MCO running on modern hardware with hardware acceleration,
which is the real goal.