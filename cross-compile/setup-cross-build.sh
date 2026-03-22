#!/bin/bash
# 设置交叉编译环境

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
ONNX_DIR="$PROJECT_ROOT/3rdparty/onnxruntime-win"

echo "📦 下载 ONNX Runtime Windows 版本..."

# 创建目录
mkdir -p "$ONNX_DIR"
cd "$ONNX_DIR"

# 下载 ONNX Runtime Windows 版本 (MinGW 可以用 .dll 版本)
ONNX_VERSION="1.16.3"
ONNX_ZIP="onnxruntime-win-x64-${ONNX_VERSION}.zip"

if [ ! -f "$ONNX_ZIP" ]; then
    echo "⬇️ 下载 ONNX Runtime ${ONNX_VERSION}..."
    wget "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/${ONNX_ZIP}" || {
        echo "⚠️ 下载失败，请手动下载:"
        echo "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/${ONNX_ZIP}"
        echo "并放到: $ONNX_DIR"
        exit 1
    }
fi

# 解压
if [ ! -d "onnxruntime-win-x64-${ONNX_VERSION}" ]; then
    echo "📂 解压..."
    unzip -q "$ONNX_ZIP"
fi

echo "✅ ONNX Runtime 准备完成: $ONNX_DIR/onnxruntime-win-x64-${ONNX_VERSION}"

# 创建 CMake 工具链文件模板
cat > "$SCRIPT_DIR/cmake/mxe-toolchain.cmake" << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MXE 编译器
set(CMAKE_C_COMPILER /usr/lib/mxe/usr/bin/x86_64-w64-mingw32.static-gcc)
set(CMAKE_CXX_COMPILER /usr/lib/mxe/usr/bin/x86_64-w64-mingw32.static-g++)
set(CMAKE_RC_COMPILER /usr/lib/mxe/usr/bin/x86_64-w64-mingw32.static-windres)

# 查找路径
set(CMAKE_FIND_ROOT_PATH /usr/lib/mxe/usr/x86_64-w64-mingw32.static)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Qt6 路径
set(Qt6_DIR /usr/lib/mxe/usr/x86_64-w64-mingw32.static/qt6/lib/cmake/Qt6)
EOF

echo "✅ 工具链文件创建完成: $SCRIPT_DIR/cmake/mxe-toolchain.cmake"
echo ""
echo "💡 现在可以运行: cd cross-compile && ./build-windows.sh"
