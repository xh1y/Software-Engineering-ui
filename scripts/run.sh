#!/bin/bash
# ============================================================
# OptiKG Docker 容器运行脚本
# 功能：启动 OptiKG Docker 容器，映射 8080 端口
# 位置：scripts/run.sh
# 用法：./scripts/run.sh
# 前提：需要先通过 build.sh 或 build-image.sh 构建好镜像
# ============================================================

docker run --rm -p 8080:8080 optikg:latest
