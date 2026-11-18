# Lord Gram architecture

Lord Gram = **upstream Telegram Android, untouched**, plus ek chhoti patch layer. Source fork nahi karte,
pinned commit pe patches apply karte hain, taaki upstream update lena aasaan rahe.

```
LordGram (is repo, chhota)            build-src/ (generated, gitignored)
  patches/       ---- git apply --->   DrKLO/Telegram @ pinned commit
  scripts/rebrand.py --- edit ------>    + submodule tgcalls (patched)
  lordgram.config (keys, package)        -> gradle -> APK
```

## Layers jo Fight mode touch karta hai

```
GroupCallActivity (UI)         3-dot menu "Fight mode" item + FightModeBubble (round mic, top right) -> tap -> FightModeWidget (Gain knob, Voice, Equalizer, Input, mic, volume, LED meter)
        |  setFightMode(enabled, gain)
VoIPService (Java)             fightModeEnabled (call ke saath marta hai; sirf 3-dot menu se OFF hota hai) + FightMode settings (prefs)
        |
NativeInstance (JNI)           setFightMode(enabled, gainDb, volume, eq[6]) + getFightModeLevel()
        |  org_telegram_messenger_voip_Instance.cpp
tgcalls::GroupInstanceCustomImpl::setFightMode      (media thread)
        |  atomics in NoiseSuppressionConfiguration  (thread-safe, lock-free)
AudioCapturePostProcessor::Process (audio thread)   <- WebRTC outgoing mic frame
        |  FightModeDsp: 40 Hz high-pass -> 6-band EQ -> gain 0..100 dB (noise gate above 40 dB) -> soft-knee limiter -> volume, 20 ms fade in/out
        v
Opus encoder -> voice chat
```

Rules:
- **Mic ek hi jagah capture hota hai** (WebRTC). Alag AudioRecord nahi kholte.
- DSP header-only (`FightModeDsp.h`) hai, WebRTC ke bina test hota hai: `tests/dsp`.
- Audio thread pe koi lock/alloc nahi, sirf `std::atomic` read.
- Fight mode sheet band hone pe OFF nahi hota; sirf 3-dot menu se ya call khatam hone pe OFF.
- Upstream file me jitna kam badle utna achha: patch me sirf additive hunks.

## Naya feature kaise jodein
1. Change upstream checkout me karo (`build-src/`), `git diff` se naya patch banao `patches/000N-*.patch`.
2. `setup.sh` me apply line add karo, `UPSTREAM.md` update karo.
