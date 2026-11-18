# Lord Gram Desktop

Lord Gram Desktop is an unofficial Windows Telegram Desktop build with Lord Gram branding and a Fight Mode microphone DSP. The repository contains patches and build scripts; the full Telegram Desktop source is downloaded during setup.

The current primary target is Windows desktop. The repository also contains the earlier Android patch set.

## Fight Mode

Fight Mode processes microphone audio inside the client for:

- Group voice and video chats
- One-to-one calls
- Voice-message recording

It does not process audio from other applications. A virtual audio cable would be required for games or music.

The desktop panel is available from the left menu under **Fight mode**. It provides gain, master volume, clarity, six-band EQ, Battle/Clean presets, Broadcast mode, and a live microphone level display. Settings are stored in `tdata/lordgram_fightmode.ini`.

Fight Mode starts disabled when the application launches. High gain can clip or distort audio because the requested DSP chain has no compressor or limiter. Use a headset to prevent feedback.

## Windows build with GitHub Actions

GitHub Actions is recommended because the first build downloads Qt, FFmpeg, WebRTC, MSVC-related dependencies, and MSYS2 packages and may take several hours.

1. Push this repository to GitHub.
2. Add these repository secrets under **Settings → Secrets and variables → Actions**:
   - `LG_API_ID`: numeric Telegram API ID
   - `LG_API_HASH`: Telegram API hash
   - `LG_APP_NAME`: optional; defaults to `Lord Gram`
3. Run **Actions → Build Lord Gram Desktop (Windows x64)**.
4. Download `Telegram.exe` from the `LordGram-Windows-x64` artifact.

The workflow builds pinned Telegram Desktop v6.9.4 as an x64 Qt6 Debug build, applies Lord Gram patches/assets, and disables auto-update and crash reports. It also replaces the obsolete MSYS2 package `mingw-w64-x86_64-diffutils` with `diffutils` and applies a CMake CMP0218 compatibility fix for the pinned `cmark_gfm` dependency.

See [desktop/DESKTOP.md](desktop/DESKTOP.md) for the complete desktop feature list and limitations.

## Local setup

Local Windows compilation requires Git Bash, Python 3 with Pillow, Visual Studio with the C++ workload, the Windows SDK, CMake, NuGet, and substantial disk space.

```bash
python -m pip install pillow
bash desktop/scripts/setup_desktop.sh
```

The setup script downloads the pinned source, applies patches, installs the Fight Mode asset, and generates icons. The final native build must run from a Visual Studio developer environment using Telegram Desktop's Windows build instructions.

## Configuration and secrets

For local setup, copy `lordgram.config.example` to `lordgram.config`:

```ini
LG_PACKAGE=com.lordgram.messenger
LG_APP_NAME=Lord Gram
LG_API_ID=12345678
LG_API_HASH=replace_with_your_api_hash
```

`lordgram.config` is ignored by Git. Line 4, `LG_API_HASH`, is a private Telegram credential. Never publish it in the README, a patch, a commit, or an issue. GitHub Actions uses repository secrets instead of this local file.

Use only API credentials created for your own application and follow Telegram's terms.

## Repository layout

```text
desktop/
  DESKTOP.md
  patches/
  scripts/setup_desktop.sh
  scripts/enable_fightmode_all_audio.py
.github/workflows/build-desktop-windows.yml
branding/
tests/dsp/
lordgram.config.example
```

## Testing and status

The DSP can be tested without WebRTC:

```bash
bash tests/dsp/run.sh
```

The source preparation pipeline and Python setup helpers have been validated. A complete C++/Qt build and live audio behavior require the Windows toolchain and real call/recording tests.

Telegram Desktop is GPLv3. Keep source and patch changes available when distributing binaries.
