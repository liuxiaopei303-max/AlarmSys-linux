#!/usr/bin/env bash
# 安装 alarmsys systemd 服务（开机自启 + 崩溃 Restart=always）
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_NAME="alarmsys.service"
UNIT_SRC="${ROOT}/${SERVICE_NAME}"
UNIT_DST="/etc/systemd/system/${SERVICE_NAME}"
EXE="${ROOT}/bin/alarmsys"

if [[ "$(id -u)" -ne 0 ]]; then
  echo "请使用 root 执行: sudo $0" >&2
  exit 1
fi

resolve_service_user() {
  if [[ -n "${SERVICE_USER:-}" ]]; then
    echo "${SERVICE_USER}"
    return
  fi
  if getent passwd dell &>/dev/null; then
    echo dell
    return
  fi
  echo root
}

SVC_USER="$(resolve_service_user)"
SVC_GROUP="${SERVICE_GROUP:-${SVC_USER}}"

if [[ ! -x "${EXE}" ]]; then
  echo "==> 未找到可执行文件，先编译（不启动）"
  mkdir -p "${ROOT}/build"
  cd "${ROOT}/build"
  FASTDDS_ROOT="${FASTDDS_ROOT:-/usr/local/fastdds3}"
  _cm=(.. -DCMAKE_BUILD_TYPE=Release -DFASTDDS_ROOT="${FASTDDS_ROOT}")
  if [[ -f /usr/lib/x86_64-linux-gnu/cmake/Qt5/Qt5Config.cmake ]]; then
    _cm+=(-DQt5_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5)
  fi
  cmake "${_cm[@]}"
  cmake --build . -j"$(nproc)"
fi

if [[ ! -x "${EXE}" ]]; then
  echo "[error] 缺少 ${EXE}" >&2
  exit 1
fi

chmod +x "${ROOT}/run_with_coredump_gdb.sh" 2>/dev/null || true

echo "==> 安装 unit: ${UNIT_DST}（User=${SVC_USER}）"
sed -e "s|__INSTALL_ROOT__|${ROOT}|g" \
    -e "s|__SERVICE_USER__|${SVC_USER}|g" \
    -e "s|__SERVICE_GROUP__|${SVC_GROUP}|g" \
    "${UNIT_SRC}" >"${UNIT_DST}"
chmod 644 "${UNIT_DST}"

# 避免与手动 nohup 实例冲突
pkill -x alarmsys 2>/dev/null || true
sleep 1

systemctl daemon-reload
systemctl enable "${SERVICE_NAME}"
systemctl restart "${SERVICE_NAME}"

echo "==> 状态:"
systemctl --no-pager status "${SERVICE_NAME}" || true
echo ""
echo "常用命令:"
echo "  sudo systemctl status ${SERVICE_NAME}"
echo "  sudo journalctl -u ${SERVICE_NAME} -f"
echo "  sudo systemctl restart ${SERVICE_NAME}"
