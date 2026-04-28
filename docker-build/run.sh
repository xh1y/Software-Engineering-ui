#!/bin/bash
# OptiKG Docker 启动脚本 (Linux/Mac)
# 使用方法：./run.sh 或双击此文件

set -e

echo "========================================="
echo "   OptiKG 信息抽取工具 Docker 启动脚本   "
echo "========================================="

# 检查 Docker 是否安装
if ! command -v docker &> /dev/null; then
    echo "❌ 未检测到 Docker，请先安装 Docker Desktop"
    echo "   下载地址：https://www.docker.com/products/docker-desktop"
    exit 1
fi

# 检查 Docker Compose
if ! command -v docker-compose &> /dev/null; then
    echo "⚠️  未检测到 docker-compose，尝试使用 docker compose"
    if ! docker compose version &> /dev/null; then
        echo "❌ 请安装 Docker Compose (V2)"
        exit 1
    else
        COMPOSE_CMD="docker compose"
    fi
else
    COMPOSE_CMD="docker-compose"
fi

echo "✅ Docker 环境检测通过"

# 检测操作系统
OS_TYPE="unknown"
case "$(uname -s)" in
    Linux*)     OS_TYPE="linux";;
    Darwin*)    OS_TYPE="mac";;
    CYGWIN*|MINGW*|MSYS*) OS_TYPE="windows";;
    *)          OS_TYPE="unknown";;
esac

echo "📱 操作系统: $OS_TYPE"

# 设置 X11 相关环境
if [ "$OS_TYPE" = "linux" ]; then
    echo "🐧 Linux 系统检测"
    if [ -z "$DISPLAY" ]; then
        echo "❌ 未检测到 DISPLAY 环境变量"
        echo "   请确保在图形界面下运行此脚本"
        exit 1
    fi

    # 允许 Docker 容器连接 X11
    xhost +local:docker > /dev/null 2>&1
    echo "✅ 已允许 Docker 访问 X11 显示"

    # 设置权限
    if [ ! -f "${HOME}/.Xauthority" ]; then
        touch "${HOME}/.Xauthority"
    fi
    chmod 600 "${HOME}/.Xauthority"

    # 设置 privileged 模式（某些 Linux 发行版需要）
    export PRIVILEGED=true

elif [ "$OS_TYPE" = "mac" ]; then
    echo "🍎 macOS 系统检测"

    # 检查是否已安装 XQuartz
    if ! brew list --formula | grep -q xquartz 2>/dev/null && [ ! -d /opt/X11 ]; then
        echo "⚠️  未检测到 XQuartz，尝试启动..."
        if [ -f /opt/X11/bin/xhost ]; then
            /opt/X11/bin/xhost +localhost
        else
            echo "❌ 请先安装 XQuartz: https://www.xquartz.org/"
            echo "   安装后重启电脑，并确保 XQuartz 正在运行"
            exit 1
        fi
    fi

    # macOS 的 DISPLAY 通常为 :0
    export DISPLAY=host.docker.internal:0
    echo "✅ 设置 DISPLAY=$DISPLAY"

    # 允许本地连接
    if command -v xhost &> /dev/null; then
        xhost +localhost > /dev/null 2>&1
    fi

    # 不需要 privileged 模式
    export PRIVILEGED=false

elif [ "$OS_TYPE" = "windows" ]; then
    echo "🪟 Windows 系统检测"
    echo "⚠️  请确保已安装 VcXsrv 或 X11 服务器"
    echo "   下载 VcXsrv: https://sourceforge.net/projects/vcxsrv/"
    echo ""
    echo "启动 VcXsrv 时请选择："
    echo "1. Multiple windows"
    echo "2. Display number: 0"
    echo "3. 勾选 'Disable access control'"
    echo "4. 其他选项默认"
    echo ""
    read -p "按回车键继续（确保 VcXsrv 正在运行）..."

    # Windows 下的 DISPLAY
    export DISPLAY=host.docker.internal:0
    export PRIVILEGED=false

else
    echo "⚠️  未知操作系统，尝试通用配置"
    export DISPLAY=:0
    export PRIVILEGED=false
fi

# 进入脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# 检查 Docker 网络状态
echo ""
echo "🔍 检查 Docker 网络状态..."
if ! docker network ls &> /dev/null; then
    echo "⚠️  Docker 网络访问异常，尝试修复..."
    echo "   如果是 Linux 系统，请尝试: sudo systemctl restart docker"
    echo "   或执行: docker network prune"
fi

echo ""
echo "🔨 开始构建 Docker 镜像（首次运行较慢，约5-10分钟）..."
echo "   如果中途失败，会自动尝试简化版构建"
echo ""

# 函数：尝试构建
try_build() {
    local compose_file=$1
    local description=$2

    echo ""
    echo "🔄 尝试使用 $description 构建..."
    echo "   使用的配置文件: $compose_file"
    echo ""

    if $COMPOSE_CMD -f "$compose_file" build --progress plain; then
        echo "✅ $description 构建成功！"
        return 0
    else
        echo "⚠️  $description 构建失败"
        return 1
    fi
}

# 首先尝试简化版构建（更可靠）
BUILD_SUCCESS=false
COMPOSE_FILE="docker-compose.simple.yml"

if try_build "$COMPOSE_FILE" "简化版配置"; then
    BUILD_SUCCESS=true
else
    echo ""
    echo "🔄 简化版构建失败，尝试原版构建（可能需要更多依赖）..."
    COMPOSE_FILE="docker-compose.yml"

    if try_build "$COMPOSE_FILE" "原版配置"; then
        BUILD_SUCCESS=true
    else
        echo ""
        echo "❌ 所有构建尝试都失败了"
        echo ""
        echo "🔥 常见问题解决方案："
        echo "   1. Docker 网络问题: sudo systemctl restart docker (Linux)"
        echo "   2. 清理 Docker: docker system prune -a"
        echo "   3. 检查磁盘空间: df -h"
        echo "   4. 增加 Docker 内存（Windows/Mac Docker Desktop 设置）"
        echo "   5. 关闭 VPN 或代理"
        echo ""
        echo "📋 手动构建命令:"
        echo "   cd $SCRIPT_DIR"
        echo "   docker build -f Dockerfile.simple -t optikg .."
        echo "   docker run -e DISPLAY=\$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix optikg"
        echo ""
        exit 1
    fi
fi

echo ""
echo "✅ 构建成功！启动应用程序..."
echo "   使用的配置: $COMPOSE_FILE"
echo "   首次启动可能需要几秒钟"
echo "   如果窗口没有弹出，请检查 Docker Desktop 是否运行"
echo ""

# 启动容器
$COMPOSE_CMD -f "$COMPOSE_FILE" up --force-recreate

echo ""
echo "========================================="
echo "   应用程序已关闭"
echo "========================================="

# 清理（仅 Linux）
if [ "$OS_TYPE" = "linux" ]; then
    echo "🧹 清理 X11 权限..."
    xhost -local:docker > /dev/null 2>&1
fi

echo "✅ 脚本执行完毕"
echo ""
echo "提示："
echo "   - 数据保存在 ./data 目录"
echo "   - 重新运行: ./run.sh"
echo "   - 仅启动后端: $COMPOSE_CMD -f $COMPOSE_FILE up -d"
echo "   - 查看日志: $COMPOSE_CMD -f $COMPOSE_FILE logs -f"
echo "   - 当前使用配置: $COMPOSE_FILE"
echo ""