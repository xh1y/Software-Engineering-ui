#!/bin/bash
# 构建带 protobuf 的 dockcross 镜像

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

echo "🔨 构建带 protobuf 的 dockcross 镜像"
echo "================================"
echo "⚠️  这需要编译 protobuf，约 5-10 分钟"
echo ""

cd "$SCRIPT_DIR"
docker build -t dockcross-win-protobuf:latest -f Dockerfile .

echo ""
echo "✅ 镜像构建完成: dockcross-win-protobuf:latest"
