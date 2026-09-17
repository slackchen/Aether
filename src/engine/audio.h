#pragma once

#include "core/platform.h"

namespace aether::engine {

enum class Sfx {
    Laser,
    Explosion,
    Hit,
    Powerup,
    Ui,
    Warning,
    Death,
    // DSP additions:
    Click,
    Build,
    Dismantle,
    PowerConnect,
    DroneLaunch,
    Warp,
    TechUnlock,
    RocketLaunch,
};

enum class MusicTrack {
    FlightShmup = 0,       // Energetic retro arcade shmup soundtrack
    DysonSpaceAmbient = 1, // Deep cosmic ambient pads & celestial twinkle
    DysonIndustrial = 2,   // Rhythmic factory groove
};

class Audio {
public:
    static void init();
    static void unlock();
    static bool available();
    static void play(Sfx sfx);
    static void start_music(i32 intensity = 0);
    static void set_music_intensity(i32 intensity);
    static void set_music_track(MusicTrack track);
    static void set_music_mode(i32 mode); // 0 = flight shmup, 1 = dsp ambient, 2 = dsp industrial
    static void set_vacuum_filter(f32 factor); // 0.0 = full air, 1.0 = deep space vacuum lowpass
    static void stop_music();
};

}
