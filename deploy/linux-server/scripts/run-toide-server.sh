#!/usr/bin/env bash
# 启动 Toide HTTP/WebSocket 服务（监听见 server/config/server.json）。
# 工作目录为部署包根目录，以便找到 server/config/server.json。
# 用法:
#   bash scripts/run-toide-server.sh
# 可选:
#   TOIDE_SERVER_CONFIG=/绝对路径/server.json
#   LD_LIBRARY_PATH   - 若 Drogon 装在自定义前缀，请包含其 lib
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"

BUILD_DIR="${BUILD_DIR:-${ROOT}/build}"
EXE="${BUILD_DIR}/server/toide_server"

if [[ ! -x "${EXE}" ]]; then
  echo "未找到 ${EXE}，请先执行: bash scripts/build-toide-server.sh"
  exit 1
fi

export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
# 常见本机前缀（install-drogon 默认 /usr/local 一般不必设）
for P in /usr/local/lib /opt/toide/env/lib; do
  if [[ -d "${P}" ]]; then
    case ":${LD_LIBRARY_PATH}:" in
      *":${P}:"*) ;;
      *) LD_LIBRARY_PATH="${P}:${LD_LIBRARY_PATH}" ;;
    esac
  fi
done
export LD_LIBRARY_PATH

exec "${EXE}" "$@"
