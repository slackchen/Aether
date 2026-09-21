#pragma once

#include "Core.h"
#include "Audio.h"
#include "Universe.h"

namespace DSP {

using Aether::f32;

class DSPAudioManager {
public:
    static void Init();
    static void Update(f32 dt, const DSPCamera& camera, f32 factoryActivity);

    static void PlayClick();
    static void PlayBuild();
    static void PlayDismantle();
    static void PlayPowerConnect();
    static void PlayDroneLaunch();
    static void PlayWarp();
    static void PlayTechUnlock();
    static void PlayRocketLaunch();
};

}
