#!/usr/bin/env bash
# Build AZMCO DX8 backend with MinGW 32-bit (MSYS / git-bash)
# Output: ../build/mingw32-release/R.DirectX.8.0.A/dx8z.dll

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(cd "$SCRIPT_DIR/.." && pwd)"
UPSTREAM="$WORKSPACE/upstream"
OUT="$WORKSPACE/build/mingw32-release"
TOOLCHAIN="${MINGW32:-/c/Users/Dadud/tools/mingw32/mingw32/bin}"
COMPAT_SHIM="$SCRIPT_DIR/../include/msvc_compat.hxx"

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

# Compile each .cxx in R.DirectX.8.0.A/ into a .o
# Includes:
#   - $SCRIPT_DIR/../include — for msvc_compat.hxx
#   - ../AZX                 — sibling to R.DirectX.8.0.A, has shared headers
#   - ../../SDK/DX80/Include — for d3d8.h (or falls back to MinGW's bundled one)

cd "$UPSTREAM/Source/R.DirectX.8.0.A"

GCC="$TOOLCHAIN/i686-w64-mingw32-gcc.exe"
# MSYS auto-translates bash args but not always reliably for gcc's -include.
# Use cygpath to convert to Windows-native paths so gcc finds them.
to_win() { cygpath -w "$1"; }

INCLUDES=(
    "-I$(to_win "$SCRIPT_DIR/include")"
    "-I../AZX"
    "-I../../SDK/DX80/Include"
)
SHIM_PATH="$(to_win "$SCRIPT_DIR/include/msvc_compat.hxx")"

# Force-include msvc_compat.hxx so _MSC_VER is defined before Basic.hxx is parsed.
# This makes the existing `_MSC_VER <= 1200` guard skip the `__int64` branch
# and use `long long` instead. Shim lives in scripts/include/.

SOURCES=(Module.cxx Renderer.cxx Images.cxx RendererValues.cxx Settings.cxx FPU.cxx)
OBJECTS=()
FAILED=0
for src in "${SOURCES[@]}"; do
    obj="$OUT/${src%.cxx}.o"
    obj_win="$(to_win "$obj")"
    if "$GCC" -c -std=c++14 -fpermissive \
            -DDIRECT3D_VERSION=0x800 \
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
    echo "This is expected for MinGW without MSVC SDK — review logs to see"
    echo "what blocks compilation and patch AZX/ headers upstream."
    exit 1
fi

# Link to dx8z.dll
echo
echo "Linking dx8z.dll..."
# Toolchain layout: $TOOLCHAIN/bin/ has both cross-tools (i686-w64-mingw32-gcc)
# and target tools (dlltool, ld, as, ar) in the same dir.
DLLTOOL="$TOOLCHAIN/dlltool.exe"
LD="$TOOLCHAIN/i686-w64-mingw32-gcc.exe"

if [[ ! -x "$DLLTOOL" ]]; then
    echo "ERROR: dlltool not found at $DLLTOOL"
    exit 1
fi

# Normalize .def file: ensure CRLF line endings and standalone keywords.
# The upstream's Renderer.Module.MSVC.def has "LIBRARY EXPORTS" on adjacent
# lines (no CR/LF between) which MinGW's dlltool rejects. Add CRs.
DEF_FIXED="$OUT/Renderer.Module.fixed.def"
DEF_FIXED_WIN="$(to_win "$DEF_FIXED")"

# Read def file, strip any CR, then convert all LF to CRLF, ensure
# LIBRARY and EXPORTS are on separate lines, write to fixed path
SRC_DEF="Renderer.Module.MSVC.def"
# Get raw content, normalize line endings
sed 's/\r$//' "$SRC_DEF" | awk 'BEGIN{ORS="\r\n"} {
    gsub(/LIBRARY EXPORTS/, "LIBRARY\r\nEXPORTS");
    print
}' > "$DEF_FIXED"

if [[ ! -s "$DEF_FIXED" ]]; then
    echo "ERROR: failed to write normalized def file"
    exit 1
fi
echo "Normalized def file -> $DEF_FIXED ($(wc -c < "$DEF_FIXED") bytes)"

# Generate import lib from the .def file (Watcom/MSVC format)
IMPORT_LIB="$OUT/dx8z.a"
IMPORT_LIB_WIN="$(to_win "$IMPORT_LIB")"
DEF_FIXED_WIN="$(to_win "$DEF_FIXED")"
"$DLLTOOL" --dllname dx8z.dll --def "$DEF_FIXED_WIN" --output-lib "$IMPORT_LIB_WIN" 2>&1 | head -5

# Link
# Need: all .o files (Win paths), import lib, and we don't strictly need -ld3dx8
# since it's a separate helper not used in this renderer. -ld3d8 is required.
OBJECTS_WIN=()
for obj in "${OBJECTS[@]}"; do
    OBJECTS_WIN+=("$(to_win "$obj")")
done
DLL_OUT_WIN="$(to_win "$OUT")/dx8z.dll"

"$LD" -shared -o "$DLL_OUT_WIN" "${OBJECTS_WIN[@]}" "$IMPORT_LIB_WIN" \
    -ld3d8 -lgdi32 -luser32 -lkernel32 -ladvapi32 2>&1 | head -30
RC=$?

echo
if [[ $RC -eq 0 && -f "$OUT/dx8z.dll" ]]; then
    echo "✓ dx8z.dll built: $OUT/dx8z.dll"
    ls -la "$OUT/dx8z.dll"
else
    echo "✗ Link failed (rc=$RC)"
    exit 1
fi