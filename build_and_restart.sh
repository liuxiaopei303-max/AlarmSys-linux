#!/usr/bin/env bash
# AlarmSys-linux：停止 alarmsys（优先 systemd，再清理残留）→ 编译 → 启动（有 unit 则 systemctl，避免双实例）
# 用法: ./build_and_restart.sh
# 可选环境变量:
#   FASTDDS_ROOT   默认 /usr/local/fastdds3
#   ALARMSYS_LOG   日志文件路径，默认 bin/alarmsys.log
#   RESTART=0      仅编译，不启动
#   ALARMSYS_ENABLE_COREDUMP_GDB 默认 1：通过 run_with_coredump_gdb.sh 启动并在崩溃后自动导出回溯
#   ALARMSYS_RUN_UNDER_GDB=1  由 gdb 直接 run 子进程（无磁盘 core 也能在 bin/crash/gdb-inline-*.log 里拿到栈；略慢）
#   ALARMSYS_FOREGROUND=1  前台运行（仍用同一套 LD_LIBRARY_PATH），便于调试；不走 systemd 启动
#   ALARMSYS_USE_SYSTEMD=0  强制不用 systemd，编译后用 nohup 手动后台启动
#   ALARMSYS_USE_SYSTEM_QT  默认 1：强制使用 /usr 下 Qt5（与脚本 LD_LIBRARY_PATH 一致），见 quick_build_start.sh 注释

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${SCRIPT_DIR}/bin"
EXE="${BIN_DIR}/alarmsys"
FASTDDS_ROOT="${FASTDDS_ROOT:-/usr/local/fastdds3}"
SERVICE_NAME="alarmsys.service"
RESTART="${RESTART:-1}"
STOP_WAIT_SEC="${ALARMSYS_STOP_WAIT_SEC:-2}"

alarmsys_systemctl() {
  if [[ "$(id -u)" -eq 0 ]]; then
    systemctl "$@"
  else
    sudo systemctl "$@"
  fi
}

alarmsys_has_systemd_unit() {
  systemctl list-unit-files "${SERVICE_NAME}" >/dev/null 2>&1
}

alarmsys_use_systemd() {
  [[ "${ALARMSYS_USE_SYSTEMD:-1}" != "0" ]] \
    && [[ "${ALARMSYS_FOREGROUND:-0}" != "1" ]] \
    && [[ "${RESTART}" == "1" ]] \
    && alarmsys_has_systemd_unit
}

# 收集 PID：精确名、bin/alarmsys 路径、gdb 包装脚本子进程
alarmsys_collect_pids() {
  local p
  for p in $(pgrep -x alarmsys 2>/dev/null); do echo "$p"; done
  for p in $(pgrep -f '[/]bin/alarmsys' 2>/dev/null); do echo "$p"; done
  for p in $(pgrep -f 'run_with_coredump_gdb\.sh.*alarmsys' 2>/dev/null); do echo "$p"; done
  for p in $(pgrep -f "${BIN_DIR}/alarmsys" 2>/dev/null); do echo "$p"; done
}

alarmsys_stop_one() {
  local pid="$1" sig="$2"
  if [[ -z "${pid}" ]] || ! kill -0 "${pid}" 2>/dev/null; then
    return 0
  fi
  kill "-${sig}" "${pid}" 2>/dev/null || true
}

alarmsys_show_stuck() {
  echo "仍存活的 alarmsys 相关进程:"
  local pid stat comm cmd
  while read -r pid; do
    [[ -z "${pid}" ]] && continue
    stat="$(awk '{print $3}' "/proc/${pid}/stat" 2>/dev/null || echo '?')"
    comm="$(cat "/proc/${pid}/comm" 2>/dev/null || echo '?')"
    cmd="$(tr '\0' ' ' < "/proc/${pid}/cmdline" 2>/dev/null || echo '?')"
    echo "  PID=${pid} STAT=${stat} COMM=${comm} CMD=${cmd}"
  done < <(alarmsys_collect_pids | sort -u)
}

alarmsys_kill_leftovers() {
  mapfile -t _alarmsys_pids < <(alarmsys_collect_pids | sort -u)
  if [[ ${#_alarmsys_pids[@]} -eq 0 ]]; then
    return 0
  fi
  echo "清理残留进程 ${#_alarmsys_pids[@]} 个: ${_alarmsys_pids[*]}"
  local pid
  for pid in "${_alarmsys_pids[@]}"; do alarmsys_stop_one "${pid}" TERM; done
  sleep 1
  mapfile -t _alarmsys_pids < <(alarmsys_collect_pids | sort -u)
  if [[ ${#_alarmsys_pids[@]} -gt 0 ]]; then
    echo "仍在运行，发送 SIGKILL..."
    for pid in "${_alarmsys_pids[@]}"; do alarmsys_stop_one "${pid}" KILL; done
    sleep 0.5
  fi
  mapfile -t _alarmsys_pids < <(alarmsys_collect_pids | sort -u)
  if [[ ${#_alarmsys_pids[@]} -gt 0 ]]; then
    alarmsys_show_stuck
    if [[ "${ALARMSYS_FORCE_BUILD:-0}" == "1" ]]; then
      echo "警告: ALARMSYS_FORCE_BUILD=1，跳过停进程继续编译（无法覆盖正在运行的旧二进制）。"
      return 0
    fi
    echo "错误: 无法结束 alarmsys（常见于 STAT=D 不可中断 I/O，或进程在容器/其他命名空间）。"
    echo "可尝试: kill -9 <PID>  或  ALARMSYS_FORCE_BUILD=1 ./build_and_restart.sh"
    exit 1
  fi
  echo "残留进程已清理."
}

alarmsys_verify_single_instance() {
  mapfile -t _alarmsys_pids < <(alarmsys_collect_pids | sort -u)
  local n="${#_alarmsys_pids[@]}"
  if [[ "${n}" -le 2 ]]; then
    echo "当前相关进程 ${n} 个（wrapper + alarmsys 为正常）: ${_alarmsys_pids[*]:-无}"
    return 0
  fi
  echo "警告: 仍有 ${n} 个相关进程，可能存在双实例: ${_alarmsys_pids[*]}"
  alarmsys_show_stuck
  return 1
}

echo "========== 1. 停止 alarmsys =========="
_before_pids="$(alarmsys_collect_pids | sort -u | xargs echo -n)"
if [[ -z "${_before_pids// }" ]]; then
  echo "当前无 alarmsys 进程."
else
  echo "发现 $(wc -w <<<"${_before_pids}") 个相关进程: ${_before_pids}"
fi

if alarmsys_has_systemd_unit; then
  if alarmsys_systemctl is-active --quiet "${SERVICE_NAME}" 2>/dev/null; then
    echo "停止 systemd 单元 ${SERVICE_NAME}（避免 Restart=always 与手动 nohup 双实例）..."
    alarmsys_systemctl stop "${SERVICE_NAME}"
    sleep "${STOP_WAIT_SEC}"
  else
    echo "${SERVICE_NAME} 未在运行."
  fi
else
  echo "未检测到 ${SERVICE_NAME}，仅清理手动/nohup 进程."
fi

alarmsys_kill_leftovers
echo "已停止 alarmsys."

echo ""
echo "========== 2. 编译 =========="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
if [[ "${ALARMSYS_USE_SYSTEM_QT:-1}" != "0" ]]; then
  _sysqt="/usr/lib/x86_64-linux-gnu/cmake/Qt5"
  if [[ -f "${_sysqt}/Qt5Config.cmake" && -f CMakeCache.txt ]]; then
    if grep -qE 'Qt5Core_DIR:.*(anaconda|miniconda|mambaforge)|Qt5_DIR:.*(anaconda|miniconda|mambaforge)' CMakeCache.txt 2>/dev/null; then
      echo "清理仍指向 conda 的 Qt CMake 缓存（改用系统 Qt）..."
      rm -f CMakeCache.txt
      rm -rf CMakeFiles
    fi
  fi
fi
_cm=(.. -DCMAKE_BUILD_TYPE=Release -DFASTDDS_ROOT="${FASTDDS_ROOT}")
if [[ "${ALARMSYS_USE_SYSTEM_QT:-1}" != "0" ]]; then
  _sysqt="/usr/lib/x86_64-linux-gnu/cmake/Qt5"
  if [[ -f "${_sysqt}/Qt5Config.cmake" ]]; then
    _cm+=(-DQt5_DIR="${_sysqt}")
  fi
fi
if [[ -f CMakeCache.txt ]]; then
  CACHED_SRC="$(grep '^CMAKE_HOME_DIRECTORY:INTERNAL=' CMakeCache.txt 2>/dev/null | cut -d= -f2-)"
  if [[ -n "${CACHED_SRC}" && "${CACHED_SRC}" != "${SCRIPT_DIR}" ]]; then
    echo "源码目录与缓存不一致，重新配置..."
    rm -f CMakeCache.txt
    rm -rf CMakeFiles
  fi
fi
cmake "${_cm[@]}"
cmake --build . -j"$(nproc)"

if [[ ! -x "${EXE}" ]]; then
  echo "错误: 编译后仍不可执行: ${EXE}"
  exit 1
fi
echo "编译完成: ${EXE}"

if [[ "${RESTART}" != "1" ]]; then
  echo ""
  echo "RESTART=0，跳过启动."
  echo "========== 完成 =========="
  exit 0
fi

echo ""
echo "========== 3. 启动 alarmsys =========="
# 与 ThirdPartyManager 一致：优先系统 libstdc++/Qt，避免 Anaconda 抢库
export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:/usr/local/fastdds3/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

cd "${BIN_DIR}"
LOG="${ALARMSYS_LOG:-${BIN_DIR}/alarmsys.log}"
RUN_WRAP="${SCRIPT_DIR}/run_with_coredump_gdb.sh"
ENABLE_COREDUMP_GDB="${ALARMSYS_ENABLE_COREDUMP_GDB:-1}"

if [[ "${ALARMSYS_FOREGROUND:-0}" == "1" ]]; then
  echo "前台运行（Ctrl+C 结束）；工作目录: ${BIN_DIR}"
  if [[ "${ENABLE_COREDUMP_GDB}" == "1" && -x "${RUN_WRAP}" ]]; then
    exec "${RUN_WRAP}" --log "${LOG}" -- ./alarmsys
  fi
  exec ./alarmsys
fi

if alarmsys_use_systemd; then
  echo "通过 systemd 启动 ${SERVICE_NAME}..."
  alarmsys_systemctl start "${SERVICE_NAME}"
  sleep 0.5
  if ! alarmsys_systemctl is-active --quiet "${SERVICE_NAME}" 2>/dev/null; then
    echo "错误: ${SERVICE_NAME} 未处于 active，请检查:"
    echo "  journalctl -u ${SERVICE_NAME} -n 50 --no-pager"
    exit 1
  fi
  MAIN_PID="$(alarmsys_systemctl show -p MainPID --value "${SERVICE_NAME}" 2>/dev/null || echo 0)"
  echo "已启动 ${SERVICE_NAME} MainPID=${MAIN_PID}"
  echo "日志: ${LOG}  或  journalctl -u ${SERVICE_NAME} -f"
  alarmsys_systemctl status "${SERVICE_NAME}" --no-pager || true
else
  if alarmsys_has_systemd_unit; then
    echo "ALARMSYS_USE_SYSTEMD=0：使用 nohup 手动后台启动（请确保未同时 systemctl start）"
  else
    echo "未检测到 ${SERVICE_NAME}，使用 nohup 后台启动"
  fi
  if [[ "${ENABLE_COREDUMP_GDB}" == "1" && -x "${RUN_WRAP}" ]]; then
    nohup "${RUN_WRAP}" --log "${LOG}" -- ./alarmsys >>"${LOG}" 2>&1 &
  else
    nohup ./alarmsys >>"${LOG}" 2>&1 &
  fi
  NEW_PID=$!
  sleep 0.4
  if kill -0 "${NEW_PID}" 2>/dev/null; then
    echo "已后台启动 PID=${NEW_PID}"
    echo "日志文件: ${LOG}"
    echo "查看: tail -f ${LOG}"
  else
    echo "进程已退出，请查看日志: ${LOG}"
    tail -n 40 "${LOG}" 2>/dev/null || true
    exit 1
  fi
fi

alarmsys_verify_single_instance || true

echo ""
echo "========== 完成 =========="
