# AZMCO Development Workspace

This is the **dev workspace** for `americusmaximus/AZMCO` — a clean fork-local checkout used to add modern render-engine backends (DX11 / Vulkan / OpenGL), 64-bit support, modern input, and Linux/Mac ports.

## Layout

```
~/projects/azmco-dev/
├── upstream/        ← bare clone of americusmaximus/AZMCO, branch dev/modernization
│   ├── Source/      ← Renderer backends live here
│   ├── Tests/       ← Image / content verification harness
│   ├── SDK/         ← DX7 / DX8 SDK headers/libs
│   ├── Compatibility/ ← Watcom + MSVC6 legacy build targets
│   └── .git/        ← upstream tracked here
├── build/           ← out-of-tree build outputs (gitignored)
├── patches/         ← binary patches (e.g. dx8z reset-loop patch)
├── scripts/         ← build / test / install helpers
└── NOTES.md         ← session-local development notes (NOT committed)
```

## Branches

| Branch | Purpose |
|---|---|
| `main` (from upstream) | Frozen mirror, never commit here directly |
| `dev/modernization` | Active development branch |
| Future: `feat/r.directx.11.0.a`, `feat/r.vulkan.1.0.a` | Per-backend feature branches |

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