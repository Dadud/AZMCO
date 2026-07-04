#!/usr/bin/env bash
# Build AZMCO DX11 backend (Phase 2 skeleton) with MinGW32
# Output: ../build/mingw32-release/R.DirectX.11.0.A/dx11.dll

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(cd "$SCRIPT_DIR/.." && pwd)"
UPSTREAM="$WORKSPACE/upstream"
OUT="$WORKSPACE/build/mingw32-release-dx11"
TOOLCHAIN="${MINGW32:-/c/Users/Dadud/tools/mingw32/mingw32/bin}"
SHIM="$SCRIPT_DIR/include/msvc_compat.hxx"

echo "AZMCO DX11 backend build (Phase 2 skeleton)"
echo "  upstream: $UPSTREAM"
echo "  output:   $OUT"
echo "  toolchain:$TOOLCHAIN"

if [[ ! -d "$TOOLCHAIN" ]]; then
    echo "ERROR: MinGW32 toolchain not found at $TOOLCHAIN"
    exit 1
fi

cd "$UPSTREAM/Source/R.DirectX.11.0.A"

# === Verify d3d11.h is available ===
# MinGW32 ships d3d11.h at $MINGW32_PREFIX/include/d3d11.h
# (We have D3D11 headers via Windows SDK / MinGW; no separate install needed.)
D3D11_HEADER=$(find /c/Users/Dadud/tools/mingw32 -name "d3d11.h" 2>/dev/null | head -1)
if [[ -z "$D3D11_HEADER" ]]; then
    echo "ERROR: d3d11.h not found in MinGW32. Need Windows SDK or MinGW with d3d11 headers."
    exit 1
fi
echo "  d3d11:   $D3D11_HEADER"

GCC="$TOOLCHAIN/i686-w64-mingw32-gcc.exe"
to_win() { cygpath -w "$1"; }

INCLUDES=(
    "-I$(to_win "$SCRIPT_DIR/include")"
    "-I../AZX"
    "-I../../SDK/DX80/Include"
    "-I$(to_win "$(dirname "$D3D11_HEADER")/..")"
)
SHIM_PATH="$(to_win "$SHIM")"

# Ensure OUT exists before log redirect
mkdir -p "$OUT"

# Skeleton sources only — Phase 2.
# Future phases will add Renderer.cxx, Images.cxx, etc. after D3D8->D3D11 conversion.
SOURCES=(Module.cxx Main.cxx)
OBJECTS=()
FAILED=0
for src in "${SOURCES[@]}"; do
    obj="$OUT/${src%.cxx}.o"
    obj_win="$(to_win "$obj")"
    if "$GCC" -c -std=c++14 -fpermissive \
            -include "$SHIM_PATH" \
            "${INCLUDES[@]}" \
            -o "$obj_win" \
            "$src" 2>"$OUT/${src%.cxx}.log"; then
        echo "  OK    $src -> $(basename "$obj")"
        OBJECTS+=("$obj")
    else
        echo "  FAIL  $src (see $OUT/${src%.cxx}.log)"
        FAILED=$((FAILED + 1))
    fi
done

if [[ $FAILED -gt 0 ]]; then
    echo
    echo "$FAILED source files failed to compile. Logs in $OUT/*.log"
    exit 1
fi

# === Link to dx11.dll ===
echo
echo "Linking dx11.dll..."
DLLTOOL="$TOOLCHAIN/dlltool.exe"
LD="$TOOLCHAIN/i686-w64-mingw32-gcc.exe"

# Reuse upstream's def file (same export names)
DEF_FIXED="$OUT/Renderer.Module.fixed.def"
sed 's/\r$//' Renderer.Module.MSVC.def | awk 'BEGIN{ORS="\r\n"} {
    gsub(/LIBRARY EXPORTS/, "LIBRARY\r\nEXPORTS");
    print
}' > "$DEF_FIXED"

IMPORT_LIB="$OUT/dx11.a"
IMPORT_LIB_WIN="$(to_win "$IMPORT_LIB")"
DEF_FIXED_WIN="$(to_win "$DEF_FIXED")"
"$DLLTOOL" --dllname dx11.dll --def "$DEF_FIXED_WIN" --output-lib "$IMPORT_LIB_WIN" 2>&1 | head -5

OBJECTS_WIN=()
for obj in "${OBJECTS[@]}"; do
    OBJECTS_WIN+=("$(to_win "$obj")")
done
DLL_OUT_WIN="$(to_win "$OUT")/dx11.dll"

"$LD" -shared -o "$DLL_OUT_WIN" "${OBJECTS_WIN[@]}" "$IMPORT_LIB_WIN" \
    -ld3d11 -ldxgi -ld3dcompiler -lgdi32 -luser32 -lkernel32 -ladvapi32 2>&1 | head -30
RC=$?

echo
if [[ $RC -eq 0 && -f "$OUT/dx11.dll" ]]; then
    echo "✓ dx11.dll built: $OUT/dx11.dll"
    ls -la "$OUT/dx11.dll"
else
    echo "✗ Link failed (rc=$RC)"
    exit 1
fi