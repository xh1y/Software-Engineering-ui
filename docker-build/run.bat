@echo off
chcp 65001 > nul
echo =========================================
echo    OptiKG 信息抽取工具 Docker 启动脚本 (Windows)
echo =========================================
echo.

REM 检查 Docker 是否安装
where docker >nul 2>nul
if %errorlevel% neq 0 (
    echo ❌ 未检测到 Docker，请先安装 Docker Desktop
    echo    下载地址：https://www.docker.com/products/docker-desktop
    echo    安装后请重启电脑
    pause
    exit /b 1
)

REM 检查 Docker Compose
docker compose version >nul 2>nul
if %errorlevel% equ 0 (
    set COMPOSE_CMD=docker compose
) else (
    docker-compose version >nul 2>nul
    if %errorlevel% equ 0 (
        set COMPOSE_CMD=docker-compose
    ) else (
        echo ❌ 未检测到 docker-compose，请安装 Docker Compose V2
        pause
        exit /b 1
    )
)

echo ✅ Docker 环境检测通过
echo.

echo =========================================
echo 🪟 Windows 系统配置
echo =========================================
echo.

echo 重要提示：
echo 1. 请确保已安装 VcXsrv (X11 服务器)
echo    下载地址：https://sourceforge.net/projects/vcxsrv/
echo.
echo 2. 启动 VcXsrv 时请选择：
echo    - Multiple windows
echo    - Display number: 0
echo    - 勾选 'Disable access control'
echo    - 其他选项默认
echo.
echo 3. 确保 VcXsrv 正在运行（任务栏有图标）
echo.

set /p confirm=按回车键继续（确保 VcXsrv 已启动）...

REM 设置环境变量
set DISPLAY=host.docker.internal:0
set PRIVILEGED=false
set QT_QPA_PLATFORM=xcb
set QT_X11_NO_MITSHM=1

echo.
echo 🔨 开始构建 Docker 镜像（首次运行较慢，约5-10分钟）...
echo    如果中途失败，会自动尝试简化版构建
echo.

REM 首先尝试修复的简化版构建（Ubuntu 22.04 正确包名）
set COMPOSE_FILE=docker-compose.simple.yml
echo.
echo 🔄 尝试使用修复的简化版构建...
echo    使用的配置文件: %COMPOSE_FILE%
echo.

%COMPOSE_CMD% -f %COMPOSE_FILE% build --progress plain
if %errorlevel% equ 0 (
    echo ✅ 修复的简化版构建成功！
    goto :build_success
)

REM 简化版失败，尝试Debian Bullseye
echo.
echo ⚠️  简化版构建失败，尝试Debian Bullseye构建...
set COMPOSE_FILE=docker-compose.debian-bullseye.yml
echo    使用的配置文件: %COMPOSE_FILE%
echo.

%COMPOSE_CMD% -f %COMPOSE_FILE% build --progress plain
if %errorlevel% equ 0 (
    echo ✅ Debian Bullseye构建成功！
    goto :build_success
)

REM Debian Bullseye失败，尝试Qt官方镜像
echo.
echo ⚠️  Debian Bullseye构建失败，尝试Qt官方镜像构建...
set COMPOSE_FILE=docker-compose.qt-official.yml
echo    使用的配置文件: %COMPOSE_FILE%
echo.

%COMPOSE_CMD% -f %COMPOSE_FILE% build --progress plain
if %errorlevel% equ 0 (
    echo ✅ Qt官方镜像构建成功！
    goto :build_success
)

REM Qt官方镜像失败，尝试原版
echo.
echo ⚠️  Qt官方镜像构建失败，尝试原版构建...
set COMPOSE_FILE=docker-compose.yml
echo    使用的配置文件: %COMPOSE_FILE%
echo.

%COMPOSE_CMD% -f %COMPOSE_FILE% build --progress plain
if %errorlevel% equ 0 (
    echo ✅ 原版构建成功！
    goto :build_success
)

REM 原版也失败，尝试不带缓存
echo.
echo ⚠️  原版构建失败，尝试不带缓存重新构建...
echo.

%COMPOSE_CMD% -f %COMPOSE_FILE% build --no-cache --progress plain
if %errorlevel% equ 0 (
    echo ✅ 不带缓存构建成功！
    goto :build_success
)

REM 所有尝试都失败
echo.
echo ❌ 所有构建尝试都失败了
echo.
echo 🔥 常见问题解决方案：
echo    1. 重启 Docker Desktop
echo    2. 清理 Docker: docker system prune -a
echo    3. 检查磁盘空间
echo    4. 在 Docker Desktop 设置中增加内存（至少 4GB）
echo    5. 关闭 VPN 或代理
echo.
echo 📋 手动构建命令（尝试修复的简化版）:
echo    cd %CD%
echo    docker build -f Dockerfile.simple -t optikg ..
echo    docker run -e DISPLAY=host.docker.internal:0 -v /tmp/.X11-unix:/tmp/.X11-unix optikg
echo.
pause
exit /b 1

:build_success

echo.
echo ✅ 构建成功！启动应用程序...
echo    使用的配置: %COMPOSE_FILE%
echo    首次启动可能需要几秒钟
echo    如果窗口没有弹出，请检查：
echo    1. Docker Desktop 是否正在运行
echo    2. VcXsrv 是否已启动并禁用访问控制
echo    3. 防火墙是否允许 Docker 访问网络
echo.

REM 启动容器
%COMPOSE_CMD% -f %COMPOSE_FILE% up --force-recreate

echo.
echo =========================================
echo    应用程序已关闭
echo =========================================
echo.
echo 提示：
echo    - 数据保存在 .\data 目录
echo    - 重新运行: 双击 run.bat
echo    - 仅启动后端: %COMPOSE_CMD% -f %COMPOSE_FILE% up -d
echo    - 查看日志: %COMPOSE_CMD% -f %COMPOSE_FILE% logs -f
echo    - 当前使用配置: %COMPOSE_FILE%
echo.
pause