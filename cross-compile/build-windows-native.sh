#!/bin/bash
# 在 Windows 上使用 Qt 官方工具链编译
# 此脚本生成在 Windows 上运行的批处理命令

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)

echo "📝 生成 Windows 编译脚本..."
echo "================================"

# 创建 Windows 批处理文件
cat > "$PROJECT_ROOT/build-windows.bat" << 'EOF'
@echo off
chcp 65001 >nul
echo 🐳 Windows 本地编译脚本
echo ================================

:: 设置 Qt 路径（根据你的安装位置修改）
set QT_PATH=C:\Qt\6.5.3\mingw_64
set PATH=%QT_PATH%\bin;%PATH%

:: 设置 MinGW 路径
set MINGW_PATH=%QT_PATH%\bin

:: 创建构建目录
if exist build-windows rmdir /s /q build-windows
mkdir build-windows
cd build-windows

echo 🔧 配置 CMake...
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
    -DONNX_INCLUDE_DIR="C:\onnx\include" ^
    -DONNXRUNTIME_LIB="C:\onnx\lib\onnxruntime.lib"

echo 🔨 编译...
mingw32-make -j%NUMBER_OF_PROCESSORS%

echo.
echo ================================
if exist qt-try.exe (
    echo ✅ 构建成功！
    echo 📁 qt-try.exe
) else (
    echo ❌ 构建失败
)
pause
EOF

echo "✅ 已生成 build-windows.bat"
echo ""
echo "📝 Windows 设置步骤："
echo "================================"
echo ""
echo "1️⃣  下载并安装 Qt 在线安装器："
echo "    https://www.qt.io/download-qt-installer"
echo ""
echo "2️⃣  在安装器中选择："
echo "    - Qt 6.5+ (或你需要的版本)"
echo "    - MinGW 11.x 64-bit"  
echo "    - CMake"
echo "    - Ninja"
echo ""
echo "3️⃣  下载 ONNX Runtime Windows 版本："
echo "    https://github.com/microsoft/onnxruntime/releases"
echo "    解压到 C:\onnx"
echo ""
echo "4️⃣  双击运行 build-windows.bat"
echo ""
echo "📦 打包发布："
echo "    windeployqt qt-try.exe"
echo ""

# 同时生成 PowerShell 版本
cat > "$PROJECT_ROOT/build-windows.ps1" << 'EOF'
# Windows 本地编译脚本 (PowerShell)
Write-Host "🐳 Windows 本地编译脚本" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan

# 设置 Qt 路径（根据你的安装位置修改）
$QT_PATH = "C:\Qt\6.5.3\mingw_64"
$env:PATH = "$QT_PATH\bin;$env:PATH"

# 创建构建目录
if (Test-Path "build-windows") {
    Remove-Item -Recurse -Force "build-windows"
}
New-Item -ItemType Directory -Name "build-windows" | Out-Null
Set-Location "build-windows"

Write-Host "🔧 配置 CMake..." -ForegroundColor Yellow
cmake .. -G "MinGW Makefiles" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$QT_PATH" `
    -DONNX_INCLUDE_DIR="C:\onnx\include" `
    -DONNXRUNTIME_LIB="C:\onnx\lib\onnxruntime.lib"

Write-Host "🔨 编译..." -ForegroundColor Yellow
mingw32-make -j$env:NUMBER_OF_PROCESSORS

Write-Host ""
Write-Host "================================" -ForegroundColor Cyan
if (Test-Path "qt-try.exe") {
    Write-Host "✅ 构建成功！" -ForegroundColor Green
    Write-Host "📁 qt-try.exe" -ForegroundColor Green
    
    # 自动打包
    Write-Host "📦 正在打包依赖..." -ForegroundColor Yellow
    windeployqt qt-try.exe
} else {
    Write-Host "❌ 构建失败" -ForegroundColor Red
}

Read-Host "按 Enter 键退出"
EOF

echo "✅ 已生成 build-windows.ps1"
