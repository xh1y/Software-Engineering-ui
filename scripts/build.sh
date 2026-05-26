#!/bin/bash
# ============================================================
# OptiKG Docker 镜像构建脚本（使用宿主机网络）
# 功能：在项目根目录上下文中构建 Docker 镜像
# 位置：scripts/build.sh
# 用法：./scripts/build.sh
# ============================================================

# 项目根目录（脚本位于 scripts/ 子目录，上级即为项目根目录）
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# 使用宿主机网络构建（解决某些网络环境下下载依赖慢的问题）
docker build --network=host \
  -f "$PROJECT_ROOT/Dockerfile" \
  -t optikg "$PROJECT_ROOT"
