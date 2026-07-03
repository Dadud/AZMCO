# MSVC Build Tools install — status & workaround

## Current state (2026-07-03)

- ❌ MSVC 2022 Build Tools install via winget **failed 2x** with exit code 1602
- The first attempt left orphan `msiexec.exe` (PID 16460) and `TrustedInstaller.exe` (PID 15400) running for 16+ minutes with 0 CPU
- These cannot be killed without TrustedInstaller privileges
- No VS install directory exists
- The `winget install` retry just printed help text because of the stuck MSI

## Why it failed

winget uses the Microsoft VS bootstrapper under the hood. Two MSI executors
running concurrently → 1602 (another install in progress). The first
attempt's msiexec is hung, blocking subsequent attempts.

## Workaround in place

The MinGW32 i686 toolchain at `~/tools/mingw32/mingw32/bin/` is **working**
and produces a usable `dx8z.dll` from the upstream AZMCO source. See
`build_mingw32.sh` and `scripts/include/msvc_compat.hxx`.

This is a viable alternative path while MSVC installation is blocked.
Differences vs MSVC build:

- Different calling convention handling (cdecl vs stdcall) — minor risk
- Struct layouts may differ (alignment, padding) — potential ABI mismatch
- No MSVC compiler intrinsics like `__assume`, `__forceinline`
- Some MSVC-isms in upstream code may compile differently

## To retry MSVC install

Once the orphan msiexec clears (may need a reboot, or wait for Windows
to time out the MSI session), try one of:

```bash
# Option A: Direct bootstrapper download + run
curl -L -o vs_bootstrapper.exe https://aka.ms/vs/17/release/vs_BuildTools.exe
./vs_bootstrapper.exe --quiet --wait --add Microsoft.VisualStudio.Workload.VCTools

# Option B: Wait 30 min, then retry winget
sleep 1800 && winget install --id Microsoft.VisualStudio.2022.BuildTools \
    --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" \
    --accept-package-agreements --accept-source-agreements --silent

# Option C: Try a different install method
# Download from https://visualstudio.microsoft.com/downloads/?q=build+tools
# Run the installer interactively
```

## What MSVC would unlock

- Canonical build path matching upstream `.vcxproj` files
- ABI-identical DLLs that drop in cleanly to MCO
- Ability to build `R.DirectX.8.0.M`, `R.DirectX.9.0.A`, `R.DirectX.11.0.A`
  backends (some have MSVC-only intrinsics)
- Cross-vendor build verification (MinGW vs MSVC vs clang-cl)

## What MinGW gets us NOW

- ✅ All 6 `R.DirectX.8.0.A` source files compile
- ✅ Links to a real PE32 i386 DLL
- ✅ Multi-toolchain build infrastructure (`scripts/toolchain/detect.sh`)
- ✅ Can iterate on the source while MSVC install is sorted out
- ⚠️ May have ABI differences — needs MCO testing to validate

## Recommendation

Proceed with MinGW build for testing. Keep MSVC install as a TODO.
A working MinGW dx8z.dll is better than a non-existent MSVC one.