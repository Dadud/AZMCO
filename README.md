# AZMCO Development Workspace
# AZMCO Development Workspace

This is the **dev workspace** for `americusmaximus/AZMCO` — a clean fork-local checkout used to add modern render-engine backends (DX11 / Vulkan / OpenGL), 64-bit support, modern input, and Linux/Mac ports.

**Current direction (2026-07-05):** building a D3D8→DX11 shim DLL
(`dxwrapper`-based, deployed as `d3d8.dll` next to `mcity.exe`) so
Motor City Online runs on modern Windows / NVIDIA hardware with
hardware acceleration. The previous 3RASH `dx8z.dll` approach
proved architecturally infeasible — see
[`docs/PIVOT_DXWRAPPER_2026-07-05.md`](docs/PIVOT_DXWRAPPER_2026-07-05.md)
and [`docs/PHASE5_RESULTS_2026-07-05.md`](docs/PHASE5_RESULTS_2026-07-05.md).

## Layout
```
~/projects/azmco-dev/
├── upstream/        ← bare clone of americusmaximus/AZMCO, branch dev/modernization
│   ├── Source/      ← Renderer backends live here
│   ├── Tests/       ← Image / content verification harness
│   ├── SDK/         ← DX7 / DX8 SDK headers/libs
│   ├── Compatibility/ ← Watcom + MSVC6 legacy build targets
│   └── .git/        ← upstream tracked here
├── mco-re/          ← bare clone of Dadud/Motor-City-Online-RE, branch dev/integration
│   ├── src/         ← RE work — game/mcity/npslib/authlogin
│   ├── tools/       ← Extraction suite (BIG, FSH, BNK, MDB, VIV, ISO, RefPack)
│   ├── data/        ← Extracted game data CSVs (Cars, Parts, Brands, StockEngines)
│   ├── docs/        ← Format / gameplay / network / media docs
│   ├── server/mco_shard/  ← Local shard server (Python)
│   ├── client/preservation_client/  ← Custom client
│   └── persistence/       ← sqlite shard DB
├── dxwrapper/       ← vendored dxwrapper source (D3D8→DX11 shim) — see docs/PIVOT
├── build/           ← out-of-tree build outputs (gitignored)
├── scripts/         ← build / test / install helpers
├── tools/           ← ad-hoc helper bat/ps1 scripts (not gitignored, kept locally)
└── docs/            ← session plans, results, pivot notes
```

## Repos

| Repo | Purpose | Branch |
|---|---|---|
| `americusmaximus/AZMCO` | Open-source re-implementation of MCO + launcher | `dev/modernization` (1 ahead of main) |
| `Dadud/Motor-City-Online-RE` | RE research, extraction tools, format docs, local shard server | `dev/integration` (1 ahead of main) |

## Cross-pollination roadmap

- Use `mco-re/docs/formats/` (BIG, FSH, VIV, MDB, RefPack) as ground truth for asset extraction
- Use `mco-re/data/*.csv` (Cars, Parts, Brands) for verification harness
- Use `mco-re/src/game/render.c` as a reference for AZMCO's `R.DirectX.8.0.A`
- Use `mco-re/server/mco_shard/` for local gameplay testing once a renderer backend works
- The 2-byte dx8z reset-loop patch (~/Apps/MCOHackAnalysis/) maps to AZMCO's `R.DirectX.8.0.A` source

## Building

### Modern MSVC (Windows, 64-bit target)

```cmd
:: Inside an x64 Native Tools Command Prompt for VS 2022:
cd %USERPROFILE%\projects\azmco-dev\upstream
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ^
    AutoZone.sln /p:Configuration=Release /p:Platform=x64 /m
```

Build outputs land in `upstream/Releases/` (created by the sln). The resulting `dx8z.dll` is a drop-in replacement for the one in `C:\Program Files (x86)\EA Games\Motor City Online\`.

### Legacy Watcom (Windows XP, 32-bit)

```cmd
:: Inside the Open Watcom IDE:
:: open Compatibility\Watcom\AZX\AZX.wpj
:: open Compatibility\Watcom\R.DirectX.7.0.A\d3da.wpj
```

### MinGW (cross-compile from MSYS, 32-bit, DX8 backend)

```bash
# Stub DX8 SDK is required because MinGW doesn't ship d3d8.h
cd ~/projects/azmco-dev/upstream
./scripts/build_mingw32.sh
```

## Testing

Tests use the image comparison harness in `Tests/`. The classic workflow:

1. Pick a reference input from `Tests/Content/`
2. Render via the backend you're testing
3. Diff against the expected output in `Tests/Images.A/`
4. Verify pixel-by-pixel match (or within tolerance for floating-point drift)

```bash
./scripts/run_tests.sh R.DirectX.8.0.A
```

## Modernization roadmap

| Module | Status | Target |
|---|---|---|
| `R.DirectX.7.0.A` | ✅ upstream | Reference, NFS lineage |
| `R.DirectX.8.0.A` | ✅ upstream | Game's original DX8 renderer (== our dx8z.dll) |
| `R.DirectX.8.0.M` | ✅ upstream | Post-update DX8 |
| `R.Software.A` | ✅ upstream | Reference / fallback |
| `R.DirectX.9.0.A` | ❌ TODO | **Quick path to DXVK** — 1:1 DX8→DX9 call translation |
| `R.DirectX.11.0.A` | ❌ TODO | Modern DirectX — own backend, no DXVK needed |
| `R.Vulkan.1.0.A` | ❌ TODO | The endgame — Linux/Mac compatible |
| `R.OpenGL.4.5.A` | ❌ TODO | Cross-platform fallback |
| Modern input (XInput) | ❌ TODO | Replace DirectInput per README goal #4 |
| 64-bit compile | ❌ TODO | Per README goal #3 |

## How to add a new renderer backend

1. Create `Source/R.<API>.<Version>.A/` mirroring `Source/R.DirectX.8.0.A/`'s file layout
2. Implement the interface declared in `Source/AZX/Renderer.Basic.hxx`
3. Hook into the renderer registry (search for `R.DirectX.8.0.A` in `Source/AZX/`)
4. Add a `R.<API>.<Version>.A` line to `Source/AutoZone.sln`
5. Add tests under `Tests/R.<API>.<Version>.A/`
6. Update `azmco.ini` to expose selection

## Sync with upstream

```bash
cd ~/projects/azmco-dev/upstream
git fetch upstream
git rebase upstream/main dev/modernization
```

Rebase rather than merge — keeps history linear and upstream-able as PRs.

## Out-of-tree build

We deliberately build out-of-tree to avoid touching the upstream's MSVC `.vcxproj` paths and to keep binary outputs out of git. `scripts/build.sh` creates `../build/<config>-<platform>/` symlinked for IDE discovery.

## Notes / known issues

- The 2-byte infinite loop at VA `0x60001C11` in `dx8z.dll` is identified upstream in source as a `Sleep + TestCooperativeLevel + Reset` cycle. Modern renderers should not need this; the loop is a DX8-era recovery hack for lost devices.
- `mcity.exe` imports: `d3d8.dll`, `ddraw.dll`, `dinput8.dll`, `dsound.dll`, `dinput.dll`. New renderers only need to handle the D3D surface; ddraw/dinput/dsound are upstream's other layers.