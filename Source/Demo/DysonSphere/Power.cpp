#include "Power.h"
#include "Math/Math.h"

#include <cmath>

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

static f32 Hash1(f32 x)
{
    f32 n = sinf(x * 127.1f) * 43758.5453f;
    return n - floorf(n);
}

PowerGrid::PowerGrid() = default;

void PowerGrid::Update(f32 dt, const Array<Building>& buildings, const Vec3& mechaPos,
                       f32& mechaEnergyMJ, f32 mechaMaxEnergyMJ, const Vec3& sunDir, f32 time)
{
    mNodes.Clear();
    mTotalGenKW = 0.0f;
    mTotalDemandKW = 0.0f;
    mMechaCharging = false;

    // 聚合发电与耗电; 太阳能受昼夜光照角约束, 风电受风速扰动
    for (const Building& b : buildings)
    {
        PowerNode n;
        n.BuildingId = b.Id;
        n.Kind = b.Kind;
        n.Pos = b.WorldPos;

        switch (b.Kind)
        {
            case BuildingKind::WindTurbine:
            {
                f32 gust = 0.55f + 0.45f * Hash1(floorf(time * 0.2f) + (f32)b.Id * 0.37f);
                n.GenCapacityKW = 300.0f * gust;
                n.SupplyRadius = 15.0f;
                n.ConnectRadius = 50.0f;
                mTotalGenKW += n.GenCapacityKW;
                break;
            }
            case BuildingKind::SolarPanel:
            {
                f32 plen = sqrtf(b.WorldPos.x * b.WorldPos.x + b.WorldPos.y * b.WorldPos.y + b.WorldPos.z * b.WorldPos.z);
                f32 ndotl = 0.0f;
                if (plen > 1.0f)
                {
                    ndotl = (b.WorldPos.x / plen) * sunDir.x + (b.WorldPos.y / plen) * sunDir.y + (b.WorldPos.z / plen) * sunDir.z;
                }
                f32 sunFactor = Math::Clamp(ndotl, 0.0f, 1.0f);
                n.GenCapacityKW = 360.0f * sunFactor * sunFactor;
                n.SupplyRadius = 12.0f;
                n.ConnectRadius = 45.0f;
                mTotalGenKW += n.GenCapacityKW;
                break;
            }
            case BuildingKind::ThermalPowerPlant:
                n.GenCapacityKW = 2160.0f;
                n.SupplyRadius = 16.0f;
                n.ConnectRadius = 60.0f;
                mTotalGenKW += n.GenCapacityKW;
                break;
            case BuildingKind::FusionPowerPlant:
                n.GenCapacityKW = 15000.0f;
                n.SupplyRadius = 24.0f;
                n.ConnectRadius = 80.0f;
                mTotalGenKW += n.GenCapacityKW;
                break;
            case BuildingKind::ArtificialStar:
                n.GenCapacityKW = 72000.0f;
                n.SupplyRadius = 35.0f;
                n.ConnectRadius = 100.0f;
                mTotalGenKW += n.GenCapacityKW;
                break;
            case BuildingKind::RayReceiver:
                n.GenCapacityKW = 12500.0f;
                n.SupplyRadius = 20.0f;
                n.ConnectRadius = 70.0f;
                mTotalGenKW += n.GenCapacityKW;
                break;
            case BuildingKind::TeslaTower:
                n.SupplyRadius = 12.0f;
                n.ConnectRadius = 55.0f;
                break;
            case BuildingKind::WirelessPowerTower:
                n.SupplyRadius = 22.0f;
                n.ConnectRadius = 80.0f;
                break;
            default:
            {
                f32 d = BuildingPowerDemandKW(b.Kind);
                n.DemandKW = d;
                mTotalDemandKW += d;
                break;
            }
        }
        mNodes.Add(n);

        // Wireless Mecha Charging check
        if (b.Kind == BuildingKind::WirelessPowerTower || b.Kind == BuildingKind::TeslaTower)
        {
            f32 dx = b.WorldPos.x - mechaPos.x;
            f32 dy = b.WorldPos.y - mechaPos.y;
            f32 dz = b.WorldPos.z - mechaPos.z;
            f32 dist = sqrtf(dx * dx + dy * dy + dz * dz);
            if (dist <= n.SupplyRadius && mechaEnergyMJ < mechaMaxEnergyMJ)
            {
                f32 chargeRateMW = (b.Kind == BuildingKind::WirelessPowerTower) ? 4.8f : 1.6f;
                mechaEnergyMJ = Math::Min(mechaMaxEnergyMJ, mechaEnergyMJ + chargeRateMW * dt);
                mTotalDemandKW += chargeRateMW * 1000.0f;
                mMechaCharging = true;
            }
        }
    }

    // Default emergency planetary baseline power
    if (mTotalGenKW < 600.0f)
    {
        mTotalGenKW += 600.0f;
    }

    // Calculate Satisfaction
    if (mTotalDemandKW <= 0.0f)
    {
        mSatisfaction = 1.0f;
    }
    else
    {
        f32 balance = mTotalGenKW - mTotalDemandKW;
        if (balance >= 0.0f)
        {
            mSatisfaction = 1.0f;
            // Charge accumulator
            mAccumChargeMJ = Math::Min(mAccumMaxMJ, mAccumChargeMJ + (balance * 0.001f) * dt * 0.2f);
        }
        else
        {
            f32 deficit = -balance;
            // Try to discharge accumulator
            if (mAccumChargeMJ > 0.0f)
            {
                f32 discharge = Math::Min(mAccumChargeMJ, (deficit * 0.001f) * dt);
                mAccumChargeMJ -= discharge;
                f32 effGen = mTotalGenKW + (discharge / dt) * 1000.0f;
                mSatisfaction = Math::Clamp(effGen / mTotalDemandKW, 0.2f, 1.0f);
            }
            else
            {
                mSatisfaction = Math::Clamp(mTotalGenKW / mTotalDemandKW, 0.15f, 1.0f);
            }
        }
    }
}

}
