#include "Mecha.h"
#include "Math/Math.h"

#include <cmath>

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

Mecha::Mecha()
{
    mFuelSlot.Kind = FuelKind::HydrogenFuelRod;
    mFuelSlot.Count = 40;
    mFuelSlot.EnergyPerItemMJ = 54.0f;
    mFuelSlot.CombustionEfficiency = 1.0f;

    mEnergyMJ = mMaxEnergyMJ;
    mDrones.Resize(mMaxDrones);
    for (ConstructionDrone& d : mDrones)
    {
        d.Pos = mPos;
        d.Speed = mDroneSpeed;
    }
}

f32 Mecha::Speed() const
{
    return sqrtf(mVel.x * mVel.x + mVel.y * mVel.y + mVel.z * mVel.z);
}

void Mecha::ConsumeEnergy(f32 amountMJ)
{
    mEnergyMJ = Math::Max(0.0f, mEnergyMJ - amountMJ);
}

void Mecha::AddFuel(FuelKind kind, u32 count)
{
    if (mFuelSlot.Kind == kind)
    {
        mFuelSlot.Count += count;
    }
    else if (mFuelSlot.Count == 0 || mFuelSlot.Kind == FuelKind::None)
    {
        mFuelSlot.Kind = kind;
        mFuelSlot.Count = count;
        switch (kind)
        {
            case FuelKind::Coal:
                mFuelSlot.EnergyPerItemMJ = 2.7f;
                mFuelSlot.CombustionEfficiency = 0.8f;
                break;
            case FuelKind::HydrogenFuelRod:
                mFuelSlot.EnergyPerItemMJ = 54.0f;
                mFuelSlot.CombustionEfficiency = 1.0f;
                break;
            case FuelKind::DeuteronFuelRod:
                mFuelSlot.EnergyPerItemMJ = 600.0f;
                mFuelSlot.CombustionEfficiency = 1.5f;
                break;
            case FuelKind::AntimatterFuelRod:
                mFuelSlot.EnergyPerItemMJ = 7200.0f;
                mFuelSlot.CombustionEfficiency = 2.0f;
                break;
            default:
                break;
        }
    }
}

void Mecha::ToggleFlight()
{
    if (mMode == MechaMode::GroundWalk && mEnergyMJ > 5.0f)
    {
        mMode = MechaMode::LowFlight;
        mAltitude = 14.0f;
    }
    else if (mMode == MechaMode::LowFlight)
    {
        mMode = MechaMode::GroundWalk;
        mAltitude = 0.0f;
    }
}

void Mecha::ToggleWarp()
{
    if (mMode == MechaMode::SpaceSailing && mEnergyMJ > 40.0f)
    {
        mWarpActive = !mWarpActive;
        mMode = mWarpActive ? MechaMode::Warping : MechaMode::SpaceSailing;
    }
}

void Mecha::Move3D(const Vec2& inputWasd, const Vec3& camRight, f32 altInput, f32 dt, Planet* planet)
{
    if (!planet) return;

    // Normal at current mecha pos
    Vec3 normal = {0.0f, 1.0f, 0.0f};
    f32 plen = sqrtf(mPos.x * mPos.x + mPos.y * mPos.y + mPos.z * mPos.z);
    if (plen > 1e-4f)
    {
        normal = {mPos.x / plen, mPos.y / plen, mPos.z / plen};
    }

    // 切向右向量: 相机右向量投影到当前点切平面。
    // 相机 right = view_fwd × up, 天然与 up 正交且始终指向屏幕右侧。
    f32 rDot = camRight.x * normal.x + camRight.y * normal.y + camRight.z * normal.z;
    Vec3 rightTangent = {camRight.x - rDot * normal.x, camRight.y - rDot * normal.y, camRight.z - rDot * normal.z};
    f32 rtLen = sqrtf(rightTangent.x * rightTangent.x + rightTangent.y * rightTangent.y + rightTangent.z * rightTangent.z);
    if (rtLen > 1e-4f) { rightTangent.x /= rtLen; rightTangent.y /= rtLen; rightTangent.z /= rtLen; }
    else rightTangent = {1.0f, 0.0f, 0.0f};

    // 切向前向量 = normal × rightTangent。
    // 恒等于相机前向的切向投影 (n × (f×n) = f_tangent), 任意半球方向一致, 无需翻转判断。
    Vec3 fwdTangent = {
        normal.y * rightTangent.z - normal.z * rightTangent.y,
        normal.z * rightTangent.x - normal.x * rightTangent.z,
        normal.x * rightTangent.y - normal.y * rightTangent.x
    };

    // Combined desired 3D velocity
    Vec3 desiredV = {
        inputWasd.y * fwdTangent.x + inputWasd.x * rightTangent.x,
        inputWasd.y * fwdTangent.y + inputWasd.x * rightTangent.y,
        inputWasd.y * fwdTangent.z + inputWasd.x * rightTangent.z
    };
    f32 vLen = sqrtf(desiredV.x * desiredV.x + desiredV.y * desiredV.y + desiredV.z * desiredV.z);

    f32 moveSpeed = (mMode == MechaMode::GroundWalk) ? mWalkSpeed :
                    (mMode == MechaMode::LowFlight) ? mFlySpeed : mSailSpeed;
    if (IsLowEnergy()) moveSpeed *= 0.4f;

    if (vLen > 0.01f)
    {
        desiredV.x = (desiredV.x / vLen) * moveSpeed;
        desiredV.y = (desiredV.y / vLen) * moveSpeed;
        desiredV.z = (desiredV.z / vLen) * moveSpeed;

        // Update heading on screen (0 = forward, PI/2 = right, PI = back, -PI/2 = left)
        mHeading = atan2f(inputWasd.x, inputWasd.y);

        // Power draw & consumption
        f32 pwr = (mMode == MechaMode::GroundWalk) ? 0.8f : (mMode == MechaMode::LowFlight ? 3.5f : 8.0f);
        mCurrentPowerDrawMW += pwr;
        ConsumeEnergy(pwr * dt);
    }

    // Smooth altitude update (One-shot takeoff & landing)
    if (altInput > 0.0f)
    {
        mThrusterBurn = Math::Min(1.0f, mThrusterBurn + dt * 4.0f);
        f32 climbSpeed = (mMode == MechaMode::GroundWalk) ? 14.0f : (mAltitude < 80.0f ? 28.0f : 80.0f);
        mAltitude += climbSpeed * dt;
        if (mAltitude > 2.0f && mMode == MechaMode::GroundWalk)
        {
            mMode = MechaMode::LowFlight;
        }
        if (mAltitude > 120.0f && mMode == MechaMode::LowFlight)
        {
            mMode = MechaMode::SpaceSailing;
        }
        ConsumeEnergy(4.0f * dt);
    }
    else if (altInput < 0.0f)
    {
        f32 sinkSpeed = (mAltitude > 80.0f) ? 60.0f : 24.0f;
        mAltitude -= sinkSpeed * dt;
        if (mAltitude <= 0.0f)
        {
            mAltitude = 0.0f;
            mMode = MechaMode::GroundWalk;
        }
        else if (mAltitude <= 100.0f && mMode == MechaMode::SpaceSailing)
        {
            mMode = MechaMode::LowFlight;
        }
        mThrusterBurn = Math::Max(0.0f, mThrusterBurn - dt * 3.0f);
    }
    else
    {
        mThrusterBurn = Math::Max(0.0f, mThrusterBurn - dt * 2.0f);
    }

    // Re-entry plasma heating effect when sinking fast into dense atmosphere
    if (altInput < 0.0f && mAltitude > 15.0f && mAltitude < 120.0f)
    {
        mReentryGlow = Math::Min(1.0f, mReentryGlow + dt * 3.0f);
    }
    else
    {
        mReentryGlow = Math::Max(0.0f, mReentryGlow - dt * 2.0f);
    }

    // Integrate 3D position
    Vec3 newPos = {
        mPos.x + desiredV.x * dt,
        mPos.y + desiredV.y * dt,
        mPos.z + desiredV.z * dt
    };
    f32 nCalc = sqrtf(newPos.x * newPos.x + newPos.y * newPos.y + newPos.z * newPos.z);
    if (nCalc > 1e-4f)
    {
        Vec3 u = {newPos.x / nCalc, newPos.y / nCalc, newPos.z / nCalc};
        f32 hTerrain = planet->GetTerrainHeight3D(u);
        f32 totalR = planet->Radius() + hTerrain + mAltitude;
        mPos = {u.x * totalR, u.y * totalR, u.z * totalR};
    }
}

void Mecha::MoveGround(const Vec2& dir, f32 dt, Planet* planet)
{
    Vec3 defaultRight = {1.0f, 0.0f, 0.0f};
    Move3D(dir, defaultRight, 0.0f, dt, planet);
}

void Mecha::MoveFlight(const Vec2& dir, f32 altInput, f32 dt, Planet* planet)
{
    Vec3 defaultRight = {1.0f, 0.0f, 0.0f};
    Move3D(dir, defaultRight, altInput, dt, planet);
}

void Mecha::MoveSpace(const Vec3& thrustDir, f32 dt)
{
    (void)thrustDir;
    (void)dt;
}

void Mecha::UpgradeCore()
{
    mCoreLevel++;
    mMaxEnergyMJ += 50.0f;
    mEnergyMJ = mMaxEnergyMJ;
}

void Mecha::UpgradeDrones()
{
    mMaxDrones += 2;
    mDroneSpeed += 10.0f;
    mDrones.Resize(mMaxDrones);
    for (ConstructionDrone& d : mDrones)
    {
        d.Pos = mPos;
        d.Speed = mDroneSpeed;
    }
}

void Mecha::UpgradeSpeed()
{
    mWalkSpeed += 4.0f;
    mFlySpeed += 8.0f;
    mSailSpeed += 40.0f;
}

void Mecha::DispatchDrone(const Vec3& targetPos, u32 targetId)
{
    for (ConstructionDrone& d : mDrones)
    {
        if (!d.Active)
        {
            d.Active = true;
            d.Returning = false;
            d.Pos = mPos;
            d.TargetPos = targetPos;
            d.TargetId = targetId;
            d.BeamTimer = 0.8f;
            break;
        }
    }
}

void Mecha::Update(f32 dt, Planet* currentPlanet, bool spaceMode)
{
    (void)spaceMode;
    UpdateEnergyAndCombustion(dt);
    UpdateDrones(dt);

    if (currentPlanet && mMode == MechaMode::GroundWalk)
    {
        f32 plen = sqrtf(mPos.x * mPos.x + mPos.y * mPos.y + mPos.z * mPos.z);
        if (plen > 1e-4f)
        {
            Vec3 u = {mPos.x / plen, mPos.y / plen, mPos.z / plen};
            f32 hTerrain = currentPlanet->GetTerrainHeight3D(u);
            f32 targetR = currentPlanet->Radius() + hTerrain + mAltitude;
            mPos = {u.x * targetR, u.y * targetR, u.z * targetR};
        }
    }
}

void Mecha::UpdateEnergyAndCombustion(f32 dt)
{
    mCurrentChargeRateMW = 0.0f;

    // Fuel combustion in chamber
    if (mEnergyMJ < mMaxEnergyMJ && mFuelSlot.Count > 0 && mFuelSlot.Kind != FuelKind::None)
    {
        f32 burnRateMJPerSec = 8.0f * mFuelSlot.CombustionEfficiency;
        f32 energyGained = burnRateMJPerSec * dt;
        mEnergyMJ = Math::Min(mMaxEnergyMJ, mEnergyMJ + energyGained);
        mCurrentChargeRateMW = burnRateMJPerSec;

        if (mEnergyMJ >= mMaxEnergyMJ)
        {
            // Reached full charge
        }
    }

    mCurrentPowerDrawMW = Math::Max(0.2f, mCurrentPowerDrawMW * (1.0f - dt * 3.0f));
}

void Mecha::UpdateDrones(f32 dt)
{
    for (ConstructionDrone& d : mDrones)
    {
        if (!d.Active) continue;

        if (!d.Returning)
        {
            // Fly towards build target
            Vec3 delta = {d.TargetPos.x - d.Pos.x, d.TargetPos.y - d.Pos.y, d.TargetPos.z - d.Pos.z};
            f32 dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
            if (dist > 2.0f)
            {
                d.Pos.x += (delta.x / dist) * d.Speed * dt;
                d.Pos.y += (delta.y / dist) * d.Speed * dt;
                d.Pos.z += (delta.z / dist) * d.Speed * dt;
            }
            else
            {
                d.BeamTimer -= dt;
                if (d.BeamTimer <= 0.0f)
                {
                    d.Returning = true;
                }
            }
        }
        else
        {
            // Fly back to mecha
            Vec3 delta = {mPos.x - d.Pos.x, mPos.y - d.Pos.y, mPos.z - d.Pos.z};
            f32 dist = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
            if (dist > 2.0f)
            {
                d.Pos.x += (delta.x / dist) * d.Speed * dt;
                d.Pos.y += (delta.y / dist) * d.Speed * dt;
                d.Pos.z += (delta.z / dist) * d.Speed * dt;
            }
            else
            {
                d.Active = false;
                d.Returning = false;
            }
        }
    }
}

}
