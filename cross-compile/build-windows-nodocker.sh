#!/bin/bash
# 不使用 Docker，直接用 MinGW-w64 交叉编译

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build-mingw"
PACKAGE_DIR="$PROJECT_ROOT/windows-package"
ONNX_VERSION="1.16.3"
ONNX_DIR="$PROJECT_ROOT/3rdparty/onnxruntime-win/onnxruntime-win-x64-${ONNX_VERSION}"

echo "🔨 本地 MinGW 交叉编译"
echo "================================"

# 检查编译器
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "❌ MinGW-w64 未安装"
    echo "请先运行: ./setup-mingw.sh"
    exit 1
fi

# 检查 ONNX
if [ ! -d "$ONNX_DIR" ]; then
    echo "📦 ONNX Runtime 未找到，请先运行: ./setup-cross-build.sh"
    exit 1
fi

# 创建工具链文件
mkdir -p "$BUILD_DIR"
cat > "$BUILD_DIR/toolchain.cmake" << EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

# 清理旧构建
rm -rf "$BUILD_DIR" "$PACKAGE_DIR"
mkdir -p "$BUILD_DIR" "$PACKAGE_DIR"

echo "🔧 配置 CMake..."
cd "$BUILD_DIR"
cmake "$PROJECT_ROOT" \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DONNX_INCLUDE_DIR="$ONNX_DIR/include" \
    -DONNXRUNTIME_LIB="$ONNX_DIR/lib/onnxruntime.lib" \
    -DSPM_USE_BUILTIN_PROTOBUF=OFF \
    -DBUILD_SHARED_LIBS=OFF

echo "🔨 开始编译..."
make -j$(nproc)

echo ""
echo "================================"
if [ -f "$BUILD_DIR/qt-try.exe" ]; then
    echo "✅ 构建成功！"
    cp "$BUILD_DIR/qt-try.exe" "$PACKAGE_DIR/"
    cp "$ONNX_DIR/lib/onnxruntime.dll" "$PACKAGE_DIR/" 2>/dev/null || true
    
    echo "📁 输出: $PACKAGE_DIR"
    ls -lh "$PACKAGE_DIR/"
else
    echo "❌ 构建失败"
    exit 1
fi
