#!/usr/bin/env bash
# 增量编译（Release）并前台启动 alarmsys；无需任何参数。
# 环境变量: FASTDDS_ROOT（默认 /usr/local/fastdds3）
#          ALARMSYS_USE_SYSTEM_QT=0 时不强制系统 Qt5_DIR
#          SKIP_BUILD=1 时跳过编译仅启动
#          ALARMSYS_ENABLE_COREDUMP_GDB 默认 1：通过 run_with_coredump_gdb.sh 启动并在崩溃后自动导出回溯

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN_DIR="${SCRIPT_DIR}/bin"
EXE="${BIN_DIR}/alarmsys"
FASTDDS_ROOT="${FASTDDS_ROOT:-/usr/local/fastdds3}"

if pgrep -x alarmsys >/dev/null 2>&1; then
  pkill -x alarmsys 2>/dev/null || true
  sleep 0.5
  pkill -9 -x alarmsys 2>/dev/null || true
  sleep 0.2
fi

if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
  echo "========== 增量编译 (Release) =========="
  mkdir -p "${BUILD_DIR}"
  cd "${BUILD_DIR}"
  if [[ "${ALARMSYS_USE_SYSTEM_QT:-1}" != "0" ]]; then
    _sysqt="/usr/lib/x86_64-linux-gnu/cmake/Qt5"
    if [[ -f "${_sysqt}/Qt5Config.cmake" && -f CMakeCache.txt ]]; then
      if grep -qE 'Qt5Core_DIR:.*(anaconda|miniconda|mambaforge)|Qt5_DIR:.*(anaconda|miniconda|mambaforge)' CMakeCache.txt 2>/dev/null; then
        echo "清理 conda Qt 的 CMake 缓存，改用系统 Qt..."
        rm -f CMakeCache.txt
        rm -rf CMakeFiles
      fi
    fi
  fi
  _cm=(.. -DCMAKE_BUILD_TYPE=Release -DFASTDDS_ROOT="${FASTDDS_ROOT}")
  if [[ "${ALARMSYS_USE_SYSTEM_QT:-1}" != "0" ]]; then
    _sysqt="/usr/lib/x86_64-linux-gnu/cmake/Qt5"
    [[ -f "${_sysqt}/Qt5Config.cmake" ]] && _cm+=(-DQt5_DIR="${_sysqt}")
  fi
  cmake "${_cm[@]}"
  cmake --build . -j"$(nproc)"
  if [[ ! -x "${EXE}" ]]; then
    echo "错误: 未生成 ${EXE}"
    exit 1
  fi
  echo "编译完成: ${EXE}"
else
  echo "SKIP_BUILD=1，跳过编译"
fi

if [[ ! -f "${BIN_DIR}/Config.ini" ]]; then
  echo "错误: ${BIN_DIR}/Config.ini 不存在（需先成功编译过）"
  exit 1
fi

echo "========== 启动 alarmsys（Ctrl+C 结束）=========="
export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:${FASTDDS_ROOT}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "${BIN_DIR}"
RUN_WRAP="${SCRIPT_DIR}/run_with_coredump_gdb.sh"
LOG="${ALARMSYS_LOG:-${BIN_DIR}/alarmsys.log}"
if [[ "${ALARMSYS_ENABLE_COREDUMP_GDB:-1}" == "1" && -x "${RUN_WRAP}" ]]; then
  exec "${RUN_WRAP}" --log "${LOG}" -- ./alarmsys
fi
exec ./alarmsys
