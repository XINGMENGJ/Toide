#!/usr/bin/env bash
set -euo pipefail

need_new_cmake() {
  if ! command -v cmake >/dev/null 2>&1; then
    return 0
  fi
  local ver major minor
  ver="$(cmake --version | head -n1 | awk '{print $3}')"
  major="${ver%%.*}"
  rest="${ver#*.}"
  minor="${rest%%.*}"
  if [[ "${major}" -gt 3 ]]; then
    return 1
  fi
  if [[ "${major}" -eq 3 ]] && [[ "${minor}" -ge 24 ]]; then
    return 1
  fi
  return 0
}

if [[ "${EUID}" -ne 0 ]]; then
  echo "请使用 root 运行（例如: sudo $0）"
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update -y
apt-get install -y \
  build-essential \
  git \
  pkg-config \
  ninja-build \
  ca-certificates \
  curl \
  gnupg \
  libssl-dev \
  zlib1g-dev \
  libjsoncpp-dev \
  libhiredis-dev \
  libmysqlclient-dev

if [[ -f /etc/os-release ]]; then
  # shellcheck source=/dev/null
  . /etc/os-release
  if [[ "${ID:-}" == "ubuntu" ]] && need_new_cmake; then
    echo ">>> 添加 Kitware APT 源以安装 CMake 3.24+ （${VERSION_CODENAME:-unknown}）"
    curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc \
      | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ ${VERSION_CODENAME} main" \
      > /etc/apt/sources.list.d/kitware.list
    apt-get update -y
  fi
fi

apt-get install -y --no-install-recommends cmake
cmake --version

echo ">>> 系统依赖已就绪。下一步: bash scripts/install-drogon.sh"
