# Lord Gram Desktop (Windows)

Android version jaisa hi idea: **official Telegram Desktop (Qt/C++) untouched + chhoti patch layer**.
Pinned: `telegramdesktop/tdesktop` **v6.9.4** (`8a22f2c106dd3c601e11d124b8d1894613d596fd`), tgcalls `616810f1539e3f5ee917fff42557c5eeb8b4be40`.

## Android se kya reuse hua, kya naya

| Cheez | Desktop me |
|---|---|
| `FightModeDsp.h` (gain, EQ, clarity, broadcast) | Same file, sidha reuse (`tests/dsp` ab bhi pass) |
| tgcalls/audio hooks | Group calls aur 1-to-1 calls ke WebRTC capture path ke saath voice-message recording ke desktop capture path par bhi Fight Mode DSP lagta hai |
| UI | Naya neon Qt panel (Lord Carry setup jaisa look): left side menu -> **Fight mode**. VB-Cable / device routing **nahi** hai, kyunki Telegram ke andar hi mic process hota hai |
| Menu entry | Left side menu (hamburger) me **Fight mode** (Calls ke neeche), call ke bina bhi khulta hai. Fight mode ON ho to naam ke saath ek dot **blink** karta hai. Voice chat ke 3-dot menu me Fight mode **nahi** hai: sirf side menu se hi on/off hota hai |
| 20 accounts | `kMaxAccounts` / `kPremiumMaxAccounts` = 20 |
| Naam + icon | `AppName` = Lord Gram (alag data folder), icon `branding/logo.jpg` se |
| Mic Input source switch | Nahi banaya: desktop pe Settings -> Calls me mic device pehle se hai |

Panel: left card me logo, `LORD'S GRAM`, **FIGHT MODE: ON/OFF** button aur live mic level bars. Right card **LIVE EFFECT RACK (9 controls)**:
Gain 0-100 dB, Master output, Clarity, 6-band EQ (60 Hz se 16 kHz), aur buttons **BATTLE PRESET**, **CLEAN PRESET**, **BROADCAST ON/OFF**, **CLOSE**.
Punch slider jaan-boojh ke nahi rakha: compressor hata diya gaya hai isliye wo kuch nahi karta. Overdrive/echo/reverb jaise controls DSP me hain hi nahi, isliye fake nahi banaye.
Logo `branding/fightmode_logo.png` hai (badalna ho to wahi file replace karo).
Settings `tdata/lordgram_fightmode.ini` me save hoti hain. App khulne pe Fight mode OFF hota hai; ON karne ke baad wo app band hone tak ON rehta hai aur har group voice chat pe lagta hai.

**Audio:** mic wahi device use hota hai jo Telegram Settings -> Calls me chuna hai. Fight mode ab **group voice/video chat, 1-to-1 calls, aur voice message recording** ke mic path par lagta hai. Doosri apps ki awaaz (game, music) Telegram ke andar se nahi badli ja sakti, uske liye virtual cable driver chahiye.

## UI (patch 0003)

- Left folder bar (All Chats, Unread, ... Edit) ke bilkul neeche **gol Fight mode button** (logo + `FIGHT` / `FIGHT ON`). Tap = Fight mode panel. ON ho to laal ring blink karti hai.
- Koi chat select na ho to centre me **rounded card**: Lord Gram logo + "Select a chat to start messaging" + "Your messages will appear here".
- Abhi baaki: dark blue/green theme aur left folder bar ko default ON karna (ye alag step hai, abhi nahi hua).

## Build (GitHub Actions, recommended)

1. Is repo ko GitHub pe push karo (**Public** rakho, GPLv3 ke liye bhi zaroori).
2. Secrets: `LG_API_ID`, `LG_API_HASH` (my.telegram.org), optional `LG_APP_NAME`.
3. *Actions -> Build Lord Gram Desktop (Windows x64) -> Run workflow*.
4. Artifact `LordGram-Windows-x64` -> `Telegram.exe`. Pehli run me dependencies (Qt, ffmpeg, webrtc...) banti hain: **kai ghante** lag sakte hain, GitHub ki 6 ghante ki limit ke paas. Agar timeout ho to bas dobara run karo: cache se aage badhta hai.

Workflow upstream ke `win.yml` jaisa hai, sirf x64 + Qt6, auto-update band, crash reports band.
Ye **Debug** build hai (upstream ki CI jaisa; Release libraries aur zyada time/disk leti hain). Exe bada hoga, par chalega.

Local build: `desktop/scripts/setup_desktop.sh` source ready karta hai, phir Telegram ka `docs/building-win.md` follow karo (Visual Studio 2022, ~40 GB).

## Status

| Cheez | Status |
|---|---|
| Patches pinned v6.9.4 pe `git apply --check` | Pass |
| DSP unit tests | Pass |
| Pura pipeline (patches + logo qrc + rebrand) saaf pinned copy pe | Pass |
| C++/Qt code compile aur panel ka asli look | **Verify nahi hua**: Qt/MSVC toolchain yahan nahi tha. Pehli CI run me compile error aaye to log ke `error C` lines bhejo, fix kar dunga |
| Fight mode ka asli audio asar | **Verify nahi hua**, 2 PC pe call me test karna hoga |
| 20 accounts | **Verify nahi hua** |
| Windows installer/updater | Nahi, sirf `Telegram.exe` |

## Safety / limitations

- Android jaisa hi: compressor/limiter nahi hai, **~20 dB se upar gain clip/distort** karta hai. Box me warning bhi hai.
- Speaker se feedback hota hai: **headset use karo**.
- Push notifications desktop pe normal chalti hain (Firebase nahi lagta).
- Unofficial client hai. Telegram Desktop **GPLv3** hai: source public rakho, `LICENSE` add karo, sirf apni `api_id/hash` use karo.
