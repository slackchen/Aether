#pragma once

#include "Core.h"
#include "Math/Vec2.h"
#include "Math/Vec3.h"
#include "Planet.h"
#include "Container/Array.h"

namespace DSP {

using Aether::f32;
using Aether::u32;
using Aether::Math::Vec2;
using Aether::Math::Vec3;
using Aether::Array;

enum class MechaMode {
    GroundWalk,
    LowFlight,
    SpaceSailing,
    Warping,
};

enum class FuelKind {
    None = 0,
    Coal,
    HydrogenFuelRod,
    DeuteronFuelRod,
    AntimatterFuelRod,
};

struct FuelItem {
    FuelKind Kind = FuelKind::None;
    u32 Count = 0;
    f32 EnergyPerItemMJ = 0.0f;
    f32 CombustionEfficiency = 1.0f;
};

struct ConstructionDrone {
    Vec3 Pos{0.0f, 0.0f, 0.0f};
    Vec3 TargetPos{0.0f, 0.0f, 0.0f};
    f32 Speed = 25.0f;
    bool Active = false;
    bool Returning = false;
    u32 TargetId = 0;
    f32 BeamTimer = 0.0f;
};

class Mecha {
public:
    Mecha();

    void Update(f32 dt, Planet* currentPlanet, bool spaceMode);

    // Unified 3D Spherical & Flight Movement (方向始终以相机为准, 球面切向移动)
    void Move3D(const Vec2& inputWasd, const Vec3& camRight, f32 altInput, f32 dt, Planet* planet);
    void MoveGround(const Vec2& dir, f32 dt, Planet* planet);
    void MoveFlight(const Vec2& dir, f32 altInput, f32 dt, Planet* planet);
    void MoveSpace(const Vec3& thrustDir, f32 dt);
    void ToggleFlight();
    void ToggleWarp();

    // Energy & Fuel
    void ConsumeEnergy(f32 amountMJ);
    void AddFuel(FuelKind kind, u32 count);
    f32 EnergyMJ() const { return mEnergyMJ; }
    f32 MaxEnergyMJ() const { return mMaxEnergyMJ; }
    f32 EnergyRatio() const { return mMaxEnergyMJ > 0.0f ? mEnergyMJ / mMaxEnergyMJ : 0.0f; }
    f32 PowerDrawMW() const { return mCurrentPowerDrawMW; }
    f32 ChargeRateMW() const { return mCurrentChargeRateMW; }

    // Drone Dispatch
    void DispatchDrone(const Vec3& targetPos, u32 targetId);
    const Array<ConstructionDrone>& Drones() const { return mDrones; }
    Array<ConstructionDrone>& Drones() { return mDrones; }

    // State & Transform
    MechaMode Mode() const { return mMode; }
    Vec3 Position() const { return mPos; }
    void SetPosition(const Vec3& p) { mPos = p; }
    void SetEnergy(f32 mj) { mEnergyMJ = mj > 0.0f ? mj : 0.0f; }
    Vec3 Velocity() const { return mVel; }
    f32 Heading() const { return mHeading; }
    f32 Altitude() const { return mAltitude; }
    f32 Speed() const;
    bool IsLowEnergy() const { return EnergyRatio() < 0.15f; }

    // Visual FX intensities for one-shot takeoff and landing
    f32 ThrusterIntensity() const { return mThrusterBurn; }
    f32 ReentryIntensity() const { return mReentryGlow; }

    // Upgrades
    u32 CoreLevel() const { return mCoreLevel; }
    void UpgradeCore();
    void UpgradeDrones();
    void UpgradeSpeed();

    // Fuel slot info
    const FuelItem& FuelSlot() const { return mFuelSlot; }

private:
    void UpdateEnergyAndCombustion(f32 dt);
    void UpdateDrones(f32 dt);

    Vec3 mPos{0.0f, 502.0f, 0.0f}; // 3D world position on R=500m planet
    Vec3 mVel{0.0f, 0.0f, 0.0f};
    f32 mLat = 0.0f;
    f32 mLon = 0.0f;
    f32 mAltitude = 0.0f; // Above surface in meters
    f32 mHeading = 0.0f; // Angle in radians
    f32 mThrusterBurn = 0.0f; // 0..1
    f32 mReentryGlow = 0.0f;  // 0..1

    MechaMode mMode = MechaMode::GroundWalk;
    bool mWarpActive = false;

    f32 mEnergyMJ = 100.0f;
    f32 mMaxEnergyMJ = 100.0f;
    f32 mCurrentPowerDrawMW = 0.0f;
    f32 mCurrentChargeRateMW = 0.0f;

    FuelItem mFuelSlot;

    // Attributes
    u32 mCoreLevel = 1;
    u32 mMaxDrones = 3;
    f32 mDroneSpeed = 45.0f;
    f32 mWalkSpeed = 24.0f;
    f32 mFlySpeed = 68.0f;
    f32 mSailSpeed = 320.0f;
    f32 mWarpSpeed = 5000.0f;

    Array<ConstructionDrone> mDrones;
};

}
