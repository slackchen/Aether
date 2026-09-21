#pragma once

#include "Core.h"
#include "Math/Vec3.h"
#include "Container/Array.h"

namespace DSP {

using Aether::f32;
using Aether::u32;
using Aether::Math::Vec3;
using Aether::Array;

struct SolarSailParticle {
    Vec3 Pos{0.0f, 0.0f, 0.0f};
    Vec3 Vel{0.0f, 0.0f, 0.0f};
    f32 OrbitRadius = 1200.0f;
    f32 Angle = 0.0f;
    f32 Inclination = 0.35f;
    f32 Life = 1200.0f; // Seconds remaining
    f32 MaxLife = 1200.0f;
    bool Absorbed = false;
};

struct DysonNode {
    u32 Id = 0;
    Vec3 Pos{0.0f, 0.0f, 0.0f}; // Position on Dyson Sphere shell
    u32 RocketsInvested = 0;
    u32 RocketsRequired = 30;
    bool Completed = false;
    f32 EnergyOutputMW = 96.0f;
};

struct DysonStrut {
    u32 NodeA = 0;
    u32 NodeB = 0;
    f32 Progress = 0.0f;
    bool Completed = false;
};

struct DysonShellPanel {
    u32 NodeIndices[3]; // Triangle cell on geodesic sphere
    u32 SailsAbsorbed = 0;
    u32 SailsRequired = 120;
    f32 FillRatio = 0.0f;
    bool Completed = false;
};

class DysonSphereManager {
public:
    DysonSphereManager();

    void Update(f32 dt, u32 newLaunchedSails, u32 newLaunchedRockets);

    // Swarm
    u32 SailCount() const { return mSails.Count(); }
    f32 SwarmGenerationGW() const { return mSwarmGenGW; }
    const Array<SolarSailParticle>& Sails() const { return mSails; }

    // Shell
    u32 NodeCount() const { return mNodes.Count(); }
    u32 CompletedNodes() const;
    f32 ShellGenerationGW() const { return mShellGenGW; }
    f32 TotalGenerationGW() const { return mSwarmGenGW + mShellGenGW; }

    const Array<DysonNode>& Nodes() const { return mNodes; }
    const Array<DysonStrut>& Struts() const { return mStruts; }
    const Array<DysonShellPanel>& Panels() const { return mPanels; }

    f32 SphereRadius() const { return mSphereRadius; }

private:
    void InitGeodesicShell();
    void SpawnSails(u32 count);
    void InvestRockets(u32 count);

    f32 mSphereRadius = 1400.0f;
    Array<SolarSailParticle> mSails;
    Array<DysonNode> mNodes;
    Array<DysonStrut> mStruts;
    Array<DysonShellPanel> mPanels;

    f32 mSwarmGenGW = 0.0f;
    f32 mShellGenGW = 0.0f;
};

}
