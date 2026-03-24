#!/bin/bash
# 在本地使用 Docker 编译 Windows 版本
# 尝试多个镜像，自动选择可用的

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build-win-docker"
PACKAGE_DIR="$PROJECT_ROOT/windows-package"
ONNX_VERSION="1.16.3"
ONNX_DIR="$PROJECT_ROOT/3rdparty/onnxruntime-win/onnxruntime-win-x64-${ONNX_VERSION}"

echo "🐳 本地 Windows 交叉编译"
echo "================================"

# 检查 Docker
if ! command -v docker &> /dev/null; then
    echo "❌ Docker 未安装，请先安装: sudo pacman -S docker"
    exit 1
fi

# 检查 ONNX
if [ ! -d "$ONNX_DIR" ]; then
    echo "📦 下载 ONNX Runtime..."
    ./setup-cross-build.sh
fi

# 清理旧构建
rm -rf "$BUILD_DIR" "$PACKAGE_DIR"
mkdir -p "$BUILD_DIR" "$PACKAGE_DIR"

# 尝试的镜像列表（按优先级）
IMAGES=(
    "a12e/docker-qt6-windows:6.5-mingw-w64-ltsc2022"
    "stateoftheartio/qt6:6.5-mingw-w64"
    "michalrad/dotnet-qt-mingw:latest"
)

WORKING_IMAGE=""

for IMAGE in "${IMAGES[@]}"; do
    echo ""
    echo "🔍 尝试镜像: $IMAGE"
    
    if docker pull "$IMAGE" 2>/dev/null; then
        echo "✅ 拉取成功，测试编译..."
        
        # 简单测试 cmake 是否可用
        if docker run --rm "$IMAGE" cmake --version &>/dev/null; then
            WORKING_IMAGE="$IMAGE"
            echo "✅ 镜像可用！"
            break
        fi
    else
        echo "❌ 拉取失败，尝试下一个..."
    fi
done

if [ -z "$WORKING_IMAGE" ]; then
    echo ""
    echo "😭 所有社区镜像都不可用"
    echo ""
    echo "方案 1: 检查网络/翻墙后重试"
    echo "方案 2: 使用自建 Qt6 镜像（耗时 2-4 小时）:"
    echo "  ./build-qt6-image.sh && ./build-windows.sh"
    echo ""
    exit 1
fi

echo ""
echo "🔨 使用镜像: $WORKING_IMAGE"
echo "================================"

# 开始编译
docker run --rm \
    -v "$PROJECT_ROOT:/project" \
    -v "$BUILD_DIR:/build" \
    -v "$ONNX_DIR:/onnx:ro" \
    -v "$PACKAGE_DIR:/package" \
    "$WORKING_IMAGE" \
    bash -c "
        echo '🔧 配置 CMake...' &&
        cd /build &&
        cmake /project \\
            -DCMAKE_BUILD_TYPE=Release \\
            -DCMAKE_PREFIX_PATH=/opt/qt6 \\
            -DONNX_INCLUDE_DIR=/onnx/include \\
            -DONNXRUNTIME_LIB=/onnx/lib/onnxruntime.lib \\
            -DBUILD_SHARED_LIBS=OFF \\
            2>&1 &&
        
        echo '🔨 编译...' &&
        cmake --build . --parallel 2>&1 &&
        
        echo '📦 打包...' &&
        cp /build/qt-try.exe /package/ &&
        cp /onnx/lib/onnxruntime.dll /package/ 2>/dev/null || true
    "

# 检查结果
echo ""
echo "================================"
if [ -f "$PACKAGE_DIR/qt-try.exe" ]; then
    echo "✅ 构建成功！"
    echo "📁 输出目录: $PACKAGE_DIR"
    ls -lh "$PACKAGE_DIR/"
    
    # 创建压缩包
    cd "$PROJECT_ROOT"
    zip -r "qt-try-windows.zip" "windows-package/"
    echo ""
    echo "📦 压缩包已创建: qt-try-windows.zip"
else
    echo "❌ 构建失败"
    exit 1
fi
