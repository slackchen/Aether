#include "demo/dyson_sphere/dsp_audio.h"
#include <algorithm>

namespace dsp {

void DSPAudioManager::init() {
    engine::Audio::set_music_track(engine::MusicTrack::DysonSpaceAmbient);
    engine::Audio::start_music(0);
}

void DSPAudioManager::update(f32 dt, const DSPCamera& camera, f32 factory_activity) {
    (void)dt;
    f32 dist = camera.zoom_distance();
    if (dist < 300.0f) {
        // Planetary surface - high industrial rhythm
        engine::Audio::set_vacuum_filter(0.0f);
        engine::Audio::set_music_mode(2);
        i32 intensity = factory_activity > 20.0f ? 2 : (factory_activity > 5.0f ? 1 : 0);
        engine::Audio::set_music_intensity(intensity);
    } else {
        // Space vacuum - deep ethereal soundscape
        f32 vacuum_factor = std::clamp((dist - 300.0f) / 1000.0f, 0.0f, 1.0f);
        engine::Audio::set_vacuum_filter(vacuum_factor);
        engine::Audio::set_music_mode(1);
        engine::Audio::set_music_intensity(0);
    }
}

void DSPAudioManager::play_click() {
    engine::Audio::play(engine::Sfx::Click);
}

void DSPAudioManager::play_build() {
    engine::Audio::play(engine::Sfx::Build);
}

void DSPAudioManager::play_dismantle() {
    engine::Audio::play(engine::Sfx::Dismantle);
}

void DSPAudioManager::play_power_connect() {
    engine::Audio::play(engine::Sfx::PowerConnect);
}

void DSPAudioManager::play_drone_launch() {
    engine::Audio::play(engine::Sfx::DroneLaunch);
}

void DSPAudioManager::play_warp() {
    engine::Audio::play(engine::Sfx::Warp);
}

void DSPAudioManager::play_tech_unlock() {
    engine::Audio::play(engine::Sfx::TechUnlock);
}

void DSPAudioManager::play_rocket_launch() {
    engine::Audio::play(engine::Sfx::RocketLaunch);
}

}
