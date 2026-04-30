#!/usr/bin/env bash
# 从源码编译安装 Drogon（MySQL ORM + Redis），安装到指定前缀（默认 /usr/local）。
# 用法: bash scripts/install-drogon.sh [安装前缀，例如 /opt/toide/env 或 /usr/local]
# 无需 root，除非前缀需要写权限（/usr/local 需 sudo）。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="${1:-/usr/local}"
DROGON_TAG="${DROGON_TAG:-v1.9.6}"
WORK="${WORK:-${ROOT}/.third_party/drogon-src}"

mkdir -p "$(dirname "${WORK}")"

if [[ ! -d "${WORK}/.git" ]]; then
  git clone --recurse-submodules https://github.com/drogonframework/drogon.git "${WORK}"
  git -C "${WORK}" checkout "${DROGON_TAG}"
else
  echo ">>> 已存在 ${WORK}，跳过 clone（如需重装请删除该目录）"
fi

BUILD="${WORK}/build-toide"
cmake -S "${WORK}" -B "${BUILD}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_CTL=OFF \
  -DBUILD_ORM=ON \
  -DBUILD_MYSQL=ON \
  -DBUILD_REDIS=ON \
  -DBUILD_SQLITE=OFF \
  -DBUILD_POSTGRESQL=OFF

cmake --build "${BUILD}" -j "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
if [[ -w "${PREFIX}" ]]; then
  cmake --install "${BUILD}"
else
  echo ">>> 安装到 ${PREFIX} 需要提升权限"
  sudo cmake --install "${BUILD}"
  if command -v ldconfig >/dev/null 2>&1; then
    sudo ldconfig "${PREFIX}/lib" 2>/dev/null || true
  fi
fi

if command -v ldconfig >/dev/null 2>&1 && [[ -w "${PREFIX}" ]]; then
  ldconfig "${PREFIX}/lib" 2>/dev/null || true
fi

echo ">>> Drogon 已安装到: ${PREFIX}"
echo "    编译 Toide 时请加: -DCMAKE_PREFIX_PATH=${PREFIX}"
echo "    运行前可将库路径加入环境: export LD_LIBRARY_PATH=${PREFIX}/lib:\${LD_LIBRARY_PATH}"
