#pragma once

#include "Core.h"

namespace Aether::Engine {

enum class Sfx
{
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

enum class MusicTrack
{
    FlightShmup = 0,       // Energetic retro arcade shmup soundtrack
    DysonSpaceAmbient = 1, // Deep cosmic ambient pads & celestial twinkle
    DysonIndustrial = 2,   // Rhythmic factory groove
};

class Audio
{
public:
    static void Init();
    static void Unlock();
    static bool Available();
    static void Play(Sfx sfx);
    static void StartMusic(i32 intensity = 0);
    static void SetMusicIntensity(i32 intensity);
    static void SetMusicTrack(MusicTrack track);
    static void SetMusicMode(i32 mode); // 0 = flight shmup, 1 = dsp ambient, 2 = dsp industrial
    static void SetVacuumFilter(f32 factor); // 0.0 = full air, 1.0 = deep space vacuum lowpass
    static void StopMusic();
};

}
