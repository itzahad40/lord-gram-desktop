#!/usr/bin/env bash
# Needs: JDK 17, Android SDK, NDK 27.2.12479018, CMake (Android Studio SDK Manager se).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
[ -f "$ROOT/signing.env" ] && source "$ROOT/signing.env"
cd "$ROOT/build-src"
./gradlew :TMessagesProj_App:assembleAfatRelease
echo "APK:"; find TMessagesProj_App/build/outputs/apk -name '*.apk'
