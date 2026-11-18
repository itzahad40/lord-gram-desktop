// Lord Gram: "Fight mode" mic processing (loud, clear PA / announcement style voice).
// Header-only so it can be unit-tested without WebRTC.
// Chain: high-pass 40 Hz -> noise gate (only above 40 dB gain) -> 6-band EQ -> clarity (cut mud, boost presence)
//        -> broadcast (dynamic de-hiss + harmonic exciter, optional) -> gain (dB) -> volume.
// NOTE: the compressor, the soft-knee limiter and the final int16 clamp ("clip") were removed on request.
// There is now NOTHING stopping the output from exceeding the -32768..32767 range at high gain: the "punch"
// parameter is kept in the struct for compatibility but no longer does anything (the compressor read it).
#ifndef TGCALLS_GROUP_FIGHT_MODE_DSP_H
#define TGCALLS_GROUP_FIGHT_MODE_DSP_H

#include <algorithm>
#include <atomic>
#include <cmath>

namespace tgcalls {

// Last output level (0..1) of the fight mode chain, readable from any thread (UI level meter).
inline std::atomic<float> &fightModeLevelStore() {
    static std::atomic<float> level{0.0f};
    return level;
}

struct FightModeParams {
    bool enabled = false;
    float gainDb = 0.0f;      // 0 .. 100
    float volume = 1.0f;      // 0 .. 1 (master output)
    float eqDb[6] = {0, 0, 0, 0, 0, 0}; // -12 .. +12 at 60 / 400 / 1000 / 2500 / 6300 / 16000 Hz
    float clarity = 0.0f;     // 0 .. 1  (cuts 250 Hz mud, boosts 3.2 kHz presence)
    float punch = 0.0f;       // 0 .. 1  (compressor amount: evens out quiet and loud syllables)
    bool broadcast = false;   // dynamic de-hiss (quiet = more cut) + harmonic exciter (denser, more "radio" voice)
};

// Works on float samples in int16 scale (-32768..32768), mono.
class FightModeDsp {
public:
    static constexpr int kBands = 6;
    static constexpr float kMaxGainDb = 100.0f;
    static constexpr float kMaxEqDb = 12.0f;

    // Processes one frame in place. Returns the output peak (0..1), 0 when bypassed.
    float process(float *samples, int count, int sampleRate, const FightModeParams &p) {
        if (!samples || count <= 0 || sampleRate <= 0) {
            return 0.0f;
        }
        if (!p.enabled && _mix <= 0.0f) {
            resetState();
            return 0.0f;
        }

        const float gainDb = clampf(p.gainDb, 0.0f, kMaxGainDb);
        const float targetGain = std::pow(10.0f, gainDb / 20.0f);
        const float targetVolume = clampf(p.volume, 0.0f, 1.0f);
        const float clarity = clampf(p.clarity, 0.0f, 1.0f);
        updateEq(p, sampleRate);
        updateClarity(clarity, sampleRate);
        updateDeHiss(sampleRate);
        const bool broadcast = p.broadcast;
        const float exCoef = std::exp(-2.0f * 3.14159265f * 1500.0f / (float)sampleRate);

        const float fadeStep = 1.0f / (0.020f * (float)sampleRate);                    // 20 ms wet/dry fade
        const float smooth = 1.0f - std::exp(-1.0f / (0.010f * (float)sampleRate));    // 10 ms param smoothing
        const float hpCoef = std::exp(-2.0f * 3.14159265f * 40.0f / (float)sampleRate);

        // Above 40 dB gain the mic hiss would become as loud as the voice, so a gate cuts the silence between words.
        const float gateAmount = clampf((gainDb - 40.0f) / 20.0f, 0.0f, 1.0f);
        const float envRelease = std::exp(-1.0f / (0.050f * (float)sampleRate));
        const float gateAttack = 1.0f - std::exp(-1.0f / (0.002f * (float)sampleRate));
        const float gateRelease = 1.0f - std::exp(-1.0f / (0.060f * (float)sampleRate));
        const float kGateThreshold = 40.0f; // int16 scale, about -58 dBFS

        float peak = 0.0f;
        for (int i = 0; i < count; i++) {
            _mix = clampf(_mix + (p.enabled ? fadeStep : -fadeStep), 0.0f, 1.0f);
            _gain += (targetGain - _gain) * smooth;
            _volume += (targetVolume - _volume) * smooth;

            const float dry = samples[i];

            // high-pass at 40 Hz: removes DC / rumble before the gain stage
            float x = hpCoef * (_hpPrevOut + dry - _hpPrevIn);
            _hpPrevIn = dry;
            _hpPrevOut = x;

            // noise gate (follows the level before any gain)
            _env = std::max(std::fabs(x), _env * envRelease);
            const float gateTarget = _env > kGateThreshold ? 1.0f : 0.0f;
            _gate += (gateTarget - _gate) * (gateTarget > _gate ? gateAttack : gateRelease);
            x *= 1.0f - gateAmount * (1.0f - _gate);

            // 6-band peaking EQ
            for (int b = 0; b < kBands; b++) {
                if (_bandActive[b]) {
                    x = runBiquad(_bq[b], x);
                }
            }

            // clarity: cut mud, boost presence
            if (_clarityActive) {
                x = runBiquad(_clarityBq[0], x);
                x = runBiquad(_clarityBq[1], x);
            }

            // broadcast: dynamic de-hiss (cuts the mic hiss while you are quiet, leaves full treble while you speak)
            // plus a harmonic exciter (adds a little "edge" so the voice sounds dense, like on the radio)
            if (broadcast) {
                _deHissEnv = std::max(std::fabs(x), _deHissEnv * envRelease);
                const float voiceRatio = clampf((_deHissEnv - 40.0f) / 460.0f, 0.0f, 1.0f);
                const float deHissed = runBiquad(_deHissBq, x);
                x = x + (deHissed - x) * (1.0f - voiceRatio);

                const float hp = exCoef * (_exPrevOut + x - _exPrevIn);
                _exPrevIn = x;
                _exPrevOut = hp;
                // adds back a saturated copy of the high content: more "edge"/density, same phase as the original highs
                const float sat = std::tanh(hp / 600.0f) * 600.0f;
                x += sat * 0.32f;
            }

            // gain + volume, straight through: no compressor, no limiter, no final clamp.
            // at high gain (or with clarity/EQ boosts stacked on top) this WILL go past the
            // -32768..32767 range; whatever receives these samples next clamps/wraps it, which
            // is heard as harsh clipping/crackling, not as "louder" audio.
            const float wet = (x / 32768.0f) * _gain * 32767.0f * _volume;
            const float out = dry + (wet - dry) * _mix;
            samples[i] = out;
            peak = std::max(peak, std::fabs(out));
        }
        return std::min(peak / 32768.0f, 1.0f); // level meter display only, 0..1 — not applied to the audio
    }

private:
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    };

    static float runBiquad(Biquad &q, float x) {
        const float y = q.b0 * x + q.z1;
        q.z1 = q.b1 * x - q.a1 * y + q.z2;
        q.z2 = q.b2 * x - q.a2 * y;
        return y;
    }

    static float clampf(float v, float lo, float hi) {
        return std::min(std::max(v, lo), hi);
    }

    // RBJ peaking EQ
    static void designPeaking(Biquad &q, float freq, float db, float Q, int sampleRate) {
        const float A = std::pow(10.0f, db / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / (float)sampleRate;
        const float alpha = std::sin(w0) / (2.0f * Q);
        const float cs = std::cos(w0);
        const float a0 = 1.0f + alpha / A;
        q.b0 = (1.0f + alpha * A) / a0;
        q.b1 = (-2.0f * cs) / a0;
        q.b2 = (1.0f - alpha * A) / a0;
        q.a1 = (-2.0f * cs) / a0;
        q.a2 = (1.0f - alpha / A) / a0;
    }

    void resetState() {
        _hpPrevIn = _hpPrevOut = 0.0f;
        _gain = 1.0f;
        _env = 0.0f;
        _gate = 1.0f;
        _deHissEnv = 0.0f;
        _exPrevIn = 0.0f;
        _exPrevOut = 0.0f;
        for (auto &q : _bq) {
            q.z1 = q.z2 = 0.0f;
        }
        for (auto &q : _clarityBq) {
            q.z1 = q.z2 = 0.0f;
        }
    }

    void updateEq(const FightModeParams &p, int sampleRate) {
        static const float kFreq[kBands] = {60.f, 400.f, 1000.f, 2500.f, 6300.f, 16000.f};
        for (int b = 0; b < kBands; b++) {
            const float db = clampf(p.eqDb[b], -kMaxEqDb, kMaxEqDb);
            const bool active = std::fabs(db) >= 0.05f && kFreq[b] < 0.45f * (float)sampleRate;
            if (!active) {
                _bandActive[b] = false;
                _cachedDb[b] = 0.0f;
                _bq[b].z1 = _bq[b].z2 = 0.0f;
                continue;
            }
            if (_bandActive[b] && _cachedDb[b] == db && _cachedRate == sampleRate) {
                continue;
            }
            designPeaking(_bq[b], kFreq[b], db, 1.0f, sampleRate);
            _bandActive[b] = true;
            _cachedDb[b] = db;
        }
        _cachedRate = sampleRate;
    }

    void updateClarity(float clarity, int sampleRate) {
        if (clarity < 0.01f) {
            _clarityActive = false;
            _cachedClarity = 0.0f;
            _clarityBq[0].z1 = _clarityBq[0].z2 = _clarityBq[1].z1 = _clarityBq[1].z2 = 0.0f;
            return;
        }
        if (_clarityActive && _cachedClarity == clarity && _clarityRate == sampleRate) {
            return;
        }
        designPeaking(_clarityBq[0], 250.0f, -4.0f * clarity, 0.8f, sampleRate);   // less mud / boominess
        designPeaking(_clarityBq[1], 3200.0f, 9.0f * clarity, 0.9f, sampleRate);   // presence: words cut through
        _clarityActive = true;
        _cachedClarity = clarity;
        _clarityRate = sampleRate;
    }

    // high-shelf used by broadcast's de-hiss (fixed design: only the wet/dry mix changes per sample, which is cheap)
    void updateDeHiss(int sampleRate) {
        if (_deHissRate == sampleRate) {
            return;
        }
        designHighShelf(_deHissBq, 4500.0f, -14.0f, 0.85f, sampleRate);
        _deHissRate = sampleRate;
    }

    // RBJ high-shelf
    static void designHighShelf(Biquad &q, float freq, float db, float S, int sampleRate) {
        const float A = std::pow(10.0f, db / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / (float)sampleRate;
        const float cs = std::cos(w0);
        const float sn = std::sin(w0);
        const float alpha = sn / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);
        const float twoSqrtAAlpha = 2.0f * std::sqrt(A) * alpha;
        const float a0 = (A + 1.0f) - (A - 1.0f) * cs + twoSqrtAAlpha;
        q.b0 = (A * ((A + 1.0f) + (A - 1.0f) * cs + twoSqrtAAlpha)) / a0;
        q.b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs)) / a0;
        q.b2 = (A * ((A + 1.0f) + (A - 1.0f) * cs - twoSqrtAAlpha)) / a0;
        q.a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cs)) / a0;
        q.a2 = ((A + 1.0f) - (A - 1.0f) * cs - twoSqrtAAlpha) / a0;
    }

    float _mix = 0.0f;
    float _gain = 1.0f;
    float _volume = 1.0f;
    float _hpPrevIn = 0.0f;
    float _hpPrevOut = 0.0f;
    float _env = 0.0f;
    float _gate = 1.0f;
    Biquad _bq[kBands];
    Biquad _clarityBq[2];
    bool _bandActive[kBands] = {false, false, false, false, false, false};
    float _cachedDb[kBands] = {0, 0, 0, 0, 0, 0};
    int _cachedRate = 0;
    bool _clarityActive = false;
    float _cachedClarity = 0.0f;
    int _clarityRate = 0;
    Biquad _deHissBq;
    int _deHissRate = 0;
    float _deHissEnv = 0.0f;
    float _exPrevIn = 0.0f;
    float _exPrevOut = 0.0f;
};

} // namespace tgcalls

#endif
