#!/usr/bin/env bash
# Detect available C++ toolchains
# Outputs JSON-ish summary, sets BEST_TC env var

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

detected=()

# MSVC
if command -v cl.exe >/dev/null 2>&1; then
    detected+=("msvc:$(cl.exe 2>&1 | head -1)")
elif [ -d "/c/Program Files/Microsoft Visual Studio" ]; then
    # Try to find vcvars
    for vs in "2022/Community" "2022/BuildTools" "2019/Community" "2019/BuildTools"; do
        vcvars="/c/Program Files/Microsoft Visual Studio/$vs/VC/Auxiliary/Build/vcvars64.bat"
        if [ -f "$vcvars" ]; then
            detected+=("msvc:$vcvars")
        fi
    done
fi

# MinGW32 (multiple known locations)
for gcc in \
    i686-w64-mingw32-gcc.exe \
    /c/Users/Dadud/tools/mingw32/mingw32/bin/i686-w64-mingw32-gcc.exe \
    /mingw32/bin/i686-w64-mingw32-gcc.exe
do
    if command -v "$gcc" >/dev/null 2>&1 || [ -x "$gcc" ]; then
        full=$(command -v "$gcc" 2>/dev/null || echo "$gcc")
        detected+=("mingw:$full")
        break
    fi
done

# clang-cl
if command -v clang-cl.exe >/dev/null 2>&1; then
    detected+=("clang:$(clang-cl.exe --version | head -1)")
fi

# Watcom
if [ -n "$WATCOM" ] && [ -d "$WATCOM" ]; then
    detected+=("watcom:$WATCOM")
fi

# CMake
if command -v cmake.exe >/dev/null 2>&1; then
    detected+=("cmake:$(cmake.exe --version | head -1)")
fi

echo "Detected toolchains:"
for tc in "${detected[@]}"; do
    echo "  - $tc"
done

# Pick best: MSVC > clang-cl > MinGW > Watcom > cmake-only (no compiler)
for tc in "${detected[@]}"; do
    case "$tc" in
        msvc:*)   echo "BEST_TC=msvc"   ; exit 0 ;;
        clang:*)  echo "BEST_TC=clang"  ; exit 0 ;;
        mingw:*)  echo "BEST_TC=mingw"  ; exit 0 ;;
        watcom:*) echo "BEST_TC=watcom" ; exit 0 ;;
    esac
done

# If only cmake is present, no compiler is usable
echo "BEST_TC=none"
exit 1