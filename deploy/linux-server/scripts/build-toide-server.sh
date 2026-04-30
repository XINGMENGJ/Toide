#!/usr/bin/env bash
# 在部署包根目录编译 toide_server。
# 用法（在包含 server/ 与 CMakeLists.txt 的目录下）:
#   bash scripts/build-toide-server.sh
# 可选环境变量:
#   CMAKE_PREFIX_PATH - Drogon 等非系统前缀，多个用分号或冒号分隔（CMake 规则）
#   BUILD_DIR         - 默认 build
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"

if [[ ! -f "${ROOT}/CMakeLists.txt" ]] || [[ ! -d "${ROOT}/server" ]]; then
  echo "错误: 请在部署包根目录运行（需要 ./CMakeLists.txt 与 ./server/）。当前: ${ROOT}"
  exit 1
fi

if [[ ! -f "${ROOT}/server/config/server.json" ]]; then
  if [[ -f "${ROOT}/server/config/server.json.example" ]]; then
    echo ">>> 从 server.json.example 创建 server/config/server.json（请按需修改数据库 Redis 地址）"
    cp -n "${ROOT}/server/config/server.json.example" "${ROOT}/server/config/server.json"
  else
    echo "错误: 缺少 server/config/server.json，且无 example 可参考。"
    exit 1
  fi
fi

BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
: "${CMAKE_PREFIX_PATH:=}"

CFG=(
  -S "${ROOT}"
  -B "${BUILD_DIR}"
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
)

if [[ -n "${CMAKE_PREFIX_PATH}" ]]; then
  CFG+=("-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
fi

cmake "${CFG[@]}"
cmake --build "${BUILD_DIR}" -j "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)" --target toide_server

echo ">>> 可执行文件: ${BUILD_DIR}/server/toide_server"
echo ">>> 运行: bash scripts/run-toide-server.sh"
