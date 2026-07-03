#!/usr/bin/env bash
# Build AZMCO DX8 backend with MinGW 32-bit (MSYS / git-bash)
# Output: ../build/mingw32-release/R.DirectX.8.0.A/dx8z.dll

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(cd "$SCRIPT_DIR/.." && pwd)"
UPSTREAM="$WORKSPACE/upstream"
OUT="$WORKSPACE/build/mingw32-release"
TOOLCHAIN="${MINGW32:-/c/Users/Dadud/tools/mingw32/mingw32/bin}"

echo "AZMCO MinGW32 build"
echo "  upstream: $UPSTREAM"
echo "  output:   $OUT"
echo "  toolchain:$TOOLCHAIN"

if [[ ! -d "$TOOLCHAIN" ]]; then
    echo "ERROR: MinGW32 toolchain not found at $TOOLCHAIN"
    echo "Set MINGW32 env var to override."
    exit 1
fi

# Check for MinGW-w64 or MinGW32 + DX8 SDK stub
# The upstream's MSVC vcxproj uses $(DXSDK_DIR); we'll fake this with
# stub headers from mingw32 + DX8 SDK from archive.org

if [[ ! -d "$UPSTREAM/SDK/DX80" ]]; then
    echo "ERROR: DX8 SDK not present at $UPSTREAM/SDK/DX80"
    echo "Get it from https://archive.org/details/dx8a_sdk and extract into SDK/DX80/."
    exit 1
fi

mkdir -p "$OUT"

# TODO: actual cmake / make / direct g++ invocation
# For now this is a smoke test that the toolchain resolves
"$TOOLCHAIN/i686-w64-mingw32-gcc" --version | head -1
echo "build stub: source layout looks good, see ../upstream/AutoZone.sln for MSVC build path"