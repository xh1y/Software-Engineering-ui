# #!/bin/bash
# set -uo pipefail

# PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
# COV_DIR="$PROJECT_DIR/cmake-build-coverage"
# HTML_DIR="$PROJECT_DIR/docs/coverage_html"

# echo "=== [1/5] 检查环境 ==="
# mkdir -p "$COV_DIR"
# cd "$COV_DIR"

# # 只有在文件夹为空（第一次运行）时才运行完整的 CMake 配置
# if [ ! -f "CMakeCache.txt" ]; then
#     echo "首次运行，正在配置项目..."
#     cmake "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage"
# fi

# # --- 核心优化步骤 ---
# echo "=== [2/5] 增量编译 (只编译修改的部分) ==="
# cmake --build . -j$(nproc)

# echo "=== [3/5] 重置旧的覆盖率数据 (不删代码，只删运行记录) ==="
# # 使用 lcov 的工具重置所有计数器，或者直接删除所有 .gcda 文件
# # 这一步是解决 "stamp mismatch" 的关键，且不需要重新编译
# find . -name "*.gcda" -delete

# echo "=== [4/5] 运行测试 (允许部分失败) ==="
# # export QT_QPA_PLATFORM=offscreen
# # 使用 ctest 运行速度更快
# ctest --output-on-failure -j$(nproc) || echo "提示：有测试未通过，但将继续生成报告"

# echo "=== [5/5] 快速生成报告 ==="
# echo "=== [5/5] 快速生成报告 (剔除头文件) ==="
# LCOV_OPTS="--rc branch_coverage=1 --ignore-errors mismatch,unused,empty"

# # 1. 采集原始数据
# lcov --capture --directory . --base-directory "$PROJECT_DIR" \
#      --output-file "tmp.info" $LCOV_OPTS --quiet

# # 2. 第一步过滤：只保留 src 目录下的内容
# lcov --extract "tmp.info" "$PROJECT_DIR/src/*" \
#      --output-file "src_only.info" $LCOV_OPTS --quiet

# # 3. 第二步过滤：彻底移除所有 .h 文件 (关键步骤)
# lcov --remove "src_only.info" "*.h" \
#      --output-file "final.info" $LCOV_OPTS --quiet

# # 4. 生成 HTML
# rm -rf "$HTML_DIR"
# genhtml "final.info" --output-directory "$HTML_DIR" \
#         --title "OptiKG Coverage (No Headers)" --legend --rc branch_coverage=1 --quiet

#!/bin/bash
set -uo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
COV_DIR="$PROJECT_DIR/cmake-build-coverage"
HTML_DIR="$PROJECT_DIR/docs/coverage_html"

echo "=== [1/5] 检查环境与编译器 ==="
# 检查 grcov 是否已安装
if ! command -v grcov &> /dev/null; then
    echo "❌ 错误: 未找到 grcov 工具！"
    echo "请运行: cargo install grcov"
    echo "或从 https://github.com/mozilla/grcov/releases 下载二进制文件放到 PATH 中。"
    exit 1
fi

mkdir -p "$COV_DIR"
cd "$COV_DIR"
export CC=clang
export CXX=clang++
# 只有在文件夹为空（第一次运行）时才运行完整的 CMake 配置
if [ ! -f "CMakeCache.txt" ]; then
    echo "首次运行，正在配置项目 (强制使用 Clang) ..."
    
    # 强制指定编译器为 Clang
    export CC=clang
    export CXX=clang++
    
    # Clang Source-based Coverage 专属 flag
    CLANG_COV_FLAGS="-fprofile-instr-generate -fcoverage-mapping"
    
    # 注意：使用 Clang 覆盖率时，Linker 也必须带上 -fprofile-instr-generate
    cmake "$PROJECT_DIR" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="$CLANG_COV_FLAGS" \
        -DCMAKE_C_FLAGS="$CLANG_COV_FLAGS" \
        -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate" \
        -DCMAKE_SHARED_LINKER_FLAGS="-fprofile-instr-generate"
fi

echo "=== [2/5] 增量编译 (只编译修改的部分) ==="
cmake --build . -j$(nproc)

echo "=== [3/5] 重置旧的覆盖率数据 ==="
# Clang 不生成 .gcda，而是生成 .profraw 文件。在这里清理旧文件
find . -name "*.profraw" -delete

echo "=== [4/5] 运行测试 (允许部分失败) ==="
# 关键点：%p 会替换为进程 PID。
# 这对于 ctest -j 并发运行极其重要，防止多个测试进程互相覆盖 profraw 文件！
export LLVM_PROFILE_FILE="optikg_test_%p.profraw"

ctest --output-on-failure -j$(nproc) || echo "提示：有测试未通过，但将继续生成报告"

echo "=== [5/5] 使用 grcov 生成绝美 HTML 报告 ==="
rm -rf "$HTML_DIR"

# 一条命令完成 Lcov 之前 4 条命令的工作 (收集、提取 src、移除 .h、生成网页)
# 解析：
# -s "$PROJECT_DIR" : 指定代码的根目录
# --binary-path .   : Clang 模式下，grcov 需要读取二进制文件中的 mapping 数据
# -t html           : 生成 HTML 格式
# --branch          : 开启分支覆盖率统计
# --keep-only       : 仅保留 src 目录下的文件
# --ignore          : 排除所有的头文件
grcov . \
    -s "$PROJECT_DIR" \
    --binary-path . \
    -t html \
    --branch \
    --ignore-not-existing \
    --keep-only "src/**/*" \
    --ignore "**/*.h" \
    -o "$HTML_DIR"

echo "========================================="
echo "✅ 覆盖率报告已生成!"
echo "👉 请在浏览器中打开: file://$HTML_DIR/index.html"
echo "========================================="