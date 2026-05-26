#!/bin/bash
# ============================================================
# 源代码提取脚本
# 功能：将项目中所有 C/C++ 源文件内容合并输出到单个文件
#       方便将代码整体提交给 AI 分析或生成文档
# 位置：scripts/script.sh
# 用法：./scripts/script.sh [输出文件名]
#       默认输出文件: extracted_files.txt
# ============================================================

# 项目根目录（脚本位于 scripts/ 子目录，上级即为项目根目录）
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

OUTPUT="${1:-extracted_files.txt}"   # 可选参数指定输出文件名，默认为 extracted_files.txt

# 清空输出文件（或创建新文件）
> "$OUTPUT"

# 切换到项目根目录，查找所有 C/C++ 源文件
cd "$PROJECT_ROOT"

# 排除构建目录和第三方库目录，避免输出文件过大
find . -type d \( -name "cmake-build-debug" -o -name "build" -o -name "3rdparty" \) -prune -o \
       -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) -print0 | while IFS= read -r -d '' file; do
    # 写入文件相对路径（去掉前导 "./"）
    relative="${file#./}"
    echo "$relative" >> "$OUTPUT"
    # 追加文件内容
    cat "$file" >> "$OUTPUT"
done

echo "所有文件已提取到 $OUTPUT"
