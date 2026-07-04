#!/usr/bin/env bash
# Build AZMCO DX11 backend (Phase 3) with MinGW32
# Output: ../build/mingw32-release-dx11/dx11.dll
#
# IMPORTANT: This build uses MinGW gcc for BOTH compile AND link, passing
# the .def file directly to the linker. This is the only way to get the
# canonical _THRASH_xxx@N export names (MSVC link.exe strips the @N from
# def LHS names when the function is __stdcall).
#
# Def file MUST start with "LIBRARY <name>" (not bare "LIBRARY").

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(cd "$SCRIPT_DIR/.." && pwd)"
UPSTREAM="$WORKSPACE/upstream"
OUT="$WORKSPACE/build/mingw32-release-dx11"
TOOLCHAIN="${MINGW32:-/c/Users/Dadud/tools/mingw32/mingw32/bin}"
SHIM="$SCRIPT_DIR/include/msvc_compat.hxx"

echo "AZMCO DX11 backend build (Phase 3)"
echo "  upstream: $UPSTREAM"
echo "  output:   $OUT"
echo "  toolchain:$TOOLCHAIN"

if [[ ! -d "$TOOLCHAIN" ]]; then
    echo "ERROR: MinGW32 toolchain not found at $TOOLCHAIN"
    exit 1
fi

cd "$UPSTREAM/Source/R.DirectX.11.0.A"

# === Verify d3d11.h is available ===
D3D11_HEADER=$(find /c/Users/Dadud/tools/mingw32 -name "d3d11.h" 2>/dev/null | head -1)
if [[ -z "$D3D11_HEADER" ]]; then
    echo "ERROR: d3d11.h not found"
    exit 1
fi
echo "  d3d11:   $D3D11_HEADER"

GCC="$TOOLCHAIN/i686-w64-mingw32-gcc.exe"
DLLTOOL="$TOOLCHAIN/dlltool.exe"
to_win() { cygpath -w "$1"; }

INCLUDES=(
    "-I$(to_win "$SCRIPT_DIR/include")"
    "-I../AZX"
    "-I../../SDK/DX80/Include"
    "-I$(to_win "$(dirname "$D3D11_HEADER")/..")"
)
SHIM_PATH="$(to_win "$SHIM")"

mkdir -p "$OUT"

# === Compile sources ===
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

# === Generate the import lib via dlltool ===
# Def file MUST start with "LIBRARY <name>" (not just "LIBRARY")
DEF_FIXED="$OUT/Renderer.Module.fixed.def"
cat > "$DEF_FIXED" << 'DEFLIB'
LIBRARY dx11
EXPORTS
    _THRASH_about@0 = AcquireDescriptor                     @1
    _THRASH_clearwindow@0 = ClearGameWindow                 @2
    _THRASH_clip@16 = ClipGameWindow                        @3
    _THRASH_createwindow@16 = CreateGameWindow              @4
    _THRASH_destroywindow@4 = DestroyGameWindow             @5
    _THRASH_drawline@8 = DrawLine                           @6
    _THRASH_drawlinemesh@12 = DrawLineMesh                  @7
    _THRASH_drawlinestrip@12 = DrawLineStrips               @8
    _THRASH_drawlinestrip@8 = DrawLineStrip                 @9
    _THRASH_drawpoint@4 = DrawPoint                         @10
    _THRASH_drawpointmesh@12 = DrawPointMesh                @11
    _THRASH_drawpointstrip@8 = DrawPointStrip               @12
    _THRASH_drawquad@16 = DrawQuad                          @13
    _THRASH_drawquadmesh@12 = DrawQuadMesh                  @14
    _THRASH_drawsprite@8 = DrawSprite                       @15
    _THRASH_drawspritemesh@12 = DrawSpriteMesh              @16
    _THRASH_drawtri@12 = DrawTriangle                       @17
    _THRASH_drawtrifan@12 = DrawTriangleFans                @18
    _THRASH_drawtrifan@8 = DrawTriangleFan                  @19
    _THRASH_drawtrimesh@12 = DrawTriangleMesh               @20
    _THRASH_drawtristrip@12 = DrawTriangleStrips            @21
    _THRASH_drawtristrip@8 = DrawTriangleStrip              @22
    _THRASH_flushwindow@0 = FlushGameWindow                 @23
    _THRASH_getstate@4 = AcquireState                       @24
    _THRASH_getwindowtexture@4 = AcquireGameWindowTexture   @25
    _THRASH_idle@0 = Idle                                   @26
    _THRASH_init@0 = Init                                   @27
    _THRASH_is@0 = Is                                       @28
    _THRASH_lockwindow@0 = LockGameWindow                   @29
    _THRASH_pageflip@0 = ToggleGameWindow                   @30
    _THRASH_readrect@20 = ReadRectangle                     @31
    _THRASH_readrect@24 = ReadRectangles                    @32
    _THRASH_restore@0 = RestoreGameWindow                   @33
    _THRASH_selectdisplay@4 = SelectDevice                  @34
    _THRASH_setstate@8 = SelectState                        @35
    _THRASH_settexture@4 = SelectTexture                    @36
    _THRASH_setvideomode@12 = SelectVideoMode               @37
    _THRASH_sync@4 = SyncGameWindow                         @38
    _THRASH_talloc@20 = AllocateTexture                     @39
    _THRASH_tfree@4 = ReleaseTexture                        @40
    _THRASH_treset@0 = ResetTextures                        @41
    _THRASH_tupdate@12 = UpdateTexture                      @42
    _THRASH_tupdaterect@36 = UpdateTextureRectangle         @43
    _THRASH_unlockwindow@4 = UnlockGameWindow               @44
    _THRASH_window@4 = SelectGameWindow                     @45
    _THRASH_writerect@20 = WriteRectangle                   @46
    _THRASH_writerect@24 = WriteRectangles                  @47
DEFLIB

IMPORT_LIB="$OUT/dx11.a"
IMPORT_LIB_WIN="$(to_win "$IMPORT_LIB")"
DEF_FIXED_WIN="$(to_win "$DEF_FIXED")"
echo
echo "Generating import lib via dlltool..."
"$DLLTOOL" --dllname dx11.dll --def "$DEF_FIXED_WIN" --output-lib "$IMPORT_LIB_WIN" 2>&1 | head -5

# === Link ===
# CRITICAL: Pass the .def file directly to gcc as a linker input.
# This makes the linker (ld) create the export table with the exact
# names from the def file, including the @N stdcall decoration.
# Without this, the linker auto-exports C++ symbol names.
echo
echo "Linking dx11.dll..."
OBJECTS_WIN=()
for obj in "${OBJECTS[@]}"; do
    OBJECTS_WIN+=("$(to_win "$obj")")
done
DLL_OUT_WIN="$(to_win "$OUT")/dx11.dll"
DEF_FIXED_WIN="$(to_win "$DEF_FIXED")"

"$GCC" -shared -o "$DLL_OUT_WIN" \
    "${OBJECTS_WIN[@]}" \
    "$DEF_FIXED_WIN" \
    "$IMPORT_LIB_WIN" \
    -ld3d11 -ldxgi -ld3dcompiler -lkernel32 2>&1 | head -30

RC=$?
echo
if [[ $RC -eq 0 && -f "$OUT/dx11.dll" ]]; then
    echo "✓ dx11.dll built: $OUT/dx11.dll"
    ls -la "$OUT/dx11.dll"
else
    echo "✗ Link failed (rc=$RC)"
    exit 1
fi
