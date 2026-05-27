#!/bin/bash
#
# OptiKG 测试与覆盖率脚本
# 用法:
#   ./scripts/run_tests.sh              # 跑 debug build 所有测试
#   ./scripts/run_tests.sh coverage     # 重新构建 coverage build 并跑 gcovr
#
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DEBUG_DIR="$PROJECT_DIR/cmake-build-debug/tests"
COV_DIR="$PROJECT_DIR/cmake-build-coverage"

# ---------- 颜色 ----------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

total=0
passed=0
failed=0
skipped=0
declare -a FAILED_TESTS=()

run_one() {
    local bin="$1"
    local name="$2"
    ((total++))
    echo -n "  [$total] $name ... "
    if [ ! -x "$bin" ]; then
        echo -e "${YELLOW}SKIP${NC} (binary not found)"
        ((skipped++))
        return
    fi
    if "$bin" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        ((passed++))
    else
        local rc=$?
        echo -e "${RED}FAIL${NC} (exit $rc)"
        ((failed++))
        FAILED_TESTS+=("$name")
    fi
}

# ==================== 模式 1: 跑 debug build 的测试 ====================
run_debug_tests() {
    echo "============================================"
    echo " Running tests from cmake-build-debug/tests"
    echo "============================================"
    echo ""
    cd "$DEBUG_DIR"

    echo "--- Non-GUI tests ---"
    run_one ./test_datamodel            "test_datamodel"
    run_one ./test_appconstants         "test_appconstants"
    run_one ./test_configmanager        "test_configmanager"
    run_one ./test_stylemanager         "test_stylemanager"
    run_one ./test_databasemanager      "test_databasemanager"
    run_one ./test_inferenceengine      "test_inferenceengine"
    run_one ./test_inferenceworker      "test_inferenceworker"
    run_one ./test_resulttreewidget     "test_resulttreewidget"
    echo ""

    echo "--- GUI tests (may need DISPLAY, will skip if no X11) ---"
    if [ -z "${DISPLAY:-}" ]; then
        echo "  No DISPLAY set, skipping GUI tests"
        ((skipped+=5))
    else
        run_one ./test_graphwidget          "test_graphwidget"
        run_one ./test_batchprocessor       "test_batchprocessor"
        run_one ./test_batchprocessdialog   "test_batchprocessdialog"
        run_one ./test_mainwindow           "test_mainwindow"
        run_one ./test_settingsdialog       "test_settingsdialog"
    fi
    echo ""

    # 性能与可靠性（单独跑，可能很慢）
    echo "--- Performance & Reliability (timeout 30s each) ---"
    if [ -x ./test_performance ]; then
        ((total++))
        echo -n "  [$total] test_performance ... "
        if timeout 30 ./test_performance > /dev/null 2>&1; then
            echo -e "${GREEN}PASS${NC}"
            ((passed++))
        else
            echo -e "${RED}FAIL${NC} (or timed out)"
            ((failed++))
            FAILED_TESTS+=("test_performance")
        fi
    fi
    if [ -x ./test_reliability ]; then
        ((total++))
        echo -n "  [$total] test_reliability ... "
        if timeout 30 ./test_reliability > /dev/null 2>&1; then
            echo -e "${GREEN}PASS${NC}"
            ((passed++))
        else
            echo -e "${RED}FAIL${NC} (or timed out)"
            ((failed++))
            FAILED_TESTS+=("test_reliability")
        fi
    fi
    echo ""
}

# ==================== 模式 2: coverage build + gcovr ====================
run_coverage() {
    echo "============================================"
    echo " Coverage build + test + gcovr"
    echo "============================================"

    echo ""
    echo "[1/4] Configuring coverage build..."
    rm -rf "$COV_DIR"
    mkdir -p "$COV_DIR"
    cd "$COV_DIR"

    cmake "$PROJECT_DIR" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="--coverage -g -O0" \
        -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
        -DCMAKE_SHARED_LINKER_FLAGS="--coverage" \
        > cmake_config.log 2>&1
    echo "  -> configure done"

    echo "[2/4] Building test targets..."
    cmake --build . -j"$(nproc)" -- \
        test_datamodel test_configmanager test_databasemanager \
        test_inferenceengine test_graphwidget test_batchprocessor \
        test_mainwindow test_batchprocessdialog test_performance \
        test_reliability test_appconstants test_stylemanager \
        test_settingsdialog test_resulttreewidget test_inferenceworker \
        test_logger test_extractionpanel test_historypanel \
        > build.log 2>&1
    echo "  -> build done"

    echo "[3/4] Running tests..."
    cd tests
    local cov_total=0 cov_pass=0 cov_fail=0
    for bin in test_datamodel test_configmanager test_databasemanager \
               test_inferenceengine test_inferenceworker test_resulttreewidget \
               test_appconstants test_stylemanager \
               test_graphwidget test_batchprocessor test_batchprocessdialog \
               test_mainwindow test_settingsdialog \
               test_logger test_extractionpanel test_historypanel \
               test_performance test_reliability; do
        if [ ! -x "$bin" ]; then continue; fi
        ((cov_total++))
        if timeout 30 "./$bin" > /dev/null 2>&1; then
            ((cov_pass++))
        else
            ((cov_fail++))
            echo "  FAIL: $bin"
        fi
    done
    echo "  -> $cov_pass / $cov_total passed"

    echo "[4/4] Collecting coverage with gcovr..."
    cd "$COV_DIR"
    if command -v gcovr &>/dev/null; then
        gcovr -r "$PROJECT_DIR" \
              --filter "$PROJECT_DIR/src/" \
              --exclude ".*_autogen.*" \
              --branches \
              --exclude-unreachable-branches \
              --print-summary \
              2>&1
        echo ""
        echo "--- Detailed: by source file (line + branch) ---"
        gcovr -r "$PROJECT_DIR" \
              --filter "$PROJECT_DIR/src/" \
              --exclude ".*_autogen.*" \
              --branches \
              --exclude-unreachable-branches \
              2>&1 | head -60
    else
        echo "  gcovr not found. Install with: pip install gcovr"
    fi
}

# ==================== 汇总 ====================
print_summary() {
    echo "============================================"
    echo " SUMMARY"
    echo "============================================"
    echo -e "  Total:   ${total}"
    echo -e "  Passed:  ${GREEN}${passed}${NC}"
    echo -e "  Failed:  ${RED}${failed}${NC}"
    echo -e "  Skipped: ${YELLOW}${skipped}${NC}"
    if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
        echo ""
        echo "  Failed tests:"
        for t in "${FAILED_TESTS[@]}"; do
            echo -e "    ${RED}- $t${NC}"
        done
    fi
    echo ""
}

# ==================== 入口 ====================
case "${1:-debug}" in
    coverage)
        run_coverage
        ;;
    debug|*)
        run_debug_tests
        ;;
esac
print_summary
