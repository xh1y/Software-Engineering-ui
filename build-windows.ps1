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
