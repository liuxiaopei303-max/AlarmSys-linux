#!/usr/bin/env bash
# 运行 alarmsys 并在崩溃后自动导出 gdb 回溯
# 用法:
#   ./run_with_coredump_gdb.sh [--log /path/to/log] [--] [program [args...]]
# 示例:
#   ./run_with_coredump_gdb.sh --log ./alarmsys.log -- ./alarmsys
#
# 环境变量:
#   ALARMSYS_CRASH_DIR   崩溃输出目录，默认 <bin>/crash
#   ALARMSYS_RUN_UNDER_GDB=1  始终在 gdb 下启动子进程；段错误时无需磁盘 core 即可在 crash 目录写回溯
#                             （略慢，适合 core_pattern 被管道到 systemd 且无 coredumpctl 的环境）

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE=""

if [[ "${1:-}" == "--log" ]]; then
  LOG_FILE="${2:-}"
  shift 2 || true
fi

if [[ "${1:-}" == "--" ]]; then
  shift
fi

PROGRAM="${1:-./alarmsys}"
shift || true
PROGRAM_ARGS=("$@")

BIN_DIR="$(cd "$(dirname "${PROGRAM}")" 2>/dev/null && pwd)"
if [[ -z "${BIN_DIR}" ]]; then
  BIN_DIR="${SCRIPT_DIR}/bin"
fi

# 可执行文件绝对路径（供 gdb / 找 core）
if [[ "${PROGRAM}" == /* ]]; then
  ABS_PROGRAM="${PROGRAM}"
else
  ABS_PROGRAM="$(cd "${BIN_DIR}" 2>/dev/null && pwd)/$(basename "${PROGRAM}")"
fi

CRASH_DIR="${ALARMSYS_CRASH_DIR:-${BIN_DIR}/crash}"
mkdir -p "${CRASH_DIR}"

timestamp() { date +"%Y-%m-%d %H:%M:%S"; }

echo "[run_with_coredump_gdb] $(timestamp) 启动: ${PROGRAM} ${PROGRAM_ARGS[*]}"
echo "[run_with_coredump_gdb] 崩溃输出目录: ${CRASH_DIR}"
if [[ -r /proc/sys/kernel/core_pattern ]]; then
  echo "[run_with_coredump_gdb] kernel.core_pattern=$(tr -d '\n' </proc/sys/kernel/core_pattern)"
  if grep -q '^|' /proc/sys/kernel/core_pattern 2>/dev/null; then
    echo "[run_with_coredump_gdb] 提示: core 被管道到外部程序，bin 目录下通常不会出现 core 文件；可安装 systemd-coredump 用 coredumpctl，或临时: sudo sysctl -w kernel.core_pattern=${CRASH_DIR}/core.%e.%p.%t"
  fi
fi

# 用于 find -newer 定位本次运行产生的 core
RUN_MARKER="${CRASH_DIR}/.run_marker_$$"
touch "${RUN_MARKER}" 2>/dev/null || true

# 尽最大努力开启 core dump（受系统 core_pattern/权限影响）
if ulimit -c unlimited 2>/dev/null; then
  echo "[run_with_coredump_gdb] core dump 已开启 (ulimit -c unlimited)"
else
  echo "[run_with_coredump_gdb] 警告: 无法设置 ulimit -c unlimited"
fi

run_inferior_direct() {
  if [[ -n "${LOG_FILE}" ]]; then
    "${PROGRAM}" "${PROGRAM_ARGS[@]}" >>"${LOG_FILE}" 2>&1
  else
    "${PROGRAM}" "${PROGRAM_ARGS[@]}"
  fi
}

# 在 gdb 内 run：段错误后执行 bt，不依赖磁盘 core（适合容器/管道 core_pattern）
run_inferior_under_gdb() {
  local bt_path="$1"
  {
    echo "[run_with_coredump_gdb] $(timestamp) ALARMSYS_RUN_UNDER_GDB=1：gdb 启动子进程，完整输出见本文件与 alarmsys 日志"
    gdb -q --batch --return-child-result \
      -ex "set pagination off" \
      -ex "set confirm off" \
      -ex "run" \
      -ex "thread apply all bt full" \
      -ex "info registers" \
      -ex "quit" \
      --args "${ABS_PROGRAM}" "${PROGRAM_ARGS[@]}"
  } 2>&1 | if [[ -n "${LOG_FILE}" ]]; then
    tee -a "${bt_path}" >>"${LOG_FILE}"
  else
    tee -a "${bt_path}"
  fi
  return "${PIPESTATUS[0]}"
}

if [[ "${ALARMSYS_RUN_UNDER_GDB:-0}" == "1" ]] && command -v gdb >/dev/null 2>&1; then
  bt_early="${CRASH_DIR}/gdb-inline-$(date +%Y%m%d_%H%M%S).log"
  run_inferior_under_gdb "${bt_early}"
  rc=$?
  rm -f "${RUN_MARKER}" 2>/dev/null || true
  if [[ "${rc}" -eq 0 ]]; then
    exit 0
  fi
  echo "[run_with_coredump_gdb] 进程异常退出，exit_code=${rc}"
  echo "[run_with_coredump_gdb] gdb 内联回溯见: ${bt_early}"
  exit "${rc}"
fi

run_inferior_direct
rc=$?

if [[ "${rc}" -eq 0 ]]; then
  rm -f "${RUN_MARKER}" 2>/dev/null || true
  exit 0
fi

echo "[run_with_coredump_gdb] 进程异常退出，exit_code=${rc}"

bt_log="${CRASH_DIR}/gdb-backtrace-$(date +%Y%m%d_%H%M%S).log"
core_file=""

# 常见 core 文件命名 + 本次运行时间窗内搜索
for pat in "${BIN_DIR}/core" "${BIN_DIR}/core."* "${BIN_DIR}/core-"* "${BIN_DIR}/alarmsys.core"* "${CRASH_DIR}/core"*; do
  if [[ -f "${pat}" ]]; then
    core_file="${pat}"
    break
  fi
done

if [[ -z "${core_file}" && -f "${RUN_MARKER}" ]]; then
  core_file="$(find "${BIN_DIR}" "${CRASH_DIR}" "${PWD}" -maxdepth 3 \( -name 'core' -o -name 'core.*' -o -name 'core-*' -o -name '*.core' \) -type f -newer "${RUN_MARKER}" -print -quit 2>/dev/null)"
fi

rm -f "${RUN_MARKER}" 2>/dev/null || true

if [[ -n "${core_file}" ]]; then
  echo "[run_with_coredump_gdb] 发现 core 文件: ${core_file}"
  if command -v gdb >/dev/null 2>&1; then
    gdb -q "${ABS_PROGRAM}" "${core_file}" \
      --batch \
      -ex "set pagination off" \
      -ex "thread apply all bt full" \
      -ex "info registers" \
      -ex "quit" >"${bt_log}" 2>&1 || true
    echo "[run_with_coredump_gdb] gdb 回溯已写入: ${bt_log}"
  else
    echo "[run_with_coredump_gdb] 警告: 未安装 gdb，无法导出回溯"
  fi
elif command -v coredumpctl >/dev/null 2>&1; then
  info_log="${CRASH_DIR}/coredumpctl-info-$(date +%Y%m%d_%H%M%S).log"
  coredumpctl -1 info "${PROGRAM##*/}" >"${info_log}" 2>&1 || true
  echo "[run_with_coredump_gdb] 未发现本地 core 文件，已导出 coredumpctl info: ${info_log}"
  if command -v gdb >/dev/null 2>&1; then
    coredumpctl -1 gdb "${PROGRAM}" \
      --batch \
      -ex "set pagination off" \
      -ex "thread apply all bt full" \
      -ex "info registers" \
      -ex "quit" >"${bt_log}" 2>&1 || true
    echo "[run_with_coredump_gdb] coredumpctl gdb 回溯已写入: ${bt_log}"
  fi
else
  echo "[run_with_coredump_gdb] 未找到 core 文件且无 coredumpctl，无法自动回溯"
  if command -v gdb >/dev/null 2>&1; then
    echo "[run_with_coredump_gdb] 请下次用: ALARMSYS_RUN_UNDER_GDB=1 ${0##*/} ... 或在可写目录设置 kernel.core_pattern 后重试"
  fi
fi

exit "${rc}"
