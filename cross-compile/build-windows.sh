#!/bin/bash
# 使用社区 Qt6 镜像交叉编译 Windows 版本

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build-win-docker"
PACKAGE_DIR="$PROJECT_ROOT/windows-package"
ONNX_VERSION="1.16.3"
ONNX_DIR="$PROJECT_ROOT/3rdparty/onnxruntime-win/onnxruntime-win-x64-${ONNX_VERSION}"

# 社区 Qt6 镜像
DOCKER_IMAGE="a12e/docker-qt6-windows:6.5-mingw-w64-ltsc2022"

echo "🐳 Windows 交叉编译脚本 (社区 Qt6 镜像)"
echo "================================"

# 检查 Docker
if ! command -v docker &> /dev/null; then
    echo "❌ Docker 未安装"
    exit 1
fi

# 拉取镜像
if ! docker image inspect "$DOCKER_IMAGE" &> /dev/null; then
    echo "⬇️  拉取社区 Qt6 镜像..."
    docker pull "$DOCKER_IMAGE" || {
        echo "❌ 拉取失败，可能网络问题或镜像不存在"
        echo "💡 建议使用 GitHub Actions 方案（更简单）"
        exit 1
    }
fi

# 检查 ONNX Runtime
if [ ! -d "$ONNX_DIR" ]; then
    echo "📦 ONNX Runtime 未找到，请先运行: ./setup-cross-build.sh"
    exit 1
fi

# 清理旧构建
echo "🧹 清理旧构建..."
rm -rf "$BUILD_DIR" "$PACKAGE_DIR"
mkdir -p "$BUILD_DIR" "$PACKAGE_DIR"

echo "🔨 开始构建..."
docker run --rm \
    -v "$PROJECT_ROOT:/project" \
    -v "$BUILD_DIR:/build" \
    -v "$ONNX_DIR:/onnx:ro" \
    -v "$PACKAGE_DIR:/package" \
    "$DOCKER_IMAGE" \
    bash -c "
        echo '🔧 配置 CMake...' &&
        cd /build &&
        cmake /project \\
            -DCMAKE_BUILD_TYPE=Release \\
            -DCMAKE_PREFIX_PATH=/opt/qt6 \\
            -DONNX_INCLUDE_DIR=/onnx/include \\
            -DONNXRUNTIME_LIB=/onnx/lib/onnxruntime.lib \\
            -DBUILD_SHARED_LIBS=OFF &&
        
        echo '🔨 编译...' &&
        cmake --build . --parallel \\
            
        echo '📦 复制到打包目录...' &&
        cp /build/qt-try.exe /package/ &&
        cp /onnx/lib/onnxruntime.dll /package/ 2>/dev/null || true
    "

echo ""
echo "================================"
if [ -f "$PACKAGE_DIR/qt-try.exe" ]; then
    echo "✅ 构建成功！"
    echo "📁 Windows 可执行文件: $PACKAGE_DIR/qt-try.exe"
    ls -lh "$PACKAGE_DIR/"
else
    echo "❌ 构建失败"
    exit 1
fi
