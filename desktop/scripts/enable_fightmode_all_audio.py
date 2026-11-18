"""Extend Fight Mode to one-to-one calls and voice-message recording."""

from pathlib import Path
import sys


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    if new in text:
        return
    if old not in text:
        raise SystemExit(f"patch anchor not found: {path}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="")


src = Path(sys.argv[1])
tgcalls = src / "Telegram" / "ThirdParty" / "tgcalls"

dsp = tgcalls / "tgcalls" / "group" / "FightModeDsp.h"
replace_once(
    dsp,
    "};\n\n// Works on float samples in int16 scale (-32768..32768), mono.\n",
    """};

struct FightModeAtomicState {
    std::atomic<bool> enabled{false};
    std::atomic<float> gainDb{0.0f};
    std::atomic<float> volume{1.0f};
    std::atomic<float> eqDb[6];
    std::atomic<float> clarity{0.0f};
    std::atomic<float> punch{0.0f};
    std::atomic<bool> broadcast{false};

    FightModeAtomicState() {
        for (auto &band : eqDb) {
            band.store(0.0f);
        }
    }
};

inline FightModeAtomicState &fightModeGlobalState() {
    static FightModeAtomicState state;
    return state;
}

inline void setFightModeGlobal(const FightModeParams &params) {
    auto &state = fightModeGlobalState();
    state.gainDb.store(params.gainDb);
    state.volume.store(params.volume);
    for (int i = 0; i != 6; ++i) {
        state.eqDb[i].store(params.eqDb[i]);
    }
    state.clarity.store(params.clarity);
    state.punch.store(params.punch);
    state.broadcast.store(params.broadcast);
    state.enabled.store(params.enabled);
}

inline FightModeParams fightModeGlobalParams() {
    auto &state = fightModeGlobalState();
    auto result = FightModeParams();
    result.enabled = state.enabled.load();
    result.gainDb = state.gainDb.load();
    result.volume = state.volume.load();
    for (int i = 0; i != 6; ++i) {
        result.eqDb[i] = state.eqDb[i].load();
    }
    result.clarity = state.clarity.load();
    result.punch = state.punch.load();
    result.broadcast = state.broadcast.load();
    return result;
}

// Works on float samples in int16 scale (-32768..32768), mono.
""",
)

group = tgcalls / "tgcalls" / "group" / "GroupInstanceCustomImpl.cpp"
replace_once(
    group,
    "    void setFightMode(bool enabled, float gainDb, float volume, std::vector<float> eqDb) {\n        if (!_noiseSuppressionConfiguration) {",
    """    void setFightMode(bool enabled, float gainDb, float volume, std::vector<float> eqDb) {
        auto params = FightModeParams();
        params.enabled = enabled;
        params.gainDb = gainDb;
        params.volume = volume;
        for (int i = 0; i != 6; ++i) {
            params.eqDb[i] = i < int(eqDb.size()) ? eqDb[i] : 0.0f;
        }
        params.clarity = eqDb.size() > 6 ? eqDb[6] : 0.0f;
        params.punch = eqDb.size() > 7 ? eqDb[7] : 0.0f;
        params.broadcast = eqDb.size() > 8 && eqDb[8] > 0.5f;
        setFightModeGlobal(params);
        if (!_noiseSuppressionConfiguration) {""",
)

manager = tgcalls / "tgcalls" / "MediaManager.cpp"
replace_once(
    manager,
    '#include "FieldTrialsConfig.h"\n',
    '#include "FieldTrialsConfig.h"\n#include "group/FightModeDsp.h"\n',
)
replace_once(
    manager,
    "        _externalAudioSamplesMutex->Unlock();\n    }\n",
    """        _externalAudioSamplesMutex->Unlock();

        const auto fightMode = fightModeGlobalParams();
        if (fightMode.enabled) {
            const auto level = _fightModeDsp.process(
                buffer->channels()[0],
                buffer->num_frames(),
                buffer->sample_rate_hz(),
                fightMode);
            fightModeLevelStore().store(level);
        }
    }
""",
)
replace_once(
    manager,
    "    std::vector<float> *_externalAudioSamples = nullptr;\n",
    "    FightModeDsp _fightModeDsp;\n\n    std::vector<float> *_externalAudioSamples = nullptr;\n",
)

capture = src / "Telegram" / "SourceFiles" / "media" / "audio" / "media_audio_capture.cpp"
replace_once(
    capture,
    '#include "base/timer.h"\n',
    '#include "base/timer.h"\n\n#include <tgcalls/group/FightModeDsp.h>\n\n#include <vector>\n',
)
replace_once(
    capture,
    "\tbool _paused = false;\n",
    "\tbool _paused = false;\n\ttgcalls::FightModeDsp _fightModeDsp;\n",
)
replace_once(
    capture,
    "\tauto srcSamplesData = &srcSamplesDataChannel;\n\n\t//\tmemcpy",
    """\tauto srcSamplesData = &srcSamplesDataChannel;

\tconst auto fightMode = tgcalls::fightModeGlobalParams();
\tif (fightMode.enabled) {
\t\tstd::vector<float> samples(samplesCnt);
\t\tfor (auto i = 0; i != samplesCnt; ++i) {
\t\t\tsamples[i] = srcSamplesDataChannel[i];
\t\t}
\t\t_fightModeDsp.process(samples.data(), samplesCnt, kCaptureFrequency, fightMode);
\t\tfor (auto i = 0; i != samplesCnt; ++i) {
\t\t\tsrcSamplesDataChannel[i] = qBound(-32768, qRound(samples[i]), 32767);
\t\t}
\t}

\t//\tmemcpy""",
)

ui = src / "Telegram" / "SourceFiles" / "calls" / "group" / "calls_group_fight_mode.cpp"
replace_once(
    ui,
    '#include <tgcalls/group/GroupInstanceCustomImpl.h>\n',
    '#include <tgcalls/group/GroupInstanceCustomImpl.h>\n#include <tgcalls/group/FightModeDsp.h>\n',
)
replace_once(
    ui,
    "void ApplyFightMode() {\n\tif (const auto call = Core::App().calls().currentGroupCall()) {",
    """void ApplyFightMode() {
    auto params = tgcalls::FightModeParams();
    const auto &settings = FightModeShared();
    params.enabled = settings.enabled;
    params.gainDb = settings.gainDb;
    params.volume = settings.volume;
    for (int i = 0; i != kEqBands; ++i) {
        params.eqDb[i] = settings.eqDb[i];
    }
    params.clarity = settings.clarity;
    params.punch = settings.punch;
    params.broadcast = settings.broadcast;
    tgcalls::setFightModeGlobal(params);
    if (const auto call = Core::App().calls().currentGroupCall()) {""",
)

print("Fight Mode enabled for group calls, one-to-one calls, and voice recording")
