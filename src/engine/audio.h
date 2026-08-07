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
};

class Audio {
public:
    static void init();
    static void unlock();
    static bool available();
    static void play(Sfx sfx);
    static void start_music(i32 intensity = 0);
    static void set_music_intensity(i32 intensity);
    static void stop_music();
};

}
