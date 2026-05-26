#!/bin/bash
# ============================================================
# OptiKG Docker 镜像构建与导出脚本
# 功能：构建 Docker 镜像并导出为 tar.gz 压缩包
# 位置：scripts/build-image.sh
# 用法：./scripts/build-image.sh
# ============================================================

# 项目根目录（脚本位于 scripts/ 子目录，上级即为项目根目录）
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# 在项目根目录上下文中构建 Docker 镜像
docker build -t optikg "$PROJECT_ROOT"

# 导出镜像为压缩包（输出到项目根目录）
docker save optikg:latest | gzip > "$PROJECT_ROOT/optikg.tar.gz"

# 运行镜像的命令（供参考）:
# docker run -p 8080:8080 optikg:latest
