#!/usr/bin/env bash
# Run AZMCO image/content tests against a built renderer backend
# Usage: ./scripts/run_tests.sh [R.DirectX.8.0.A]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(cd "$SCRIPT_DIR/.." && pwd)"
BACKEND="${1:-R.DirectX.8.0.A}"
BUILD_DIR="$WORKSPACE/build"

echo "AZMCO test runner"
echo "  backend: $BACKEND"
echo "  build:   $BUILD_DIR"

# Find any built DLL matching the backend
DLL=$(find "$BUILD_DIR" -name "dx*.dll" 2>/dev/null | head -1)
if [[ -z "$DLL" ]]; then
    echo "ERROR: No built renderer DLL found under $BUILD_DIR"
    echo "Run scripts/build_msvc.bat or scripts/build_mingw32.sh first."
    exit 1
fi

echo "  dll:     $DLL"

# TODO: actually invoke the test harness
# For now this is a smoke check
if [[ -d "$WORKSPACE/upstream/Tests/Content" ]]; then
    CT=$(find "$WORKSPACE/upstream/Tests/Content" -type f | wc -l)
    echo "  test inputs: $CT files in Tests/Content"
fi
if [[ -d "$WORKSPACE/upstream/Tests/Images.A" ]]; then
    IM=$(find "$WORKSPACE/upstream/Tests/Images.A" -type f | wc -l)
    echo "  reference images: $IM files in Tests/Images.A"
fi

echo
echo "Test runner stub OK. Full harness needs TestRunner exe from upstream build."