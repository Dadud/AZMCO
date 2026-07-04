# Modern Render Engine Development Plan

Goal: produce a working **modern DirectX 11 renderer** for AZMCO that replaces the DirectX 8 fixed-function pipeline with something that runs on Win10/11+ with modern GPUs. Vulkan port comes after DX11 is feature-complete.

## Decisions (locked in 2026-07-03)

1. **DX11 first.** Vulkan port comes later (Phase 7).
2. **Phased PRs.** Each phase ships something working and upstreamable.
3. **Skip MCODeadlockFix.dll.** The DX11 backend will have windowed mode + reset-loop fix built in. Existing MCODeadlockFix becomes redundant — we'll write a migration note but won't carry it forward.
4. **64-bit from day 1.** All new code targets x64 where possible. Win32 stays only where required (MCO's `mcity.exe` is 32-bit and we're a DLL injected into it, so the *renderer DLL itself* must be 32-bit WOW64, but dev tools and test harnesses are 64-bit).

**Important constraint on #4**: MCO's `mcity.exe` is a 32-bit process. The renderer DLL we ship must be **32-bit** to load into it. So "64-bit from day 1" means:
- All dev tooling (compilers, IDEs, Vulkan SDK install) installs 64-bit
- Test harnesses, asset extractors, modern tools build 64-bit
- The renderer DLL itself stays 32-bit (WOW64)
- 64-bit renderer becomes possible if/when MCO gets a 64-bit re-release

## Why DX11 first, not Vulkan or DX12

| API | Pros | Cons |
|---|---|---|
| **Vulkan** | Linux/Mac portable, lowest CPU overhead, future-proof | Big surface area (1000s of lines for boilerplate), need SPIR-V shaders, harder learning curve |
| **DirectX 11** | Familiar D3D9-style API, smaller than DX12, Windows-only | Less portable, slightly higher overhead |
| **DirectX 12** | Lowest latency, best multi-GPU | Massive boilerplate, command lists + descriptors + heaps, Windows 10+ only |

DX11 is the pragmatic choice because:
- DX8 → DX11 has a known migration path (DX10/11 retain the D3D pipeline mental model)
- Shader Model 4/5 lets us write modern HLSL instead of fixed-function
- Most rendering concepts (buffers, textures, shaders, state objects) port 1:1 to Vulkan later
- Easier to validate against DXVK's DX9 implementation as a reference

Vulkan comes after — once DX11 works, the AZMCO source becomes the canonical reference, and a Vulkan port is "just" a different backend of the same abstraction.

## Architecture — what's actually portable

```
Source/AZX/                       ← engine-agnostic core (DO NOT MODIFY)
├── Renderer.Basic.hxx            ← RTLVX vertex structs, pixel format enums
├── RendererModule.Basic.hxx      ← 33 KB of renderer interface contract
├── RendererModule.Export.hxx     ← __declspec(dllexport) entry points
├── RendererModule.Settings.hxx   ← shared settings names
├── RendererModule.Import.hxx     ← shared host imports (mcity.exe calls)
└── (Basic, Graphics, Mathematics, Native utility headers)

Source/R.DirectX.7.0.A/          ← DX7/DDraw renderer (346 KB, NFS lineage)
Source/R.DirectX.8.0.A/          ← existing backend (374 KB, builds clean)
Source/R.DirectX.8.0.M/          ← post-update renderer (246 KB, similar shape)
Source/R.SoftWare.A/             ← software renderer (105 KB, reference impl)

Source/Launcher/                   ← MFC launcher (88 KB) — needs Win32 build
Source/Implode/                    ← PKWARE decompression (12 KB) — pure C, easy
```

The renderer interface in `RendererModule.Basic.hxx` is **the key abstraction**. Every backend implements it. A new `R.DirectX.11.0.A/` folder just plugs into the same interface.

## Phased roadmap — 8 phases, ordered by value-to-effort

### Phase 0 — Prerequisites (1-2 days)
**Goal**: solid toolchain + working reference builds before writing any new code.

- [ ] Resolve MSVC install blocker. **Install 64-bit Build Tools + 32-bit cross-tools** so we can build both targets.
- [ ] Get **MSVC** build of `R.DirectX.8.0.A` working (32-bit DLL, since that's what mcity.exe loads)
- [ ] Verify MinGW + MSVC builds produce functionally equivalent output (drop-in compatible)
- [ ] Apply **only the reset-loop bounded-retry fix** to DX8 source (line 1482 in `R.DirectX.8.0.A/Renderer.cxx`). Skip the windowed-mode and resolution tweaks — DX11 backend will own those.
- [ ] Build the rest of the existing 32-bit components: `R.DirectX.8.0.M`, `Source/Implode`, `Source/Launcher`
- [ ] Install 64-bit **Vulkan SDK** from LunarG (for shader tooling in Phase 4+)
- [ ] Install **DirectXTex** (Microsoft, MIT-licensed) for texture format conversion in Phase 5

**Exit criteria**: `dx8z.dll` from MinGW and MSVC both run MCO successfully with reset-loop fix applied. 64-bit toolchain installed for dev use.

### Phase 1 — Renderer interface audit (2-3 days)
**Goal**: understand what every entry point in `RendererModule.Basic.hxx` does, document, identify gaps for modern APIs.

- [ ] Read all 33 KB of `RendererModule.Basic.hxx` end-to-end
- [ ] Catalog every function pointer, struct, enum the renderer must provide
- [ ] Document the **vertex pipeline** (`RTLVX` → output) and **pixel pipeline** (texture → framebuffer)
- [ ] Identify all **fixed-function state** (transforms, lights, materials, fog, alpha blend modes)
- [ ] Map each fixed-function concept to a modern equivalent:
  - `D3DTS_WORLD` / `D3DTS_VIEW` / `D3DTS_PROJECTION` → uniform buffer
  - `D3DLIGHT8` × N → uniform buffer of light structs
  - `D3DMATERIAL8` → uniform buffer
  - Texture stage state (colorop, alphaop, colorarg1/2/3, alphaarg1/2/3) → shader combination
  - Fog → vertex shader output + uniform color/density
  - Alpha test → shader branch (discard)
  - Alpha blend → blend state object
- [ ] Produce `docs/RENDERER_INTERFACE.md` with full mapping table

**Exit criteria**: clear specification of what the DX11/Vulkan backend must implement.

### Phase 2 — Skeleton DX11 backend (3-5 days)
**Goal**: stub `R.DirectX.11.0.A/` that loads, returns valid handles, doesn't crash MCO.

- [ ] Copy `R.DirectX.8.0.A/` to `R.DirectX.11.0.A/`
- [ ] Rename files: `DirectX.hxx` → `DirectX11.hxx`, `Renderer.Module.MSVC.def` stays (same exports)
- [ ] Strip D3D8-specific calls; replace with `ID3D11Device` / `ID3D11DeviceContext` interfaces
- [ ] Implement `Direct3DCreate11`-equivalent (just create the device from DXGI factory)
- [ ] Stub `CreateDevice` and `Reset` — return success without doing anything
- [ ] Stub all other entry points — return failure or no-op
- [ ] Add to `AutoZone.sln`, build with MSVC
- [ ] Update `azmco.ini [Renderer]` to select DX11
- [ ] Smoke test: launch MCO, see black screen but no crash

**Exit criteria**: MCO loads `R.DirectX.11.0.A`, device created, no crashes, just black output.

### Phase 3 — DX11 frame output (1-2 weeks)
**Goal**: render something — even just a clear color — to the window.

- [ ] Set up swap chain via DXGI factory
- [ ] Map AZMCO's `D3DPRESENT_PARAMETERS` → DX11 swap chain description
- [ ] Render-target view from back buffer
- [ ] Implement a single pass-through vertex shader that outputs `RTLVX.XYZ` directly (no projection yet)
- [ ] Implement a single pass-through pixel shader that outputs `RTLVX.Color` directly
- [ ] `Present` the swap chain each frame
- [ ] Verify: MCO shows some color (probably wrong, but renders)

**Exit criteria**: MCO shows a non-black window with SOMETHING being drawn.

### Phase 4 — DX11 fixed-function state (2-3 weeks)
**Goal**: replace AZMCO's fixed-function state setup with shader+constant-buffer updates.

- [ ] World/View/Projection transforms → `cbuffer PerFrame`
- [ ] Light setup (point, directional, spot) → `cbuffer PerFrame`
- [ ] Material → `cbuffer PerObject`
- [ ] Texture stage state → 8-16 shader permutations (modulate, decal, blend, replace, etc.)
- [ ] Fog → vertex shader output + per-pixel lerp in fragment
- [ ] Alpha blending → 8 blend state objects (src/dst alpha combinations)
- [ ] Write the HLSL shaders (a few `.hlsl` files in `Source/R.DirectX.11.0.A/Shaders/`)

This is the **hardest phase**. AZMCO sets ~50 different state combinations per draw call.

**Exit criteria**: AZMCO's game content (cars, track, etc.) shows on screen, possibly with visual glitches.

### Phase 5 — DX11 textures and palette conversion (1 week)
**Goal**: handle MCO's proprietary texture formats (P8, R5G5B5, etc.) on a modern GPU.

- [ ] Implement `R5G5B5` → `B5G5R5A1` upload path
- [ ] Implement `P8` (paletted) → upload as 32-bit RGBA via lookup table
- [ ] Implement `DXT1/3/5` → BC1/2/3 GPU formats (auto via D3DX or DirectXTex)
- [ ] Implement `YUV2` → `DXGI_FORMAT_YUY2` (DX11 native) or upload as RGBA
- [ ] Build mipmaps via `ID3D11Device::GenerateMips`
- [ ] Handle texture cache eviction (AZMCO tracks ~1000 textures max)

**Exit criteria**: textures show correctly with right colors.

### Phase 6 — DX11 polish (1-2 weeks)
**Goal**: ship-quality DX11 renderer that matches DX8 feature parity AND replaces MCODeadlockFix.dll functionality.

- [ ] Resolution override via `azmco.ini [DX11] Width/Height` — replaces what MCODeadlockFix does
- [ ] Windowed/borderless toggle via `azmco.ini [DX11] Windowed`
- [ ] VSync control via `azmco.ini [DX11] VSync`
- [ ] Hot-reload of `azmco.ini` while running
- [ ] Logging via OutputDebugString + `dx11_render.log`
- [ ] Performance counters (FPS, draw calls, texture memory)
- [ ] **Migration note**: document that MCODeadlockFix.dll is now redundant, can be removed from MCO install

**Exit criteria**: DX11 backend works as drop-in replacement for DX8 with no crashes. MCODeadlockFix.dll removable.

### Phase 7 — Vulkan backend (3-4 weeks)
**Goal**: port the working DX11 backend to Vulkan.

- [ ] Set up Vulkan SDK (already installed in Phase 0)
- [ ] Choose Vulkan loader approach (dynamically linked vs bundled)
- [ ] Create instance, physical device, logical device, queues
- [ ] Swap chain via VK_KHR_swapchain
- [ ] Port shader permutation strategy → SPIR-V cross-compilation
- [ ] Descriptor sets for per-frame/per-object uniforms
- [ ] Command buffer recording for each draw call
- [ ] Pipeline objects (one per shader permutation × blend state)
- [ ] Texture uploads via staging buffer
- [ ] Render passes + framebuffers

Vulkan's biggest cost is upfront: ~500 lines of boilerplate before drawing anything. After that, the per-draw-call logic is similar to DX11.

**Exit criteria**: Vulkan backend works on Windows. Linux/Mac builds are a bonus after that.

### Phase 8 — Cross-platform polish (2-3 weeks)
**Goal**: run on Linux/Mac, modern input, packaging.

- [ ] Linux build via clang + Vulkan SDK
- [ ] X11/Wayland window integration (use SDL2 or GLFW)
- [ ] Mac build via MoltenVK (translates Vulkan → Metal)
- [ ] Replace DirectInput with `SDL_GameController` (modern gamepads/XInput)
- [ ] Package as AppImage / DMG / Steam dep

**Exit criteria**: MCO runs natively on Linux + Mac.

## Effort estimate

| Phase | Duration | Cumulative | Risk |
|---|---|---|---|
| 0 — Prereqs | 1-2 days | 1-2 days | Low |
| 1 — Interface audit | 2-3 days | 4-5 days | Low |
| 2 — Skeleton DX11 | 3-5 days | 1-2 weeks | Low |
| 3 — Frame output | 1-2 weeks | 3-4 weeks | Medium |
| 4 — Fixed-function | 2-3 weeks | 5-7 weeks | High |
| 5 — Textures | 1 week | 6-8 weeks | Low |
| 6 — DX11 polish | 1-2 weeks | 8-10 weeks | Low |
| 7 — Vulkan | 3-4 weeks | 11-14 weeks | High |
| 8 — Cross-platform | 2-3 weeks | 13-17 weeks | Medium |

**Total: 3-4 months to Vulkan-on-Windows**, ~4-5 months to Linux/Mac native.

## Upstreamable PRs

| Phase | PR | What |
|---|---|---|
| 0 | #1 | Reset loop bounded retry (32-bit DLL, drop-in) |
| 2 | #2 | DX11 backend skeleton (compiles but renders nothing) |
| 6 | #3 | DX11 backend feature-complete with built-in windowed/reset fixes |
| 7 | #4 | Vulkan backend |

All of these are MIT-licensed contributions americusmaximus has explicitly invited.

Note: The windowed-mode and resolution-override settings are **rolled into PR #3** (DX11 backend), not separate PRs. The DX11 backend owns those features instead of patching DX8.

## What we're NOT doing in this plan

- **Replacing MCO's netcode** — separate project, not in scope
- **Asset extraction improvements** — that's `mco-re` (your other repo)
- **Modernization of `Source/Launcher`** — it works, leave it
- **Server emulator** — that's `rustymotors/server`, different project
- **MCODeadlockFix.dll maintenance** — superseded by DX11 backend in Phase 6

## Success metrics

- ✅ MCO launches and renders correctly with DX11 backend
- ✅ Resolution/windowed/Vsync configurable from `azmco.ini` (no registry hacks)
- ✅ No more Alt+Enter deadlock
- ✅ No reset-loop hang on focus loss
- ✅ MCODeadlockFix.dll removable
- ✅ Linux native port compiles and runs
- ✅ ~4 PRs upstreamed to americusmaximus/AZMCO

## Sources of risk

- **AZMCO hasn't been validated end-to-end** yet — we built but haven't run. Phase 0 must include a successful MCO launch with the new build before committing to more code.
- **The D3D8 API surface is huge** — even DX8 had ~150 functions. Translating every one is real work.
- **MCO's asset formats** are RE'd by your mco-re project, not AZMCO. Texture format conversions need cross-project knowledge.
- **No regression test infrastructure** — we can launch and look, but no automated checks. Build a snapshot test harness in Phase 1.

## First 5 actions to start today

1. Wait for orphan msiexec to clear, retry MSVC install (with **64-bit** Build Tools)
2. Apply **only the reset-loop fix** in DX8 source (skip windowed + resolution — those belong in DX11)
3. Rebuild with MinGW AND MSVC, drop into MCO, launch, observe
4. Install 64-bit Vulkan SDK + DirectXTex for downstream phases
5. Read `Source/AZX/RendererModule.Basic.hxx` end-to-end

Each takes < 1 hour. Doing them today gets Phase 0 + start of Phase 1 done.