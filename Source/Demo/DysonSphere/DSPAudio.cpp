#include "DSPAudio.h"
#include "Math/Math.h"

namespace DSP {

using namespace Aether;

void DSPAudioManager::Init()
{
    Engine::Audio::SetMusicTrack(Engine::MusicTrack::DysonSpaceAmbient);
    Engine::Audio::StartMusic(0);
}

void DSPAudioManager::Update(f32 dt, const DSPCamera& camera, f32 factoryActivity)
{
    (void)dt;
    f32 dist = camera.ZoomDistance();
    if (dist < 300.0f)
    {
        // Planetary surface - high industrial rhythm
        Engine::Audio::SetVacuumFilter(0.0f);
        Engine::Audio::SetMusicMode(2);
        i32 intensity = factoryActivity > 20.0f ? 2 : (factoryActivity > 5.0f ? 1 : 0);
        Engine::Audio::SetMusicIntensity(intensity);
    }
    else
    {
        // Space vacuum - deep ethereal soundscape
        f32 vacuumFactor = Math::Clamp((dist - 300.0f) / 1000.0f, 0.0f, 1.0f);
        Engine::Audio::SetVacuumFilter(vacuumFactor);
        Engine::Audio::SetMusicMode(1);
        Engine::Audio::SetMusicIntensity(0);
    }
}

void DSPAudioManager::PlayClick()
{
    Engine::Audio::Play(Engine::Sfx::Click);
}

void DSPAudioManager::PlayBuild()
{
    Engine::Audio::Play(Engine::Sfx::Build);
}

void DSPAudioManager::PlayDismantle()
{
    Engine::Audio::Play(Engine::Sfx::Dismantle);
}

void DSPAudioManager::PlayPowerConnect()
{
    Engine::Audio::Play(Engine::Sfx::PowerConnect);
}

void DSPAudioManager::PlayDroneLaunch()
{
    Engine::Audio::Play(Engine::Sfx::DroneLaunch);
}

void DSPAudioManager::PlayWarp()
{
    Engine::Audio::Play(Engine::Sfx::Warp);
}

void DSPAudioManager::PlayTechUnlock()
{
    Engine::Audio::Play(Engine::Sfx::TechUnlock);
}

void DSPAudioManager::PlayRocketLaunch()
{
    Engine::Audio::Play(Engine::Sfx::RocketLaunch);
}

}
