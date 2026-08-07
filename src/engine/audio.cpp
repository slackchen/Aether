#include "engine/audio.h"

#include <cmath>
#include <vector>
#include <deque>
#include <mutex>
#include <memory>
#include <random>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/em_js.h>
#else
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <thread>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")
#endif

using aether::u32;
using aether::engine::Sfx;

// ============================================================
// Shared synth engine (platform independent)
// ============================================================

namespace {

u32 g_sample_rate = 44100;
constexpr float kStepSeconds = 60.0f / 148.0f / 4.0f;
u32 kStepFrames() { return (u32)(kStepSeconds * (float)g_sample_rate); }

std::atomic<int> g_music_intensity{0};

struct MusicBlock {
    std::vector<float> data;
    size_t pos = 0;
};
std::mutex g_music_mutex;
std::deque<std::shared_ptr<MusicBlock>> g_music_queue;
bool g_music_on = false;
int g_music_step = 0;

constexpr int kSfxSlots = 8;
struct SfxSlot {
    std::shared_ptr<std::vector<float>> data;
    size_t pos = 0;
};
SfxSlot g_sfx[kSfxSlots];
std::mutex g_sfx_mutex;

struct Biquad {
    double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void lowpass(double fc) {
        double w0 = 2.0 * 3.14159265358979 * fc / g_sample_rate;
        double alpha = std::sin(w0) / (2.0 * 1.0);
        double c = std::cos(w0);
        double a0 = 1.0 + alpha;
        b0 = (1.0 - c) / (2.0 * a0);
        b1 = (1.0 - c) / a0;
        b2 = b0;
        a1 = (-2.0 * c) / a0;
        a2 = (1.0 - alpha) / a0;
    }
    void highpass(double fc) {
        double w0 = 2.0 * 3.14159265358979 * fc / g_sample_rate;
        double alpha = std::sin(w0) / (2.0 * 1.0);
        double c = std::cos(w0);
        double a0 = 1.0 + alpha;
        b0 = (1.0 + c) / (2.0 * a0);
        b1 = -(1.0 + c) / a0;
        b2 = b0;
        a1 = (-2.0 * c) / a0;
        a2 = (1.0 - alpha) / a0;
    }
    void bandpass(double fc) {
        double w0 = 2.0 * 3.14159265358979 * fc / g_sample_rate;
        double alpha = std::sin(w0) / (2.0 * 1.0);
        double a0 = 1.0 + alpha;
        b0 = alpha / a0;
        b1 = 0.0;
        b2 = -alpha / a0;
        a1 = (-2.0 * std::cos(w0)) / a0;
        a2 = (1.0 - alpha) / a0;
    }
    double process(double x) {
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

double poly_blep(double t, double dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    } else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

void synth_tone(std::vector<float>& out, float freq, float dur, int wave, float gain,
                float slide_to, float delay) {
    int ns = (int)(dur * (float)g_sample_rate);
    int skip = (int)(delay * (float)g_sample_rate);
    if (skip + ns > (int)out.size()) out.resize(skip + ns, 0.0f);
    double phase = 0.0;
    double rate = -std::log(0.0001) / dur;
    const double two_pi = 6.28318530717959;
    for (int i = 0; i < ns; i++) {
        double ffrac = (double)i / (double)ns;
        double fnow = freq * std::pow(slide_to / freq, ffrac);
        phase += two_pi * fnow / (double)g_sample_rate;
        double dt = fnow / (double)g_sample_rate;
        double ph = std::fmod(phase / two_pi, 1.0);
        double s = 0.0;
        switch (wave) {
            case 0: s = std::sin(phase); break;
            case 1: s = (ph < 0.5) ? 1.0 : -1.0; s += poly_blep(ph, dt) - poly_blep(fmod(ph + 0.5, 1.0), dt); break;
            case 2: s = 2.0 * ph - 1.0; s -= poly_blep(ph, dt); break;
            case 3: s = 2.0 / 3.14159265358979 * std::asin(std::sin(phase)); break;
        }
        double tsec = (double)i / (double)g_sample_rate;
        out[skip + i] += (float)(s * gain * std::exp(-rate * tsec));
    }
}

void synth_noise(std::vector<float>& out, float dur, float gain, float fc, int filter, float delay) {
    int ns = (int)(dur * (float)g_sample_rate);
    int skip = (int)(delay * (float)g_sample_rate);
    if (skip + ns > (int)out.size()) out.resize(skip + ns, 0.0f);
    Biquad bq;
    if (filter == 1) bq.bandpass(fc);
    else if (filter == 2) bq.highpass(fc);
    else bq.lowpass(fc);
    static std::mt19937 rng(0x5eedu);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    for (int i = 0; i < ns; i++) {
        double x = dist(rng) * (1.0 - (double)i / ns);
        out[skip + i] += (float)(bq.process(x) * gain);
    }
}

void synth_sfx(Sfx sfx, std::vector<float>& out) {
    switch (sfx) {
        case Sfx::Laser:
            synth_tone(out, 920.0f, 0.12f, 2, 0.22f, 240.0f, 0.0f);
            synth_tone(out, 1840.0f, 0.07f, 1, 0.08f, 460.0f, 0.0f);
            break;
        case Sfx::Explosion:
            synth_noise(out, 0.7f, 0.7f, 400.0f, 0, 0.0f);
            synth_tone(out, 130.0f, 0.55f, 3, 0.4f, 40.0f, 0.0f);
            synth_noise(out, 0.4f, 0.4f, 2400.0f, 1, 0.02f);
            break;
        case Sfx::Hit:
            synth_tone(out, 640.0f, 0.05f, 1, 0.14f, 320.0f, 0.0f);
            break;
        case Sfx::Powerup:
            synth_tone(out, 660.0f, 0.1f, 1, 0.18f, 660.0f, 0.0f);
            synth_tone(out, 990.0f, 0.1f, 1, 0.18f, 990.0f, 0.09f);
            synth_tone(out, 1320.0f, 0.22f, 1, 0.18f, 1320.0f, 0.18f);
            break;
        case Sfx::Ui:
            synth_tone(out, 520.0f, 0.06f, 1, 0.13f, 520.0f, 0.0f);
            break;
        case Sfx::Warning:
            synth_tone(out, 440.0f, 0.28f, 2, 0.2f, 330.0f, 0.0f);
            synth_tone(out, 440.0f, 0.28f, 2, 0.2f, 330.0f, 0.38f);
            break;
        case Sfx::Death:
            synth_tone(out, 420.0f, 0.8f, 2, 0.28f, 50.0f, 0.0f);
            synth_noise(out, 0.9f, 0.5f, 500.0f, 0, 0.0f);
            break;
    }
}

constexpr float kBassRoots[4] = {110.0f, 87.31f, 130.81f, 98.0f};
constexpr int kBassPat[16] = {1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1};
constexpr float kLead[64] = {
    440, 0, 0, 0, 523.25f, 0, 0, 0, 587.33f, 0, 0, 523.25f, 440, 0, 0, 0,
    440, 0, 0, 0, 523.25f, 0, 0, 0, 659.25f, 0, 587.33f, 0, 523.25f, 0, 440, 0,
    392, 0, 0, 0, 523.25f, 0, 0, 0, 587.33f, 0, 0, 392, 0, 0, 0, 0,
    523.25f, 0, 0, 0, 659.25f, 0, 0, 0, 783.99f, 0, 659.25f, 0, 587.33f, 0, 523.25f, 0,
};

void synth_music_step(int step, int intensity, std::vector<float>& out) {
    int bar = (step / 16) % 4;
    int bs = step % 16;
    float gm = intensity >= 1 ? 1.15f : 1.0f;

    if (kBassPat[bs] == 1) {
        synth_tone(out, kBassRoots[bar], 0.16f, 3, 0.28f * gm, kBassRoots[bar], 0.0f);
        synth_tone(out, kBassRoots[bar] * 2.0f, 0.1f, 1, 0.05f * gm, kBassRoots[bar] * 2.0f, 0.0f);
    }
    float lf = kLead[step];
    if (lf > 0) {
        synth_tone(out, lf, 0.14f, 1, 0.1f * gm, lf, 0.0f);
    }
    if (intensity >= 1 && step % 2 == 0) {
        synth_tone(out, lf > 0 ? lf * 2.0f : 880.0f, 0.05f, 2, 0.04f * gm, lf > 0 ? lf * 2.0f : 880.0f, 0.0f);
    }
    if (bs == 0 || bs == 8) {
        synth_tone(out, 150.0f, 0.1f, 0, 0.6f * gm, 45.0f, 0.0f);
    } else if (bs == 4 || bs == 12) {
        synth_noise(out, 0.12f, 0.22f * gm, 1800.0f, 1, 0.0f);
    }
    if (bs % 2 == 0) {
        synth_noise(out, 0.04f, 0.08f * gm, 7000.0f, 2, 0.0f);
    }
}

void fill_audio(float* dst, u32 frames) {
    std::fill(dst, dst + frames, 0.0f);
    u32 off = 0;
    u32 left = frames;

    {
        std::lock_guard<std::mutex> lk(g_music_mutex);
        if (g_music_on) {
            static std::vector<float> carry;
            while (g_music_queue.size() < 2) {
                auto blk = std::make_shared<MusicBlock>();
                blk->data.resize(kStepFrames(), 0.0f);
                size_t cn = carry.size() < blk->data.size() ? carry.size() : blk->data.size();
                for (size_t i = 0; i < cn; i++) blk->data[i] += carry[i];
                synth_music_step(g_music_step, g_music_intensity.load(), blk->data);
                g_music_step = (g_music_step + 1) % 64;
                if (blk->data.size() > kStepFrames()) {
                    carry.assign(blk->data.begin() + kStepFrames(), blk->data.end());
                    blk->data.resize(kStepFrames());
                } else {
                    carry.clear();
                }
                g_music_queue.push_back(blk);
            }
            while (left > 0 && !g_music_queue.empty()) {
                MusicBlock& blk = *g_music_queue.front();
                u32 take = (u32)std::min<size_t>(left, blk.data.size() - blk.pos);
                const float* src = blk.data.data() + blk.pos;
                for (u32 i = 0; i < take; i++) dst[off + i] += src[i];
                blk.pos += take;
                off += take;
                left -= take;
                if (blk.pos >= blk.data.size()) g_music_queue.pop_front();
            }
        }
    }

    {
        std::lock_guard<std::mutex> lk(g_sfx_mutex);
        for (int s = 0; s < kSfxSlots; s++) {
            SfxSlot& slot = g_sfx[s];
            if (!slot.data || slot.pos >= slot.data->size()) continue;
            size_t p = slot.pos;
            for (u32 i = 0; i < frames && p < slot.data->size(); i++) {
                dst[i] += (*slot.data)[p++];
            }
            slot.pos = p;
        }
    }

    float peak = 0.0f;
    for (u32 i = 0; i < frames; i++) {
        float a = dst[i] < 0.0f ? -dst[i] : dst[i];
        if (a > peak) peak = a;
    }
    (void)peak;
}

void sfx_queue(Sfx sfx) {
    std::vector<float> pcm;
    synth_sfx(sfx, pcm);
    std::lock_guard<std::mutex> lk(g_sfx_mutex);
    int best = 0;
    size_t oldest = 0;
    for (int i = 0; i < kSfxSlots; i++) {
        if (!g_sfx[i].data || g_sfx[i].pos >= g_sfx[i].data->size()) {
            best = i;
            break;
        }
        if (g_sfx[i].pos > oldest) {
            oldest = g_sfx[i].pos;
            best = i;
        }
    }
    g_sfx[best].data = std::make_shared<std::vector<float>>(std::move(pcm));
    g_sfx[best].pos = 0;
}

}  // namespace

namespace aether::engine {

#ifdef __EMSCRIPTEN__

// WebAudio backend: ScriptProcessor pulls PCM from the shared synth engine.
EM_JS(void, js_audio_init, (), {
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

EM_JS(void, js_audio_unlock, (), {
    var ctx = Module.__aether_ctx;
    if (ctx) {
        if (ctx.state === 'suspended') ctx.resume();
        if (ctx.state === 'running') Module.__aether_audio_ready = true;
    }
});

EM_JS(int, js_audio_available, (), {
    var ctx = Module.__aether_ctx;
    return (ctx && (ctx.state === 'running' || Module.__aether_audio_ready)) ? 1 : 0;
});

extern "C" {
EMSCRIPTEN_KEEPALIVE void audio_set_sample_rate(int rate) {
    g_sample_rate = rate > 0 ? (u32)rate : 44100u;
}
EMSCRIPTEN_KEEPALIVE float* audio_get_buffer(int frames) {
    static std::vector<float> scratch;
    if ((int)scratch.size() < frames) scratch.resize(frames);
    return scratch.data();
}
EMSCRIPTEN_KEEPALIVE void audio_fill(float* out, int frames) {
    fill_audio(out, (u32)frames);
}
}

#else

// WASAPI backend: event-driven render thread pulls PCM from the shared engine.
namespace {

IAudioClient* g_client = nullptr;
IAudioRenderClient* g_render = nullptr;
HANDLE g_audio_event = nullptr;
UINT32 g_buffer_frames = 0;
std::thread g_audio_thread;
std::atomic<bool> g_audio_running{false};
bool g_audio_ok = false;

enum class OutFmt { Float32, Int16, Int32 };
OutFmt g_out_fmt = OutFmt::Float32;
u32 g_out_ch = 2;

struct AudioShutdown {
    ~AudioShutdown() {
        g_audio_running = false;
        if (g_audio_thread.joinable()) g_audio_thread.join();
    }
};
AudioShutdown g_audio_shutdown;

void write_out(float* mono, u32 frames, BYTE* raw) {
    if (g_out_fmt == OutFmt::Float32) {
        float* d = (float*)raw;
        for (u32 i = 0; i < frames; i++) {
            float s = mono[i] * 0.5f;
            for (u32 c = 0; c < g_out_ch; c++) d[i * g_out_ch + c] = s;
        }
    } else if (g_out_fmt == OutFmt::Int16) {
        short* d = (short*)raw;
        for (u32 i = 0; i < frames; i++) {
            float s = mono[i] * 0.5f;
            short v = (short)(s * 32767.0f);
            for (u32 c = 0; c < g_out_ch; c++) d[i * g_out_ch + c] = v;
        }
    } else {
        int* d = (int*)raw;
        for (u32 i = 0; i < frames; i++) {
            float s = mono[i] * 0.5f;
            int v = (int)(s * 2147483647.0f);
            for (u32 c = 0; c < g_out_ch; c++) d[i * g_out_ch + c] = v;
        }
    }
}

void audio_thread() {
    DWORD mmcss_index = 0;
    HANDLE mmcss = AvSetMmThreadCharacteristicsW(L"Audio", &mmcss_index);
    std::vector<float> mono;
    while (g_audio_running.load()) {
        DWORD w = WaitForSingleObject(g_audio_event, 1000);
        if (w == WAIT_TIMEOUT) continue;
        if (w != WAIT_OBJECT_0) break;
        UINT32 pad = 0;
        if (FAILED(g_client->GetCurrentPadding(&pad))) continue;
        UINT32 frames = g_buffer_frames - pad;
        if (frames == 0) continue;
        BYTE* data = nullptr;
        if (FAILED(g_render->GetBuffer(frames, &data))) continue;
        if (mono.size() < frames) mono.resize(frames);
        fill_audio(mono.data(), frames);
        write_out(mono.data(), frames, data);
        g_render->ReleaseBuffer(frames, 0);
    }
    if (mmcss) AvRevertMmThreadCharacteristics(mmcss);
}

}  // namespace
#endif

void Audio::init() {
#ifdef __EMSCRIPTEN__
    js_audio_init();
#else
    if (g_audio_ok) return;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr)) {
        printf("Audio: no MMDeviceEnumerator\n");
        return;
    }
    IMMDevice* device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    enumerator->Release();
    if (FAILED(hr)) {
        printf("Audio: no default render device\n");
        return;
    }
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&g_client);
    device->Release();
    if (FAILED(hr)) {
        g_client = nullptr;
        printf("Audio: IAudioClient activate failed\n");
        return;
    }

    WAVEFORMATEX* mix = nullptr;
    hr = g_client->GetMixFormat(&mix);
    if (FAILED(hr) || !mix) {
        g_client->Release();
        g_client = nullptr;
        printf("Audio: GetMixFormat failed\n");
        return;
    }
    g_sample_rate = mix->nSamplesPerSec ? mix->nSamplesPerSec : 44100u;
    g_out_ch = mix->nChannels ? mix->nChannels : 2;
    WAVEFORMATEXTENSIBLE* ext =
        (mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE) ? (WAVEFORMATEXTENSIBLE*)mix : nullptr;
    if (ext) {
        if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            g_out_fmt = OutFmt::Float32;
        } else if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
            g_out_fmt = (mix->wBitsPerSample >= 32) ? OutFmt::Int32 : OutFmt::Int16;
        } else {
            g_out_fmt = OutFmt::Int16;
        }
    } else {
        if (mix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) g_out_fmt = OutFmt::Float32;
        else if (mix->wFormatTag == WAVE_FORMAT_PCM) g_out_fmt = OutFmt::Int16;
        else g_out_fmt = OutFmt::Int16;
    }

    hr = g_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                              0, 0, mix, nullptr);
    CoTaskMemFree(mix);
    if (FAILED(hr)) {
        g_client->Release();
        g_client = nullptr;
        printf("Audio: Initialize failed hr=0x%08lx\n", (unsigned long)hr);
        return;
    }
    g_audio_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!g_audio_event) {
        g_client->Release();
        g_client = nullptr;
        printf("Audio: CreateEvent failed\n");
        return;
    }
    g_client->SetEventHandle(g_audio_event);
    g_client->GetBufferSize(&g_buffer_frames);
    hr = g_client->GetService(IID_PPV_ARGS(&g_render));
    if (FAILED(hr)) {
        g_render = nullptr;
        CloseHandle(g_audio_event);
        g_audio_event = nullptr;
        g_client->Release();
        g_client = nullptr;
        printf("Audio: GetService failed\n");
        return;
    }

    g_audio_running = true;
    g_audio_thread = std::thread(audio_thread);
    g_audio_thread.detach();
    hr = g_client->Start();
    if (FAILED(hr)) {
        g_audio_running = false;
        printf("Audio: Start failed\n");
        return;
    }
    g_audio_ok = true;
    printf("Audio: WASAPI ready %u Hz, %u ch\n", g_sample_rate, g_out_ch);
#endif
}

void Audio::unlock() {
#ifdef __EMSCRIPTEN__
    js_audio_unlock();
#endif
}

bool Audio::available() {
#ifdef __EMSCRIPTEN__
    return js_audio_available() != 0;
#else
    return g_audio_ok;
#endif
}

void Audio::play(Sfx sfx) {
    sfx_queue(sfx);
}

void Audio::start_music(i32 intensity) {
    std::lock_guard<std::mutex> lk(g_music_mutex);
    g_music_intensity.store((int)intensity);
    if (!g_music_on) {
        g_music_on = true;
        g_music_step = 0;
        g_music_queue.clear();
    }
}

void Audio::set_music_intensity(i32 intensity) {
    g_music_intensity.store((int)intensity);
}

void Audio::stop_music() {
    std::lock_guard<std::mutex> lk(g_music_mutex);
    g_music_on = false;
    g_music_queue.clear();
}

}
