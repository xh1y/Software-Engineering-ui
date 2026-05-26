#!/bin/bash
set -uo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
COV_DIR="$PROJECT_DIR/cmake-build-coverage"

echo "=== [1/4] Configure coverage build ==="
rm -rf "$COV_DIR" && mkdir "$COV_DIR"
cd "$COV_DIR"
cmake "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage -g -O0" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
    -DCMAKE_SHARED_LINKER_FLAGS="--coverage" \
    > cmake_config.log 2>&1
echo "  OK"

echo "=== [2/4] Build test targets ==="
TESTS="test_datamodel test_configmanager test_databasemanager \
test_inferenceengine test_graphwidget test_batchprocessor \
test_mainwindow test_batchprocessdialog test_performance \
test_reliability test_appconstants test_stylemanager \
test_settingsdialog test_resulttreewidget test_inferenceworker"
cmake --build . -j"$(nproc)" -- $TESTS > build.log 2>&1
echo "  OK ($(wc -l < build.log) build lines)"

echo "=== [3/4] Run all tests (offscreen) ==="
cd tests
TOTAL=0; PASS=0; FAIL=0
for bin in $TESTS; do
    [ -x "$bin" ] || continue
    ((TOTAL++))
    if QT_QPA_PLATFORM=offscreen timeout 120 "./$bin" > /dev/null 2>&1; then
        ((PASS++))
    else
        ((FAIL++))
        echo "  FAIL: $bin"
    fi
done
echo "  $PASS / $TOTAL passed, $FAIL failed"
echo ""

echo "=== [4/4] gcovr coverage report ==="
cd "$COV_DIR"
gcovr -r "$PROJECT_DIR" \
      --filter "$PROJECT_DIR/src/" \
      --exclude ".*_autogen.*" \
      --exclude ".*main\.cpp" \
      --print-summary \
      --exclude-unreachable-branches \
      2>&1

echo ""
echo "--- Per-file details ---"
gcovr -r "$PROJECT_DIR" \
      --filter "$PROJECT_DIR/src/" \
      --exclude ".*_autogen.*" \
      --exclude ".*main\.cpp" \
      --exclude-unreachable-branches \
      2>&1 | sort -t: -k2 -rn

echo ""
echo "=== Done ==="
