#!/bin/bash
# 构建带 Qt6 的 Docker 镜像
# ⚠️  警告：这需要编译 Qt6，可能需要 2-4 小时！

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

echo "🐳 构建带 Qt6 的交叉编译镜像"
echo "================================"
echo "⚠️  警告：这非常耗时（2-4 小时）"
echo "⏸️  按 Ctrl+C 取消，或按 Enter 继续..."
read -r

cd "$SCRIPT_DIR"

echo "🔨 开始构建..."
docker build -t dockcross-qt6:latest -f Dockerfile.qt6 .

echo ""
echo "✅ 镜像构建完成: dockcross-qt6:latest"
