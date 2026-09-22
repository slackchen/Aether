#include "Audio.h"
#include "Container/Array.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "Math/Math.h"
#include "Random.h"
#include "Threading/Atomic.h"
#include "Threading/Event.h"
#include "Threading/Mutex.h"
#include "Threading/Thread.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/em_js.h>
#else
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#pragma comment(lib, "ole32.lib")
#endif

using namespace Aether;
using Aether::Engine::MusicTrack;
using Aether::Engine::Sfx;

// ============================================================
// Shared synth engine (platform independent)
// ============================================================

namespace {

u32 gSampleRate = 44100;
constexpr float STEP_SECONDS = 60.0f / 148.0f / 4.0f;
u32 StepFrames() { return (u32)(STEP_SECONDS * (float)gSampleRate); }

Atomic<i32> gMusicIntensity{0};

struct MusicBlock
{
    Array<float> Data;
    u32 Pos = 0;
};
Platform::Mutex gMusicMutex;
Array<MusicBlock> gMusicQueue;
bool gMusicOn = false;
int gMusicStep = 0;

constexpr int NUM_SFX_SLOTS = 8;
struct SfxSlot
{
    Array<float> Data;
    u32 Pos = 0;
};
SfxSlot gSfx[NUM_SFX_SLOTS];
Platform::Mutex gSfxMutex;

struct Biquad
{
    double B0 = 0, B1 = 0, B2 = 0, A1 = 0, A2 = 0;
    double Z1 = 0, Z2 = 0;

    void Lowpass(double fc)
    {
        double w0 = 2.0 * 3.14159265358979 * fc / gSampleRate;
        double alpha = std::sin(w0) / (2.0 * 1.0);
        double c = std::cos(w0);
        double a0 = 1.0 + alpha;
        B0 = (1.0 - c) / (2.0 * a0);
        B1 = (1.0 - c) / a0;
        B2 = B0;
        A1 = (-2.0 * c) / a0;
        A2 = (1.0 - alpha) / a0;
    }
    void Highpass(double fc)
    {
        double w0 = 2.0 * 3.14159265358979 * fc / gSampleRate;
        double alpha = std::sin(w0) / (2.0 * 1.0);
        double c = std::cos(w0);
        double a0 = 1.0 + alpha;
        B0 = (1.0 + c) / (2.0 * a0);
        B1 = -(1.0 + c) / a0;
        B2 = B0;
        A1 = (-2.0 * c) / a0;
        A2 = (1.0 - alpha) / a0;
    }
    void Bandpass(double fc)
    {
        double w0 = 2.0 * 3.14159265358979 * fc / gSampleRate;
        double alpha = std::sin(w0) / (2.0 * 1.0);
        double a0 = 1.0 + alpha;
        B0 = alpha / a0;
        B1 = 0.0;
        B2 = -alpha / a0;
        A1 = (-2.0 * std::cos(w0)) / a0;
        A2 = (1.0 - alpha) / a0;
    }
    double Process(double x)
    {
        double y = B0 * x + Z1;
        Z1 = B1 * x - A1 * y + Z2;
        Z2 = B2 * x - A2 * y;
        return y;
    }
};

double PolyBlep(double t, double dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    else if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

void SynthTone(Array<float>& out, float freq, float dur, int wave, float gain,
               float slideTo, float delay)
{
    int ns = (int)(dur * (float)gSampleRate);
    int skip = (int)(delay * (float)gSampleRate);
    if (skip + ns > (int)out.Count()) out.Resize(skip + ns, 0.0f);
    double phase = 0.0;
    double rate = -std::log(0.0001) / dur;
    const double twoPi = 6.28318530717959;
    for (int i = 0; i < ns; i++)
    {
        double ffrac = (double)i / (double)ns;
        double fnow = freq * std::pow(slideTo / freq, ffrac);
        phase += twoPi * fnow / (double)gSampleRate;
        double dt = fnow / (double)gSampleRate;
        double ph = std::fmod(phase / twoPi, 1.0);
        double s = 0.0;
        switch (wave)
        {
            case 0: s = std::sin(phase); break;
            case 1: s = (ph < 0.5) ? 1.0 : -1.0; s += PolyBlep(ph, dt) - PolyBlep(std::fmod(ph + 0.5, 1.0), dt); break;
            case 2: s = 2.0 * ph - 1.0; s -= PolyBlep(ph, dt); break;
            case 3: s = 2.0 / 3.14159265358979 * std::asin(std::sin(phase)); break;
        }
        double tsec = (double)i / (double)gSampleRate;
        out[skip + i] += (float)(s * gain * std::exp(-rate * tsec));
    }
}

void SynthNoise(Array<float>& out, float dur, float gain, float fc, int filter, float delay)
{
    int ns = (int)(dur * (float)gSampleRate);
    int skip = (int)(delay * (float)gSampleRate);
    if (skip + ns > (int)out.Count()) out.Resize(skip + ns, 0.0f);
    Biquad bq;
    if (filter == 1) bq.Bandpass(fc);
    else if (filter == 2) bq.Highpass(fc);
    else bq.Lowpass(fc);
    static Random sRng(0x5eedull);
    for (int i = 0; i < ns; i++)
    {
        double x = sRng.Range(-1.0, 1.0) * (1.0 - (double)i / ns);
        out[skip + i] += (float)(bq.Process(x) * gain);
    }
}

void SynthSfx(Sfx sfx, Array<float>& out)
{
    switch (sfx)
    {
        case Sfx::Laser:
            SynthTone(out, 920.0f, 0.12f, 2, 0.22f, 240.0f, 0.0f);
            SynthTone(out, 1840.0f, 0.07f, 1, 0.08f, 460.0f, 0.0f);
            break;
        case Sfx::Explosion:
            SynthNoise(out, 0.7f, 0.7f, 400.0f, 0, 0.0f);
            SynthTone(out, 130.0f, 0.55f, 3, 0.4f, 40.0f, 0.0f);
            SynthNoise(out, 0.4f, 0.4f, 2400.0f, 1, 0.02f);
            break;
        case Sfx::Hit:
            SynthTone(out, 640.0f, 0.05f, 1, 0.14f, 320.0f, 0.0f);
            break;
        case Sfx::Powerup:
            SynthTone(out, 660.0f, 0.1f, 1, 0.18f, 660.0f, 0.0f);
            SynthTone(out, 990.0f, 0.1f, 1, 0.18f, 990.0f, 0.09f);
            SynthTone(out, 1320.0f, 0.22f, 1, 0.18f, 1320.0f, 0.18f);
            break;
        case Sfx::Ui:
            SynthTone(out, 520.0f, 0.06f, 1, 0.13f, 520.0f, 0.0f);
            break;
        case Sfx::Warning:
            SynthTone(out, 440.0f, 0.28f, 2, 0.2f, 330.0f, 0.0f);
            SynthTone(out, 440.0f, 0.28f, 2, 0.2f, 330.0f, 0.38f);
            break;
        case Sfx::Death:
            SynthTone(out, 420.0f, 0.8f, 2, 0.28f, 50.0f, 0.0f);
            SynthNoise(out, 0.9f, 0.5f, 500.0f, 0, 0.0f);
            break;
        case Sfx::Click:
            SynthTone(out, 1400.0f, 0.03f, 0, 0.15f, 1800.0f, 0.0f);
            break;
        case Sfx::Build:
            SynthTone(out, 440.0f, 0.08f, 1, 0.18f, 880.0f, 0.0f);
            SynthTone(out, 660.0f, 0.12f, 0, 0.15f, 1320.0f, 0.03f);
            break;
        case Sfx::Dismantle:
            SynthTone(out, 680.0f, 0.09f, 2, 0.15f, 240.0f, 0.0f);
            SynthNoise(out, 0.12f, 0.12f, 800.0f, 0, 0.0f);
            break;
        case Sfx::PowerConnect:
            SynthTone(out, 330.0f, 0.22f, 1, 0.18f, 660.0f, 0.0f);
            SynthTone(out, 660.0f, 0.18f, 2, 0.12f, 1320.0f, 0.04f);
            break;
        case Sfx::DroneLaunch:
            SynthTone(out, 550.0f, 0.30f, 0, 0.20f, 1650.0f, 0.0f);
            SynthNoise(out, 0.25f, 0.10f, 3200.0f, 1, 0.05f);
            break;
        case Sfx::Warp:
            SynthTone(out, 120.0f, 0.70f, 3, 0.35f, 750.0f, 0.0f);
            SynthTone(out, 750.0f, 0.50f, 1, 0.25f, 2200.0f, 0.15f);
            SynthNoise(out, 0.60f, 0.20f, 1100.0f, 0, 0.10f);
            break;
        case Sfx::TechUnlock:
            SynthTone(out, 523.25f, 0.16f, 1, 0.18f, 523.25f, 0.0f);
            SynthTone(out, 659.25f, 0.16f, 1, 0.18f, 659.25f, 0.08f);
            SynthTone(out, 783.99f, 0.20f, 1, 0.20f, 783.99f, 0.16f);
            SynthTone(out, 1046.50f, 0.30f, 0, 0.25f, 1046.50f, 0.24f);
            break;
        case Sfx::RocketLaunch:
            SynthNoise(out, 1.10f, 0.55f, 380.0f, 0, 0.0f);
            SynthTone(out, 95.0f, 0.90f, 3, 0.38f, 280.0f, 0.0f);
            break;
    }
}

Atomic<i32> gMusicMode{0};
Atomic<f32> gVacuumFilter{0.0f};

// Mode 0 (Flight Shmup Arcade Music)
constexpr float BASS_ROOTS[4] = {110.0f, 87.31f, 130.81f, 98.0f};
constexpr int BASS_PATTERN[16] = {1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1};
constexpr float LEAD_PATTERN[64] = {
    440, 0, 0, 0, 523.25f, 0, 0, 0, 587.33f, 0, 0, 523.25f, 440, 0, 0, 0,
    440, 0, 0, 0, 523.25f, 0, 0, 0, 659.25f, 0, 587.33f, 0, 523.25f, 0, 440, 0,
    392, 0, 0, 0, 523.25f, 0, 0, 0, 587.33f, 0, 0, 392, 0, 0, 0, 0,
    523.25f, 0, 0, 0, 659.25f, 0, 0, 0, 783.99f, 0, 659.25f, 0, 587.33f, 0, 523.25f, 0,
};

// Mode 1: Deep Ethereal Space Ambient (Dyson Sphere Program)
constexpr float SPACE_PAD_ROOTS[4] = {174.61f, 196.00f, 220.00f, 164.81f};   // F3, G3, A3, E3
constexpr float SPACE_PAD_THIRDS[4] = {220.00f, 246.94f, 261.63f, 196.00f};  // A3, B3, C4, G3
constexpr float SPACE_PAD_FIFTHS[4] = {261.63f, 293.66f, 329.63f, 246.94f};  // C4, D4, E4, B3
constexpr float SPACE_PAD_NINTHS[4] = {392.00f, 440.00f, 493.88f, 369.99f};  // G4, A4, B4, F#4

constexpr float SPACE_TWINKLE[64] = {
    523.25f, 0, 659.25f, 0, 783.99f, 0, 1046.50f, 0, 880.00f, 0, 659.25f, 0, 523.25f, 0, 0, 0,
    587.33f, 0, 783.99f, 0, 880.00f, 0, 1174.66f, 0, 987.77f, 0, 783.99f, 0, 587.33f, 0, 0, 0,
    659.25f, 0, 880.00f, 0, 1046.50f, 0, 1318.51f, 0, 1046.50f, 0, 880.00f, 0, 659.25f, 0, 0, 0,
    493.88f, 0, 659.25f, 0, 783.99f, 0, 987.77f, 0, 880.00f, 0, 659.25f, 0, 493.88f, 0, 0, 0
};

// Mode 2: Planetary Industrial Pulse (DSP Ground Factor)
constexpr float FACTORY_BASS[4] = {55.0f, 65.41f, 73.42f, 48.99f}; // A1, C2, D2, G1

void SynthMusicStep(int step, int intensity, Array<float>& out)
{
    int mode = gMusicMode.LoadRelaxed();
    int bar = (step / 16) % 4;
    int bs = step % 16;
    float gm = intensity >= 1 ? 1.15f : 1.0f;

    if (mode == 0)
    {
        // Mode 0: Flight Shmup Arcade Music
        if (BASS_PATTERN[bs] == 1)
        {
            SynthTone(out, BASS_ROOTS[bar], 0.16f, 3, 0.28f * gm, BASS_ROOTS[bar], 0.0f);
            SynthTone(out, BASS_ROOTS[bar] * 2.0f, 0.1f, 1, 0.05f * gm, BASS_ROOTS[bar] * 2.0f, 0.0f);
        }
        float lf = LEAD_PATTERN[step];
        if (lf > 0)
        {
            SynthTone(out, lf, 0.14f, 1, 0.1f * gm, lf, 0.0f);
        }
        if (intensity >= 1 && step % 2 == 0)
        {
            SynthTone(out, lf > 0 ? lf * 2.0f : 880.0f, 0.05f, 2, 0.04f * gm, lf > 0 ? lf * 2.0f : 880.0f, 0.0f);
        }
        if (bs == 0 || bs == 8)
        {
            SynthTone(out, 150.0f, 0.1f, 0, 0.6f * gm, 45.0f, 0.0f);
        }
        else if (bs == 4 || bs == 12)
        {
            SynthNoise(out, 0.12f, 0.22f * gm, 1800.0f, 1, 0.0f);
        }
        if (bs % 2 == 0)
        {
            SynthNoise(out, 0.04f, 0.08f * gm, 7000.0f, 2, 0.0f);
        }
    }
    else if (mode == 1)
    {
        // Mode 1: Deep Cosmic Space Ambient (Dyson Sphere Program)
        if (bs == 0)
        {
            // Sustained Ambient Chord Pads
            SynthTone(out, SPACE_PAD_ROOTS[bar] * 0.5f, 1.8f, 0, 0.18f * gm, SPACE_PAD_ROOTS[bar] * 0.5f, 0.0f);
            SynthTone(out, SPACE_PAD_ROOTS[bar], 1.6f, 1, 0.12f * gm, SPACE_PAD_ROOTS[bar], 0.0f);
            SynthTone(out, SPACE_PAD_THIRDS[bar], 1.6f, 0, 0.10f * gm, SPACE_PAD_THIRDS[bar], 0.0f);
            SynthTone(out, SPACE_PAD_FIFTHS[bar], 1.5f, 1, 0.08f * gm, SPACE_PAD_FIFTHS[bar], 0.0f);
            SynthTone(out, SPACE_PAD_NINTHS[bar], 1.4f, 0, 0.06f * gm, SPACE_PAD_NINTHS[bar], 0.0f);
        }
        // Gentle Starry Twinkle
        float tw = SPACE_TWINKLE[step];
        if (tw > 0.0f && bs % 2 == 0)
        {
            SynthTone(out, tw, 0.45f, 0, 0.045f * gm, tw, 0.0f);
            SynthTone(out, tw * 1.5f, 0.35f, 1, 0.020f * gm, tw * 1.5f, 0.05f);
        }
        // Deep sub-drone
        if (bs == 0 || bs == 8)
        {
            SynthTone(out, 43.65f, 0.8f, 0, 0.15f, 43.65f, 0.0f);
        }
    }
    else if (mode == 2)
    {
        // Mode 2: Planetary Industrial Groove
        if (bs % 4 == 0)
        {
            SynthTone(out, FACTORY_BASS[bar], 0.22f, 3, 0.22f * gm, FACTORY_BASS[bar], 0.0f);
            SynthTone(out, FACTORY_BASS[bar] * 2.0f, 0.15f, 1, 0.10f * gm, FACTORY_BASS[bar] * 2.0f, 0.0f);
        }
        // Industrial Clock & Hi-tech pulses
        if (bs % 2 == 0)
        {
            SynthNoise(out, 0.03f, 0.04f * gm, 4800.0f, 2, 0.0f);
        }
        if (bs == 4 || bs == 12)
        {
            SynthNoise(out, 0.06f, 0.08f * gm, 1600.0f, 1, 0.0f);
        }
        float tw = SPACE_TWINKLE[step];
        if (tw > 0.0f)
        {
            SynthTone(out, tw * 0.5f, 0.12f, 2, 0.06f * gm, tw * 0.5f, 0.0f);
        }
    }
}

void FillAudio(float* dst, u32 frames)
{
    for (u32 i = 0; i < frames; i++) dst[i] = 0.0f;
    u32 off = 0;
    u32 left = frames;

    {
        Platform::ScopedLock lock(gMusicMutex);
        if (gMusicOn)
        {
            static Array<float> sCarry;
            while (gMusicQueue.Count() < 2)
            {
                MusicBlock& blk = gMusicQueue.EmplaceAdd();
                blk.Pos = 0;
                blk.Data.Resize(StepFrames(), 0.0f);
                u32 carryCount = sCarry.Count() < blk.Data.Count() ? sCarry.Count() : blk.Data.Count();
                for (u32 i = 0; i < carryCount; i++) blk.Data[i] += sCarry[i];
                SynthMusicStep(gMusicStep, gMusicIntensity.LoadRelaxed(), blk.Data);
                gMusicStep = (gMusicStep + 1) % 64;
                if (blk.Data.Count() > StepFrames())
                {
                    sCarry.Clear();
                    for (u32 i = StepFrames(); i < blk.Data.Count(); i++) sCarry.Add(blk.Data[i]);
                    blk.Data.Resize(StepFrames());
                }
                else
                {
                    sCarry.Clear();
                }
            }
            while (left > 0 && !gMusicQueue.IsEmpty())
            {
                MusicBlock& blk = gMusicQueue.First();
                u32 remaining = blk.Data.Count() - blk.Pos;
                u32 take = left < remaining ? left : remaining;
                const float* src = blk.Data.Data() + blk.Pos;
                for (u32 i = 0; i < take; i++) dst[off + i] += src[i];
                blk.Pos += take;
                off += take;
                left -= take;
                if (blk.Pos >= blk.Data.Count()) gMusicQueue.RemoveAt(0);
            }
        }
    }

    {
        Platform::ScopedLock lock(gSfxMutex);
        for (int s = 0; s < NUM_SFX_SLOTS; s++)
        {
            SfxSlot& slot = gSfx[s];
            if (slot.Data.IsEmpty() || slot.Pos >= slot.Data.Count()) continue;
            u32 p = slot.Pos;
            for (u32 i = 0; i < frames && p < slot.Data.Count(); i++)
            {
                dst[i] += slot.Data[p++];
            }
            slot.Pos = p;
        }
    }

    float vac = gVacuumFilter.LoadRelaxed();
    if (vac > 0.01f)
    {
        static float sLpfState = 0.0f;
        float alpha = 1.0f - vac * 0.88f;
        for (u32 i = 0; i < frames; i++)
        {
            sLpfState += alpha * (dst[i] - sLpfState);
            dst[i] = sLpfState;
        }
    }
}

void QueueSfx(Sfx sfx)
{
    Array<float> pcm;
    SynthSfx(sfx, pcm);
    Platform::ScopedLock lock(gSfxMutex);
    int best = 0;
    u32 oldest = 0;
    for (int i = 0; i < NUM_SFX_SLOTS; i++)
    {
        if (gSfx[i].Data.IsEmpty() || gSfx[i].Pos >= gSfx[i].Data.Count())
        {
            best = i;
            break;
        }
        if (gSfx[i].Pos > oldest)
        {
            oldest = gSfx[i].Pos;
            best = i;
        }
    }
    gSfx[best].Data = std::move(pcm);
    gSfx[best].Pos = 0;
}

}  // namespace

namespace Aether::Engine {

#ifdef __EMSCRIPTEN__

// WebAudio backend: ScriptProcessor pulls PCM from the shared synth engine.
EM_JS(void, JsAudioInit, (), {
    if (Module.__aether_ctx) return;
    var AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) return;
var ctx = new AC();
    Module.__aether_ctx = ctx;
    Module.ccall('audio_set_sample_rate', null, ['number'], [ctx.sampleRate]);
    var node = ctx.createScriptProcessor(2048, 0, 1);
    var callCount = 0;
    var peak = 0;
    node.onaudioprocess = function(e) {
        var len = e.outputBuffer.length;
        var ptr = Module.ccall('audio_get_buffer', 'number', ['number'], [len]);
        Module.ccall('audio_fill', null, ['number', 'number'], [ptr, len]);
        var base = ptr >> 2;
        var out = e.outputBuffer.getChannelData(0);
        var heap = HEAPF32;
        peak = 0;
        for (var i = 0; i < len; i++) {
            var v = heap[base + i];
            out[i] = v;
            if (v < 0) v = -v;
            if (v > peak) peak = v;
        }
        callCount++;
        if (callCount === 1 || callCount % 600 === 0) {
            console.log("audio: #" + callCount + " peak=" + peak.toFixed(4));
        }
    };
    node.connect(ctx.destination);
    var resume = function() {
        if (ctx.state === 'suspended') ctx.resume();
        if (ctx.state === 'running') Module.__aether_audio_ready = true;
    };
    document.addEventListener("keydown", resume, { once: true });
    document.addEventListener("pointerdown", resume, { once: true });
    resume();
});

EM_JS(void, JsAudioUnlock, (), {
    var ctx = Module.__aether_ctx;
    if (ctx) {
        if (ctx.state === 'suspended') ctx.resume();
        if (ctx.state === 'running') Module.__aether_audio_ready = true;
    }
});

EM_JS(int, JsAudioAvailable, (), {
    var ctx = Module.__aether_ctx;
    return (ctx && (ctx.state === 'running' || Module.__aether_audio_ready)) ? 1 : 0;
});

extern "C" {
// NOTE: These symbol names are referenced from the EM_JS bodies above via
// Module.ccall('audio_set_sample_rate' / 'audio_get_buffer' / 'audio_fill'),
// so they are intentionally kept in snake_case (C ABI, not C++ style).
EMSCRIPTEN_KEEPALIVE void audio_set_sample_rate(int rate) {
    gSampleRate = rate > 0 ? (u32)rate : 44100u;
}
EMSCRIPTEN_KEEPALIVE float* audio_get_buffer(int frames) {
    static Array<float> sScratch;
    if ((int)sScratch.Count() < frames) sScratch.Resize((u32)frames);
    return sScratch.Data();
}
EMSCRIPTEN_KEEPALIVE void audio_fill(float* out, int frames) {
    FillAudio(out, (u32)frames);
}
}

#else

// WASAPI backend: event-driven render thread pulls PCM from the shared engine.
namespace {

IAudioClient* gClient = nullptr;
IAudioRenderClient* gRenderClient = nullptr;
Platform::Event gAudioEvent;
UINT32 gBufferFrames = 0;
Platform::Thread gAudioThread;
Atomic<bool> gAudioRunning{false};
bool gAudioOk = false;

enum class OutFormat
{
    Float32,
    Int16,
    Int32,
};
OutFormat gOutFormat = OutFormat::Float32;
u32 gOutChannels = 2;

struct AudioShutdown
{
    ~AudioShutdown()
    {
        gAudioRunning.StoreRelaxed(false);
        if (gAudioThread.Joinable()) gAudioThread.Join();
    }
};
AudioShutdown gAudioShutdown;

void WriteOut(float* mono, u32 frames, BYTE* raw)
{
    if (gOutFormat == OutFormat::Float32)
    {
        float* d = (float*)raw;
        for (u32 i = 0; i < frames; i++)
        {
            float s = mono[i] * 0.5f;
            for (u32 c = 0; c < gOutChannels; c++) d[i * gOutChannels + c] = s;
        }
    }
    else if (gOutFormat == OutFormat::Int16)
    {
        short* d = (short*)raw;
        for (u32 i = 0; i < frames; i++)
        {
            float s = mono[i] * 0.5f;
            short v = (short)(s * 32767.0f);
            for (u32 c = 0; c < gOutChannels; c++) d[i * gOutChannels + c] = v;
        }
    }
    else
    {
        int* d = (int*)raw;
        for (u32 i = 0; i < frames; i++)
        {
            float s = mono[i] * 0.5f;
            int v = (int)(s * 2147483647.0f);
            for (u32 c = 0; c < gOutChannels; c++) d[i * gOutChannels + c] = v;
        }
    }
}

void AudioThreadProc()
{
    // MMCSS "Audio" boost is applied by the Thread layer (ThreadPriority::Audio).
    Array<float> mono;
    while (gAudioRunning.LoadRelaxed())
    {
        if (!gAudioEvent.Wait(1000))
        {
            continue; // timeout, re-check the running flag
        }
        UINT32 pad = 0;
        if (FAILED(gClient->GetCurrentPadding(&pad))) continue;
        UINT32 frames = gBufferFrames - pad;
        if (frames == 0) continue;
        BYTE* data = nullptr;
        if (FAILED(gRenderClient->GetBuffer(frames, &data))) continue;
        if (mono.Count() < frames) mono.Resize(frames);
        FillAudio(mono.Data(), frames);
        WriteOut(mono.Data(), frames, data);
        gRenderClient->ReleaseBuffer(frames, 0);
    }
}

}  // namespace
#endif

void Audio::Init()
{
#ifdef __EMSCRIPTEN__
    JsAudioInit();
#else
    if (gAudioOk) return;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr))
    {
        printf("Audio: no MMDeviceEnumerator\n");
        return;
    }
    IMMDevice* device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    enumerator->Release();
    if (FAILED(hr))
    {
        printf("Audio: no default render device\n");
        return;
    }
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&gClient);
    device->Release();
    if (FAILED(hr))
    {
        gClient = nullptr;
        printf("Audio: IAudioClient activate failed\n");
        return;
    }

    WAVEFORMATEX* mix = nullptr;
    hr = gClient->GetMixFormat(&mix);
    if (FAILED(hr) || !mix)
    {
        gClient->Release();
        gClient = nullptr;
        printf("Audio: GetMixFormat failed\n");
        return;
    }
    gSampleRate = mix->nSamplesPerSec ? mix->nSamplesPerSec : 44100u;
    gOutChannels = mix->nChannels ? mix->nChannels : 2;
    WAVEFORMATEXTENSIBLE* ext =
        (mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE) ? (WAVEFORMATEXTENSIBLE*)mix : nullptr;
    if (ext)
    {
        if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        {
            gOutFormat = OutFormat::Float32;
        }
        else if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
        {
            gOutFormat = (mix->wBitsPerSample >= 32) ? OutFormat::Int32 : OutFormat::Int16;
        }
        else
        {
            gOutFormat = OutFormat::Int16;
        }
    }
    else
    {
        if (mix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) gOutFormat = OutFormat::Float32;
        else if (mix->wFormatTag == WAVE_FORMAT_PCM) gOutFormat = OutFormat::Int16;
        else gOutFormat = OutFormat::Int16;
    }

    hr = gClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                             0, 0, mix, nullptr);
    CoTaskMemFree(mix);
    if (FAILED(hr))
    {
        gClient->Release();
        gClient = nullptr;
        printf("Audio: Initialize failed hr=0x%08lx\n", (unsigned long)hr);
        return;
    }
    if (!gAudioEvent.NativeHandle())
    {
        gClient->Release();
        gClient = nullptr;
        printf("Audio: event creation failed\n");
        return;
    }
    gClient->SetEventHandle(static_cast<HANDLE>(gAudioEvent.NativeHandle()));
    gClient->GetBufferSize(&gBufferFrames);
    hr = gClient->GetService(IID_PPV_ARGS(&gRenderClient));
    if (FAILED(hr))
    {
        gRenderClient = nullptr;
        gClient->Release();
        gClient = nullptr;
        printf("Audio: GetService failed\n");
        return;
    }

    gAudioRunning.StoreRelaxed(true);
    gAudioThread.Run(AudioThreadProc, "AetherAudio", Platform::ThreadPriority::Audio);
    gAudioThread.Detach();
    hr = gClient->Start();
    if (FAILED(hr))
    {
        gAudioRunning.StoreRelaxed(false);
        printf("Audio: Start failed\n");
        return;
    }
    gAudioOk = true;
    printf("Audio: WASAPI ready %u Hz, %u ch\n", gSampleRate, gOutChannels);
#endif
}

void Audio::Unlock()
{
#ifdef __EMSCRIPTEN__
    JsAudioUnlock();
#endif
}

bool Audio::Available()
{
#ifdef __EMSCRIPTEN__
    return JsAudioAvailable() != 0;
#else
    return gAudioOk;
#endif
}

void Audio::Play(Sfx sfx)
{
    QueueSfx(sfx);
}

void Audio::StartMusic(i32 intensity)
{
    Platform::ScopedLock lock(gMusicMutex);
    gMusicIntensity.StoreRelaxed(intensity);
    if (!gMusicOn)
    {
        gMusicOn = true;
        gMusicStep = 0;
        gMusicQueue.Clear();
    }
}

void Audio::SetMusicIntensity(i32 intensity)
{
    gMusicIntensity.StoreRelaxed(intensity);
}

void Audio::SetMusicTrack(MusicTrack track)
{
    SetMusicMode((i32)track);
}

void Audio::SetMusicMode(i32 mode)
{
    if (gMusicMode.LoadRelaxed() != mode)
    {
        gMusicMode.StoreRelaxed(mode);
        Platform::ScopedLock lock(gMusicMutex);
        gMusicQueue.Clear();
        gMusicStep = 0;
    }
}

void Audio::SetVacuumFilter(f32 factor)
{
    gVacuumFilter.StoreRelaxed(Math::Clamp(factor, 0.0f, 1.0f));
}

void Audio::StopMusic()
{
    Platform::ScopedLock lock(gMusicMutex);
    gMusicOn = false;
    gMusicQueue.Clear();
}

}
