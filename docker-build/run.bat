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
echo    如果中途失败，请检查网络连接后重试
echo.

REM 构建镜像
%COMPOSE_CMD% build --progress plain
if %errorlevel% neq 0 (
    echo.
    echo ⚠️  构建失败，尝试不带缓存重新构建...
    echo.
    %COMPOSE_CMD% build --no-cache --progress plain
    if %errorlevel% neq 0 (
        echo.
        echo ❌ Docker 构建失败，请检查错误信息
        pause
        exit /b 1
    )
)

echo.
echo ✅ 构建成功！启动应用程序...
echo    首次启动可能需要几秒钟
echo    如果窗口没有弹出，请检查：
echo    1. Docker Desktop 是否正在运行
echo    2. VcXsrv 是否已启动并禁用访问控制
echo    3. 防火墙是否允许 Docker 访问网络
echo.

REM 启动容器
%COMPOSE_CMD% up --force-recreate

echo.
echo =========================================
echo    应用程序已关闭
echo =========================================
echo.
echo 提示：
echo    - 数据保存在 .\data 目录
echo    - 重新运行: 双击 run.bat
echo    - 仅启动后端: %COMPOSE_CMD% up -d
echo    - 查看日志: %COMPOSE_CMD% logs -f
echo.
pause