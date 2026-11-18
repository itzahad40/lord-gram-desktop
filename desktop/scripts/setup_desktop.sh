#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
SRC="${1:-$ROOT/build-desktop/tdesktop}"
CFG="${2:-$ROOT/lordgram.config}"
TD_TAG=v6.9.4
TD_COMMIT=8a22f2c106dd3c601e11d124b8d1894613d596fd
PYTHON_BIN="$(command -v python || command -v python3 || true)"
[ -n "$PYTHON_BIN" ] || { echo "Python 3 is required (python3 or python)." >&2; exit 1; }

[ -f "$CFG" ] && . "$CFG"
APP_NAME="${LG_APP_NAME:-Lord Gram}"

if [ ! -d "$SRC/.git" ]; then
  mkdir -p "$SRC"
  git -C "$SRC" init -q
  git -C "$SRC" remote add origin https://github.com/telegramdesktop/tdesktop.git
  git -C "$SRC" fetch --depth 1 origin "$TD_COMMIT"
  git -C "$SRC" checkout -q FETCH_HEAD
  git -C "$SRC" submodule update --init --recursive --depth 1 --jobs 4
fi

TGCALLS="$SRC/Telegram/ThirdParty/tgcalls"
apply_once() {
  local repo="$1"
  local patch="$2"
  local marker="$3"
  if git -C "$repo" apply --check -R "$patch" 2>/dev/null; then
    return
  fi
  if git -C "$repo" apply --check "$patch" 2>/dev/null; then
    git -C "$repo" apply "$patch"
    return
  fi
  if [ -e "$repo/$marker" ]; then
    return
  fi
  echo "Unable to apply patch: $patch" >&2
  exit 1
}

apply_once "$SRC" "$ROOT/desktop/patches/0001-fight-mode-desktop-app.patch" "Telegram/SourceFiles/calls/group/calls_group_fight_mode.h"
apply_once "$SRC" "$ROOT/desktop/patches/0003-lordgram-ui-empty-card-and-fight-button.patch" "Telegram/SourceFiles/window/window_filters_menu.cpp"
apply_once "$TGCALLS" "$ROOT/desktop/patches/0002-fight-mode-desktop-tgcalls.patch" "tgcalls/group/FightModeDsp.h"

"$PYTHON_BIN" "$HERE/rebrand_desktop.py" "$SRC" "$APP_NAME"
"$PYTHON_BIN" "$HERE/install_fight_assets.py" "$SRC" "$ROOT/branding/fightmode_logo.png"
"$PYTHON_BIN" "$HERE/enable_fightmode_all_audio.py" "$SRC"
if [ -f "$ROOT/branding/logo.jpg" ]; then
  "$PYTHON_BIN" "$HERE/make_desktop_icons.py" "$SRC" "$ROOT/branding/logo.jpg"
fi
echo "Desktop setup done: $SRC ($TD_TAG)"
