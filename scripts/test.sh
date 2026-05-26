#!/bin/bash

# ====================================================
# OptiKG 测试脚本
# 功能：构建并运行项目的单元测试套件
# 位置：scripts/test.sh
# 用法：./scripts/test.sh [选项] [测试名称...]
#       --build   构建所有测试
#       --all     运行所有测试
#       --info    显示测试列表
# ====================================================

set +e  # 禁用错误退出（部分测试失败是预期行为，脚本会继续运行其他测试）

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# --- 项目路径（脚本位于 scripts/ 子目录，上级即为项目根目录）---
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TESTS_DIR="$BUILD_DIR/tests"

# --- 测试可执行文件列表 ---
ALL_TESTS=(
    "test_mainwindow"         # 主窗口UI测试 (BB-01, BB-02, BB-04, BB-07)
    "test_batchprocessdialog" # 批量处理对话框测试 (BB-15)
    "test_batchprocessor"     # 批量处理器测试 (BB-08, BB-10, BB-12, BB-13, BB-14, BB-36, BB-37)
    "test_performance"        # 性能测试 (PF-01 到 PF-07)
    "test_reliability"        # 可靠性测试 (RB-01 到 RB-06)
    "test_inferenceengine"    # 推理引擎测试
    "test_databasemanager"    # 数据库管理器测试
    "test_datamodel"          # 数据模型测试
    "test_configmanager"      # 配置管理器测试
    "test_graphwidget"        # 图谱控件测试
    "test_inferenceworker"    # 推理工作线程测试
    "test_appconstants"       # 应用常量测试
    "test_stylemanager"       # 样式管理器测试
    "test_settingsdialog"     # 设置对话框UI测试
    "test_resulttreewidget"   # 结果树控件测试
)

# --- 需要特殊参数的测试分类 ---
PERFORMANCE_TESTS=("test_performance")
RELIABILITY_TESTS=("test_reliability")
UI_TESTS=("test_mainwindow" "test_batchprocessdialog" "test_settingsdialog" "test_resulttreewidget")

# --- 全局开关变量 ---
VERBOSE=false
BUILD_TESTS=false
RUN_ALL=false
RUN_SPECIFIC=()
SKIP_BUILD=false
OUTPUT_FILE=""
ENABLE_PERF=false
ENABLE_LONGRUN=false
ENABLE_CONCURRENT=false

# ====================================================
# 工具函数
# ====================================================

print_header() {
    echo -e "${PURPLE}"
    echo "================================================"
    echo "  OptiKG 测试套件"
    echo "================================================"
    echo -e "${NC}"
}

print_section() {
    echo -e "${CYAN}"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
    echo -e "${NC}"
}

print_success() {
    echo -e "${GREEN}[✓] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[!] $1${NC}"
}

print_error() {
    echo -e "${RED}[✗] $1${NC}"
}

print_info() {
    echo -e "${BLUE}[i] $1${NC}"
}

check_build_dir() {
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "构建目录不存在: $BUILD_DIR"
        print_info "请先在项目根目录运行: mkdir -p build && cd build && cmake .."
        exit 1
    fi
}

check_test_exists() {
    local test_name="$1"
    local test_path="$TESTS_DIR/$test_name"

    if [ ! -f "$test_path" ]; then
        print_warning "测试可执行文件不存在: $test_name"
        return 1
    fi

    if [ ! -x "$test_path" ]; then
        print_warning "测试文件不可执行: $test_name"
        return 1
    fi

    return 0
}

# ====================================================
# 构建函数
# ====================================================

build_tests() {
    print_section "构建测试"

    check_build_dir

    cd "$BUILD_DIR"

    # 逐个构建所有测试目标
    for test_name in "${ALL_TESTS[@]}"; do
        print_info "构建: $test_name"
        if ! make "$test_name" 2>&1 | tee -a /tmp/build_${test_name}.log; then
            print_error "构建失败: $test_name"
            print_info "查看详细日志: /tmp/build_${test_name}.log"
            return 1
        fi
    done

    print_success "所有测试构建完成"
}

# ====================================================
# 测试运行函数
# ====================================================

run_single_test() {
    local test_name="$1"
    local test_path="$TESTS_DIR/$test_name"

    print_info "运行测试: $test_name"

    # 检查测试文件是否存在
    if ! check_test_exists "$test_name"; then
        print_warning "跳过测试: $test_name (未找到可执行文件)"
        return 2
    fi

    # 准备测试参数
    local test_args=""

    # 添加性能测试标志
    if [[ " ${PERFORMANCE_TESTS[@]} " =~ " ${test_name} " ]] && [ "$ENABLE_PERF" = true ]; then
        test_args="-perf"
        print_info "启用性能测试标志: -perf"
    fi

    # 可靠性测试通过环境变量传递参数
    if [[ " ${RELIABILITY_TESTS[@]} " =~ " ${test_name} " ]]; then
        if [ "$ENABLE_LONGRUN" = true ]; then
            print_info "启用长时间运行测试标志（通过环境变量）"
        fi
        if [ "$ENABLE_CONCURRENT" = true ]; then
            print_info "启用并发测试标志（通过环境变量）"
        fi
    fi

    # 设置环境变量
    local env_vars=""
    if [ "$ENABLE_LONGRUN" = true ]; then
        env_vars="$env_vars OPTIKG_LONGRUN=1"
    fi
    if [ "$ENABLE_CONCURRENT" = true ]; then
        env_vars="$env_vars OPTIKG_CONCURRENT=1"
    fi

    # 执行测试并计时
    local start_time=$(date +%s)
    local output=""

    if [ "$VERBOSE" = true ]; then
        if [ -n "$test_args" ]; then
            output=$(env $env_vars "$test_path" $test_args 2>&1)
        else
            output=$(env $env_vars "$test_path" 2>&1)
        fi
    else
        if [ -n "$test_args" ]; then
            output=$(env $env_vars "$test_path" $test_args 2>&1 | grep -E "PASS|FAIL|SKIP|Totals|QDEBUG|QWARN|QCRITICAL")
        else
            output=$(env $env_vars "$test_path" 2>&1 | grep -E "PASS|FAIL|SKIP|Totals|QDEBUG|QWARN|QCRITICAL")
        fi
    fi

    local exit_code=$?
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    # 输出单测结果
    echo "----------------------------------------"
    echo "测试: $test_name"
    echo "耗时: ${duration}秒"
    echo "退出码: $exit_code"
    echo "----------------------------------------"
    echo "$output"
    echo "----------------------------------------"

    # 记录到输出文件（如果指定了 -o）
    if [ -n "$OUTPUT_FILE" ]; then
        {
            echo "========================================"
            echo "测试: $test_name"
            echo "时间: $(date)"
            echo "耗时: ${duration}秒"
            echo "退出码: $exit_code"
            echo "----------------------------------------"
            echo "$output"
        } >> "$OUTPUT_FILE"
    fi

    # 判断测试结果
    if [ $exit_code -eq 0 ]; then
        print_success "测试通过: $test_name"
        return 0
    elif [ $exit_code -eq 2 ]; then
        print_warning "测试跳过: $test_name (退出码: $exit_code)"
        return 2
    else
        print_error "测试失败: $test_name (退出码: $exit_code)"
        return 1
    fi
}

run_all_tests() {
    print_section "运行所有测试"

    check_build_dir

    local total_tests=0
    local passed_tests=0
    local failed_tests=0
    local skipped_tests=0

    # 初始化输出文件
    if [ -n "$OUTPUT_FILE" ]; then
        echo "OptiKG 测试报告" > "$OUTPUT_FILE"
        echo "生成时间: $(date)" >> "$OUTPUT_FILE"
        echo "========================================" >> "$OUTPUT_FILE"
    fi

    # 逐个运行测试
    for test_name in "${ALL_TESTS[@]}"; do
        ((total_tests++))

        if run_single_test "$test_name"; then
            ((passed_tests++))
        else
            local result=$?
            if [ $result -eq 2 ]; then
                ((skipped_tests++))
            else
                ((failed_tests++))
            fi
        fi

        echo ""
    done

    # 打印汇总报告
    print_section "测试汇总"
    echo "总计测试: $total_tests"
    echo -e "${GREEN}通过: $passed_tests${NC}"
    echo -e "${RED}失败: $failed_tests${NC}"
    echo -e "${YELLOW}跳过: $skipped_tests${NC}"

    # 追加汇总到输出文件
    if [ -n "$OUTPUT_FILE" ]; then
        {
            echo "========================================"
            echo "测试汇总:"
            echo "总计测试: $total_tests"
            echo "通过: $passed_tests"
            echo "失败: $failed_tests"
            echo "跳过: $skipped_tests"
            echo "生成时间: $(date)"
        } >> "$OUTPUT_FILE"
        print_info "详细报告已保存到: $OUTPUT_FILE"
    fi

    if [ $failed_tests -eq 0 ]; then
        print_success "所有测试通过!"
        return 0
    else
        print_error "有 $failed_tests 个测试失败"
        return 1
    fi
}

# ====================================================
# 信息显示函数
# ====================================================

show_test_info() {
    print_header

    echo -e "${CYAN}可用的测试:${NC}"
    for i in "${!ALL_TESTS[@]}"; do
        echo "  $((i+1)). ${ALL_TESTS[$i]}"
    done

    echo ""
    echo -e "${CYAN}测试分类:${NC}"
    echo "  UI测试: ${UI_TESTS[*]}"
    echo "  性能测试: ${PERFORMANCE_TESTS[*]} (需要 -perf 标志)"
    echo "  可靠性测试: ${RELIABILITY_TESTS[*]} (需要 -longrun/-concurrent 标志)"

    echo ""
    echo -e "${CYAN}测试说明:${NC}"
    echo "  1. test_mainwindow - 主窗口功能测试"
    echo "  2. test_batchprocessdialog - 批量处理对话框测试"
    echo "  3. test_performance - 性能测试自动化"
    echo "  4. test_reliability - 可靠性测试自动化"
    echo "  5. test_inferenceworker - 推理工作线程测试"
    echo "  6. test_appconstants - 应用常量测试"
    echo "  7. test_stylemanager - 样式管理器测试"
    echo "  8. test_settingsdialog - 设置对话框UI测试"
    echo "  9. test_resulttreewidget - 结果树控件测试"
}

show_help() {
    echo -e "${GREEN}OptiKG 测试脚本使用说明${NC}"
    echo ""
    echo "用法: $0 [选项] [测试名称...]"
    echo ""
    echo "选项:"
    echo "  -h, --help            显示此帮助信息"
    echo "  -v, --verbose         详细输出模式"
    echo "  -b, --build           构建所有测试"
    echo "  -a, --all             运行所有测试"
    echo "  -s, --skip-build      跳过构建步骤（假设测试已构建）"
    echo "  -o, --output FILE     将输出保存到文件"
    echo "  --perf                启用性能测试（需要 -perf 标志的测试）"
    echo "  --longrun             启用长时间运行测试（可靠性测试）"
    echo "  --concurrent          启用并发测试（可靠性测试）"
    echo "  --info                显示测试信息"
    echo ""
    echo "示例:"
    echo "  $0 --info                    # 显示测试信息"
    echo "  $0 --build                   # 构建所有测试"
    echo "  $0 --all                     # 运行所有测试"
    echo "  $0 --all --perf              # 运行所有测试，启用性能测试"
    echo "  $0 test_mainwindow           # 运行特定测试"
    echo "  $0 -b -a --perf              # 构建并运行所有测试，启用性能测试"
    echo "  $0 -o test_report.log --all  # 运行所有测试并保存报告"
    echo ""
}

# ====================================================
# 参数解析
# ====================================================

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -b|--build)
                BUILD_TESTS=true
                shift
                ;;
            -a|--all)
                RUN_ALL=true
                shift
                ;;
            -s|--skip-build)
                SKIP_BUILD=true
                shift
                ;;
            -o|--output)
                OUTPUT_FILE="$2"
                shift 2
                ;;
            --perf)
                ENABLE_PERF=true
                shift
                ;;
            --longrun)
                ENABLE_LONGRUN=true
                shift
                ;;
            --concurrent)
                ENABLE_CONCURRENT=true
                shift
                ;;
            --info)
                show_test_info
                exit 0
                ;;
            -*)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
            *)
                # 验证是否为已知测试名称
                if [[ " ${ALL_TESTS[@]} " =~ " $1 " ]]; then
                    RUN_SPECIFIC+=("$1")
                else
                    print_warning "未知测试名称: $1 (已忽略)"
                fi
                shift
                ;;
        esac
    done

    # 如果没有指定任何操作，显示帮助
    if [ "$BUILD_TESTS" = false ] && [ "$RUN_ALL" = false ] && [ ${#RUN_SPECIFIC[@]} -eq 0 ]; then
        show_help
        exit 0
    fi
}

# ====================================================
# 主函数
# ====================================================

main() {
    print_header

    # 解析参数
    parse_args "$@"

    # 构建测试
    if [ "$BUILD_TESTS" = true ] && [ "$SKIP_BUILD" = false ]; then
        if ! build_tests; then
            print_error "构建失败，终止测试"
            exit 1
        fi
    fi

    # 运行测试
    if [ "$RUN_ALL" = true ]; then
        run_all_tests
    elif [ ${#RUN_SPECIFIC[@]} -gt 0 ]; then
        print_section "运行指定测试"
        local passed=0
        local failed=0

        for test_name in "${RUN_SPECIFIC[@]}"; do
            if run_single_test "$test_name"; then
                ((passed++))
            else
                ((failed++))
            fi
        done

        echo ""
        echo -e "${GREEN}通过: $passed${NC}"
        echo -e "${RED}失败: $failed${NC}"

        if [ $failed -eq 0 ]; then
            print_success "所有指定测试通过!"
        else
            print_error "有 $failed 个测试失败"
            exit 1
        fi
    fi

    print_success "测试执行完成"
}

# ====================================================
# 脚本入口
# ====================================================

# 确保从项目根目录执行（构建系统要求）
cd "$PROJECT_ROOT"

# 运行主函数
main "$@"

# 恢复原始工作目录
cd - > /dev/null
