# Lord Gram

**Lord Gram** ek unofficial Telegram Android client hai (open-source Telegram for Android ke upar patches). Sab kuch Telegram jaisa chalta hai, bas group aur channel **voice chat** me ek naya **Fight mode** milta hai: tumhari mic ki awaaz announcement / PA system jaisi loud hoke voice chat me jaati hai, aur ek chhota Microphone panel se tum gain, equalizer aur volume control karte ho.

> Ye repo poora Telegram source nahi rakhta (wo kai GB hai). Yahan sirf **patches + scripts + CI workflow** hain. Setup script pinned Telegram version download karke patches lagata hai, naam/logo/keys badalta hai, aur APK banata hai.

> **Desktop (Windows) version:** `desktop/DESKTOP.md` dekho. Telegram Desktop pe alag patches + workflow `build-desktop-windows.yml`.

---

## Contents

1. [Features](#features)
2. [Fight mode kaise use karein](#fight-mode-kaise-use-karein)
3. [Controls ka detail](#controls-ka-detail)
4. [Architecture](#architecture)
5. [Repo structure](#repo-structure)
6. [Build karne se pehle chahiye](#build-karne-se-pehle-chahiye)
7. [Build Option A: GitHub Actions (recommended)](#build-option-a-github-actions-recommended)
8. [Build Option B: Local](#build-option-b-local)
9. [Configuration](#configuration)
10. [Testing](#testing)
11. [Status: kya verify hua hai](#status-kya-verify-hua-hai)
12. [Known limitations](#known-limitations)
13. [Troubleshooting](#troubleshooting)
14. [Safety notes](#safety-notes)
15. [Roadmap](#roadmap)
16. [Legal](#legal)

---

## Features

| Feature | Detail |
|---|---|
| **Fight mode** | Voice chat ke 3-dot menu me Noise suppression ke neeche naya option |
| **Round mic bubble** | Fight mode ON hote hi upar right me gol mic. Tap karo to panel khulta hai |
| **Microphone panel** | Neon studio look: rotary Gain knob, Voice, Equalizer, Input, glowing studio mic (tap = mute/unmute), vertical volume, LED level meter |
| **Gain 0 se 100 dB** | 40 dB ke upar noise gate hiss rokta hai |
| **Voice (Clarity + Punch + Broadcast)** | Clarity = mud kam, 3.2 kHz presence zyada (shabd saaf). Punch = compressor, dheemi aur tez awaaz barabar. 5 styles: Natural, Clear, Loud & Clear, Megaphone, **Broadcast** (dynamic de-hiss + harmonic exciter — radio jaisi dense, loud awaaz, kam hiss) |
| **6-band Equalizer** | 60 / 400 / 1000 / 2500 / 6300 / 16000 Hz, har band ±12 dB, 13 presets (Megaphone bhi) + Reset |
| **Input (In Channel)** | Mic source: Default, Cam, Voice, Mic, Call. Call ke beech me badalta hai |
| **Soft-knee limiter** | Output kabhi clip ya overflow nahi karta |
| **Zyada accounts** | Telegram ki 3 (Premium pe 5) wali limit hata di: ek saath 20 accounts login ho sakte hain, Premium ki zaroorat nahi |
| **Lord Gram branding** | Chat list ka title `𝗟𝗢𝗥𝗗'𝗦 𝗚𝗥𝗔𝗠` (bold font), app ka naam, launcher icon (Lords Community logo) |
| **Persistent settings** | Gain, volume aur EQ yaad rehte hain |

Baaki sab (chats, channels, media, calls) upstream Telegram jaisa hi hai.

---

## Fight mode kaise use karein

1. Kisi group ya channel ka **voice chat** join karo.
2. Upar **3-dot menu** kholo aur **Fight mode** dabao. Subtext "Enabled" ho jaata hai.
3. Upar right me **gol mic bubble** aata hai.
   - Neela = mic muted. Laal = mic live (awaaz ke saath glow karta hai).
   - Bubble ko pakad ke kahin bhi **drag** kar sakte ho.
4. Bubble pe **tap** karo, **Microphone panel** khulta hai.
5. Panel ke **bahar kahin bhi tap** karo (ya header ka ▾ dabao): panel band ho jaata hai, **bubble rehta hai**.
6. Fight mode band karne ke liye wapas **3-dot menu → Fight mode** dabao. Kisi aur tareeke se band nahi hota (screen tap, sheet band karna waghera se nahi).

Call khatam hone pe Fight mode apne aap OFF ho jaata hai. Nayi call hamesha OFF se shuru hoti hai.

---

## Controls ka detail

| Control | Kya karta hai |
|---|---|
| **Gain** (tile + knob) | 0 se 100 dB mic boost. Knob ko upar/neeche drag karo, ya slider, ya quick buttons (12, 24, 40, 60 dB) |
| **Voice** | Style chips + **Clarity** aur **Punch** sliders (0 se 100%). Megaphone style EQ bhi set karta hai |
| **Equalizer** | Tile pe preset ka naam ya `Custom`. Khol ke 6 sliders, **Presets** (list) aur **Reset** |
| **Input** | Mic source chuno. Sirf Fight mode ON ke dauran lagta hai, OFF karte hi normal Call source wapas |
| **Studio mic** | Tap = mic mute/unmute. LIVE/MUTED pill, awaaz ke saath rings pulse |
| **Volume** (vertical slider) | Final output ka master volume, 0 se 100% |
| **LED meter** (mic ke left) | Boost ke **baad** ki awaaz dikhata hai (hara, peela, laal). Dusre phone ke bina bhi dikh jaata hai ki gain lag raha hai |

**Presets:** Flat, Easy Listening, Voice, Bass Boost, Treble Boost, Rock, Pop, Dance, Hip Hop, Jazz, Classical, Heavy Metal, Megaphone.

**Panel header** pakad ke panel ko bhi drag kar sakte ho.

---

## Architecture

Lord Gram = **upstream Telegram Android, untouched** + ek chhoti patch layer. Fork nahi rakhte, pinned commit pe patches lagate hain, taaki upstream update lena aasaan rahe.

```
LordGram (ye repo, chhota)              build-src/ (generate hota hai, gitignored)
  patches/        --- git apply --->     DrKLO/Telegram @ pinned commit
  scripts/        --- rebrand ------>      + submodule tgcalls (patched)
  branding/       --- icons -------->      -> Gradle -> APK
  lordgram.config (keys, naam)
```

Mic source ("Input") ka path: `FightMode.applyInput` -> `WebRtcAudioRecord.switchAudioSource` (Telegram ka `org.webrtc.voiceengine` recorder): reader thread rokta hai, naya `AudioRecord` naye source se banata hai, wapas start karta hai (lagbhag 0.2 s ka gap). Naya source na chale to purana source wapas laga deta hai. Call khatam hone pe source reset.

Fight mode ka audio path:

```
GroupCallActivity (UI)       3-dot menu item, FightModeBubble, FightModeWidget
        |  setFightMode / applyFightMode
VoIPService (Java)           ON/OFF state (call ke saath marta hai) + FightMode settings (SharedPreferences)
        |
NativeInstance (JNI)         setFightMode(enabled, gainDb, volume, eq[6]), getFightModeLevel()
        |
tgcalls GroupInstanceCustomImpl::setFightMode          (media thread)
        |  std::atomic values (lock-free)
AudioCapturePostProcessor::Process                     (audio thread, WebRTC outgoing mic frame)
        |  FightModeDsp:
        |    40 Hz high-pass
        |    noise gate (40 dB gain ke upar, threshold lagbhag -58 dBFS)
        |    6-band peaking EQ (Q = 1)
        |    clarity (250 Hz mud kam, 3.2 kHz presence zyada)
        |    broadcast (de-hiss + harmonic exciter, optional)
        |    gain (0..100 dB, smoothed)
        |    master volume
        |    -- compressor, soft-knee limiter aur final int16 clamp HATA DIYE GAYE (request pe) --
        v
Opus encoder -> voice chat
```

Design rules:

- **Mic sirf ek jagah capture hota hai** (WebRTC). Doosra `AudioRecord` nahi kholte, warna mic conflict hota.
- DSP **header-only** hai (`FightModeDsp.h`), WebRTC ke bina test hota hai.
- Audio thread pe koi lock ya allocation nahi, sirf `std::atomic` reads.
- Fight mode OFF hone pe 20 ms fade-out, aur bypass **bit-exact** hai.
- Patches additive rakhe hain: upstream files me jitna kam badle utna achha.

Pinned versions (`UPSTREAM.md`): DrKLO/Telegram `3de37c8cfb039452be7398909461da6288babf15`, tgcalls `1398c1648f52666b3249111ec3c5b531946e41e4`.

---

## Repo structure

```
.
├── .github/workflows/build-apk.yml   # cloud build
├── branding/logo.jpg                 # launcher icon ka source
├── docs/ARCHITECTURE.md
├── patches/
│   ├── 0001-fight-mode-app.patch     # app: UI, bubble, widget, JNI, VoIPService, mic source switch, title
│   └── 0002-fight-mode-tgcalls.patch # native: DSP + audio hook (tgcalls submodule me lagta hai)
├── scripts/
│   ├── setup.sh                      # fetch, patch, rebrand, icons, signing key
│   ├── rebrand.py                    # package, naam, api_id/hash
│   ├── make_icons.py                 # logo se launcher icons
│   └── build.sh                      # Gradle build
├── tests/dsp/                        # FightModeDsp.h + unit test
├── lordgram.config.example
├── UPSTREAM.md
└── README.md
```

---

## Build karne se pehle chahiye

1. **Telegram API keys**: https://my.telegram.org -> *API development tools* -> apna `api_id` aur `api_hash`. Telegram ki official keys use mat karo (`rebrand.py` bina apni keys ke chalta hi nahi).
2. **Apna package name**, jaise `com.lordgram.messenger` (`org.telegram.*` allowed nahi).
3. **Signing keystore** (apni key). Isse backup rakho, kho gayi to app update nahi de paoge.

Keystore banana (JDK 17 ke `keytool` se), password sirf letters aur numbers:

```bash
keytool -genkeypair -keystore lordgram.keystore -alias lordgram -keyalg RSA -keysize 4096 -validity 10000 \
  -storepass YOURPASSWORD -keypass YOURPASSWORD \
  -dname "CN=Your Name, OU=Your Unit, O=Your Org, L=City, ST=State, C=IN"
```

Windows pe `C:\` ke root me file nahi banti (Access denied), apne user folder me banao. Base64 secret ke liye PowerShell:

```powershell
$b = [Convert]::ToBase64String([IO.File]::ReadAllBytes("$HOME\lordgram.keystore"))
$b.Length        # lagbhag 5900 ke aas-paas aana chahiye
$b | Set-Clipboard
```

---

## Build Option A: GitHub Actions (recommended)

Windows pe local NDK build jhanjhat hai, isliye cloud build best hai.

**1. Repo banao aur push karo.** Repo **Public** rakho: free Public runner pe 4 CPU / 16 GB RAM milti hai, Private pe sirf 2 CPU / 7 GB, aur Telegram ki build wahan toot sakti hai.

**2. Secrets daalo.** *Settings -> Secrets and variables -> Actions -> New repository secret*:

| Secret | Value |
|---|---|
| `LG_API_ID` | my.telegram.org ka api_id (sirf number) |
| `LG_API_HASH` | my.telegram.org ka api_hash |
| `LG_PACKAGE` | `com.lordgram.messenger` (apna) |
| `LG_APP_NAME` | `Lord Gram` |
| `RELEASE_KEYSTORE_B64` | keystore ka base64 (poora, ek line) |
| `RELEASE_STORE_PASSWORD` | keystore ka password |
| `RELEASE_KEY_ALIAS` | `lordgram` |

Naam bilkul exact likho, aur value ke aage-peeche space mat chhodo.

**3. Run karo.** *Actions -> Build Lord Gram APK -> Run workflow* (branch `main`). Purani run ko "Re-run" mat karo, wo purana workflow file hi chalata hai.

**4. APK download karo.** Run khatam hone pe page ke neeche *Artifacts -> LordGram-apk*. Lagbhag 1.5 se 2 ghante lagte hain.

### Workflow kya karta hai

| Step | Kaam |
|---|---|
| Check secrets | 7 secrets ka `SET / EMPTY` check |
| Free disk space | Extra preinstalled cheezein hatata hai, taaki disk aur RAM milein |
| JDK 17, Android SDK pieces | NDK `27.2.12479018`, CMake `3.22.1`, platform 36, build-tools 36.0.0 |
| Fetch, patch, rebrand | Keystore decode + verify, Telegram pinned commit download, patches, naam/keys, icons |
| Disable Telegram Firebase config | Telegram ka `google-services.json` tumhare package ke liye valid nahi, isliye plugin band |
| Build only arm64 | `abiFilters` sirf `arm64-v8a`, build kai guna tez |
| Skip R8 | Runner pe R8 memory khatam kar deta tha, isliye minify/shrink band |
| Build | `assembleAfatRelease` |
| Diagnostics | Fail pe disk/RAM ka haal |

Timeout 300 minute hai.

---

## Build Option B: Local

Chahiye: git, python3 (+ `pip install pillow`), JDK 17, Android SDK + NDK `27.2.12479018` + CMake, lagbhag 30 GB disk. **WSL2 Ubuntu, Linux ya macOS** pe chalao.

```bash
cp lordgram.config.example lordgram.config     # bharo
pip install pillow
scripts/setup.sh      # download, patch, rebrand, icons, signing key
# workflow wale 3 sed (firebase, arm64, R8) local build me bhi lagane honge
scripts/build.sh      # APK: build-src/TMessagesProj_App/build/outputs/apk/
```

`setup.sh` pehli baar `signing.env` aur `lordgram-backup.keystore` banata hai. Dono ka backup rakho.

---

## Configuration

`lordgram.config` (gitignored):

```
LG_PACKAGE=com.lordgram.messenger
LG_APP_NAME=Lord Gram
LG_API_ID=123456
LG_API_HASH=your_api_hash
```

`rebrand.py` ye badalta hai: `APP_PACKAGE` (`gradle.properties`), `APP_ID` / `APP_HASH` (`BuildVars.java`), `AppName` string. `make_icons.py` `branding/logo.jpg` se launcher icons banata hai (legacy PNG, round, adaptive foreground).

Logo badalna ho to `branding/logo.jpg` replace karo (square image best hai, adaptive mask me edges kat jaate hain, isliye important cheezein beech me rakho).

---

## Testing

DSP ka unit test (WebRTC ke bina, sirf g++):

```bash
tests/dsp/run.sh
```

Ye check karta hai: bypass bit-exact, gain dB accurate (limiter se neeche), output kabhi int16 range se bahar nahi, volume 50% = lagbhag -6 dB, EQ band +9 dB sahi, 60 Hz bass boost, 100 dB pe bhi bounded, gate mic hiss ko dabata hai, OFF pe fade-out, level meter range.

Patches ko pinned commits pe `git apply --check` se verify kiya gaya hai.

---

## Status: kya verify hua hai

| Cheez | Status |
|---|---|
| APK build (GitHub Actions) | Ban chuki hai aur phone pe chalti hai |
| Microphone panel (Gain, Phonic, Equalizer tile, mic, volume, level bar) | Phone pe dikha (screenshot) |
| DSP logic | Unit tests pass |
| Naya neon UI, widget/bubble/FightMode ka Java code | Real `javac` se compile check hua (Android stubs ke saath), phone pe **verify karna baaki** |
| Mic source switch (`WebRtcAudioRecord`) | Naya code, sirf type-check hua, **phone pe verify karna baaki** (kuch phones source ignore karte hain) |
| Gol mic bubble, outside-tap collapse | Patch me hai, **verify karna baaki** |
| Voice (Clarity, Punch), 13 presets, Reset | DSP unit tests pass, UI **verify karna baaki** |
| **Broadcast mode (de-hiss + harmonic exciter)** | DSP unit tests pass (hiss down, output always bounded, density +1.4 dB on a test tone), UI **verify karna baaki** |
| 100 dB gain slider | DSP tests pass, **verify karna baaki** |
| 20 accounts limit | Native `tgnet` change, patch verified against pinned source, **phone pe verify karna baaki** |
| Chat list title `𝗟𝗢𝗥𝗗'𝗦 𝗚𝗥𝗔𝗠` aur Spartan logo icon | Patch/scripts me hai, **verify karna baaki** |
| Dusre users ko loud awaaz sunai dena | **Dusre phone pe test karna baaki** |

---

## Known limitations

- **Compressor, limiter aur clip hata diye gaye hain (request pe).** Isका matlab: **20 dB se upar gain output int16 ki range (32767) se bahar nikal sakta hai**, aur bahut zyada gain (60–100 dB) pe output ceiling se **hazaron guna** upar jaata hai. Test me 100 dB pe peak ~9117x upar nikla. Ye audio pipeline me undefined/garbage behaviour (tez crackle, static, ya bahut harsh noise) bana sakta hai, "aur loud" nahi. **Sirf 10–20 dB gain safe range hai is state me.** Agar chaho to simple safety clamp wapas daal sakta hoon (bina compressor/limiter ke, sirf overflow na ho).
- **Punch slider ab inert hai.** Compressor hatne se "Punch" control kuch nahi karta, par UI me abhi bhi dikhta hai. Hatwana ho to bata dena.

- **Push notifications nahi**: Firebase config band hai (Telegram ka config sirf official packages ke liye hai). App band hone pe FCM push nahi aayegi. Apna Firebase project aur push solution alag kaam hai.
- **Sirf arm64-v8a**. Bahut purane 32-bit phones pe install nahi hoga.
- **R8 band hai**, isliye APK bada hai aur obfuscate nahi hai.
- **Stereo nahi**: mic ek hi hai aur voice chat ka capture path mono hai. Isliye Phonic tile hata diya (sirf "Mono" dikhata tha).
- **200 dB ya 150 dB ka fayda nahi**: digital audio 0 dBFS pe cap hota hai. Test me normal awaaz 60 dB pe aur dheemi awaaz 100 dB pe limit tak pahunch jaati hai. Usse upar sirf shor distort hota hai.
- **Rate (44 kHz) aur Out** (Microphone app ke options, jo Input ke alawa image me the): voice chat 48 kHz Opus pe fixed hai, aur output stream sunne wale ki awaaz badalta hai, bhejne wale ki nahi. Isliye nahi banaye.
- **Baaki jagah "Telegram" text**: sirf app naam, chat list title aur icon badle hain. Settings aur doosri screens me kuch strings abhi "Telegram" hi hain.
- Kuch phones pe bahut dheemi awaaz noise gate se kat sakti hai (gate sirf 40 dB gain ke upar chalu hota hai).

---

## Troubleshooting

| Error / Symptom | Wajah aur fix |
|---|---|
| `Missing RELEASE_STORE_PASSWORD secret` / `... : EMPTY` | Secret khali ya naam galat. Delete karke naam copy-paste karke dobara banao |
| `base64: error: invalid input` / `not valid Base64` / `bahut chhota hai (3 bytes)` | `RELEASE_KEYSTORE_B64` adhura paste hua. `$b.Length` check karo, Notepad me paste karke dekho, phir secret poora khali karke dobara daalo |
| `KEYTOOL FAILED` / `password was incorrect` | `RELEASE_STORE_PASSWORD` wo nahi jo `-storepass` me diya tha, ya alias `lordgram` nahi |
| `keytool` prompts me loop (Is ... correct? [no]) | Wahan sirf `yes` likhna hota hai. Country code do letters ka (`IN`). Ya poori command `-dname` ke saath ek line me chalao |
| `keytool error: ... Access is denied` | `C:\` root me nahi, `cd $HOME` karke chalao |
| `No matching client found for package name` (`processAfatReleaseGoogleServices`) | Telegram ka Firebase config. Workflow ka "Disable Telegram Firebase config" step chahiye. Run naya chalao, Re-run nahi |
| `The runner has received a shutdown signal` / `The operation was canceled` | R8 ya disk/RAM. "Skip R8" aur "Free disk space" steps chahiye. Repo Public rakho. Diagnostics step ka output dekho |
| Tap karne pe Fight mode band ho jaata tha | Purani patch. `patches/0001-fight-mode-app.patch` me `FightModeBubble` dhundo, na mile to latest patch upload karo |
| `git apply` fail | Pinned commit se alag Telegram version. `UPSTREAM.md` ka commit use karo |
| Git galat folder ko repo samajh raha (`index.lock`) | `git init` home folder me chal gaya. `git rev-parse --show-toplevel` check karo, project folder me naya repo banao |

Build fail ho to jis step pe laal X ho, uski aakhri 30 se 40 lines (khaas kar `error:`, `FAILED`, `What went wrong`) issue me paste karo.

---

## Safety notes

- **Kaan ko nuksaan**: 60 dB ke upar gain pe awaaz limiter ki upar ki limit pe hoti hai. Sunne wale ke kaan aur speaker ke liye ye bahut tez ho sakti hai. Dusron ko pehle bata do ya gain kam rakho.
- **Feedback (cheekh)**: speaker se mic me awaaz wapas jaane se cheekh aati hai. **Headset use karo.**
- Extreme gain pe peeche ka shor aur saans bhi loud aati hai. 40 dB ke upar gate se kam hoti hai, lekin khatam nahi.
- Apna signing key aur `signing.env` kabhi public repo me commit mat karo (`.gitignore` me pehle se hain).

---

## Roadmap

- [x] **In Channel** (mic source) code ho gaya hai, phone pe test karna baaki
- [ ] Input source ke saath "Raw/Unprocessed" option
- [ ] Push notifications (apna Firebase ya alag push solution)
- [ ] 32-bit (`armeabi-v7a`) build option
- [ ] R8/minify wapas, kam memory ke saath
- [ ] Baaki screens ke "Telegram" strings ko Lord Gram me badalna
- [ ] Gate threshold ko setting banana
- [ ] Screenshots `docs/screens/` me add karna

---

## Legal

- Lord Gram **unofficial** Telegram client hai aur Telegram se affiliated nahi hai. App me ye likhna chahiye.
- Telegram for Android **GPLv2** hai. Ye patches uska derivative hain, isliye is repo ka source **public** rakho aur GPLv2 ke terms follow karo. Repo me `LICENSE` file add kar dena.
- **Sirf apni `api_id` / `api_hash`** use karo, aur Telegram API Terms of Service follow karo.
- Telegram ka naam aur logo use mat karo. Launcher icon (Lords Community logo) tumhari community ka apna logo hai.
- `branding/logo.jpg` aur app ke naam ka zimma repo owner ka hai.
