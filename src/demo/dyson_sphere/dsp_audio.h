#pragma once

#include "core/platform.h"
#include "engine/audio.h"
#include "demo/dyson_sphere/universe.h"

namespace dsp {

using namespace aether;

class DSPAudioManager {
public:
    static void init();
    static void update(f32 dt, const DSPCamera& camera, f32 factory_activity);

    static void play_click();
    static void play_build();
    static void play_dismantle();
    static void play_power_connect();
    static void play_drone_launch();
    static void play_warp();
    static void play_tech_unlock();
    static void play_rocket_launch();
};

}
