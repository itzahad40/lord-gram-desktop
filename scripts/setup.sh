#!/usr/bin/env bash
# Lord Gram setup: fetch pinned Telegram, apply patches, rebrand, create signing key.
# Run in Linux / WSL / macOS / Git Bash.   usage: scripts/setup.sh [lordgram.config]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CFG="${1:-$ROOT/lordgram.config}"
SRC="$ROOT/build-src"
TG_COMMIT=3de37c8cfb039452be7398909461da6288babf15
[ -f "$CFG" ] || { echo "Config nahi mila: $CFG (lordgram.config.example copy karke bharo)"; exit 1; }

if [ ! -d "$SRC/.git" ]; then
  mkdir -p "$SRC" && cd "$SRC"
  git init -q && git remote add origin https://github.com/DrKLO/Telegram.git
  git fetch --depth 1 origin "$TG_COMMIT"
  git checkout -q FETCH_HEAD
  git submodule update --init --recursive --depth 1
fi
cd "$SRC"

if ! git apply --check -R "$ROOT/patches/0001-fight-mode-app.patch" 2>/dev/null; then
  git apply "$ROOT/patches/0001-fight-mode-app.patch"
fi
( cd TMessagesProj/jni/tgcalls
  if ! git apply --check -R "$ROOT/patches/0002-fight-mode-tgcalls.patch" 2>/dev/null; then
    git apply "$ROOT/patches/0002-fight-mode-tgcalls.patch"
  fi )

python3 "$ROOT/scripts/rebrand.py" "$SRC" "$CFG"

# Lord Gram launcher icon from branding/logo.jpg (needs: pip install pillow)
if [ -f "$ROOT/branding/logo.jpg" ]; then
  python3 "$ROOT/scripts/make_icons.py" "$SRC" "$ROOT/branding/logo.jpg"
fi

# own signing key (upstream's committed release.keystore must not be used)
if [ ! -f "$ROOT/signing.env" ]; then
  PASS="$(head -c 24 /dev/urandom | base64 | tr -dc 'A-Za-z0-9' | head -c 20)"
  rm -f TMessagesProj/config/release.keystore
  keytool -genkeypair -keystore TMessagesProj/config/release.keystore -alias lordgram \
    -keyalg RSA -keysize 4096 -validity 10000 -storepass "$PASS" -keypass "$PASS" \
    -dname "CN=Lord Gram, O=Lord Gram, C=IN"
  cat > "$ROOT/signing.env" <<ENV
export ORG_GRADLE_PROJECT_RELEASE_STORE_PASSWORD=$PASS
export ORG_GRADLE_PROJECT_RELEASE_KEY_PASSWORD=$PASS
export ORG_GRADLE_PROJECT_RELEASE_KEY_ALIAS=lordgram
ENV
  cp TMessagesProj/config/release.keystore "$ROOT/lordgram-backup.keystore"
  echo "signing.env + lordgram-backup.keystore bane. BACKUP le lo: ye key kho gayi to updates nahi de paoge."
fi
echo "Setup done. Ab: scripts/build.sh"
