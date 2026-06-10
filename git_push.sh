#!/usr/bin/env bash
# AlarmSys-linux 一键 add / commit / push
# 用法: ./git_push.sh [提交说明]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CAM_GIT_LIB="/mnt/localdata/lp/camServer/_git_lib.sh"
# shellcheck source=/mnt/localdata/lp/camServer/_git_lib.sh
source "$CAM_GIT_LIB"

CRED_FILE="$ROOT/git地址和token"
COMMIT_MSG="${1:-Update AlarmSys-linux $(date '+%Y-%m-%d %H:%M:%S')}"

[[ -f "$CRED_FILE" ]] || { echo "[git_push] 错误: 找不到 $CRED_FILE" >&2; exit 1; }
_git_read_creds "$CRED_FILE"
_git_init_repo "$ROOT"

if ! _git_commit_paths "$COMMIT_MSG" .; then
  rc=$?
  [[ "$rc" -eq 2 ]] && exit 0
  exit "$rc"
fi

_git_push_with_fallback
echo "[git_push] 完成: $DISPLAY_URL"
