#pragma once

#include "Core.h"
#include "Math/Vec3.h"
#include "Factory.h"
#include "Container/Array.h"

namespace DSP {

using Aether::f32;
using Aether::u32;
using Aether::Math::Vec3;
using Aether::Array;

struct PowerNode {
    u32 BuildingId = 0;
    BuildingKind Kind = BuildingKind::None;
    Vec3 Pos{0.0f, 0.0f, 0.0f};
    f32 SupplyRadius = 12.0f;
    f32 ConnectRadius = 50.0f;
    f32 GenCapacityKW = 0.0f;
    f32 DemandKW = 0.0f;
};

class PowerGrid {
public:
    PowerGrid();

    void Update(f32 dt, const Array<Building>& buildings, const Vec3& mechaPos,
                f32& mechaEnergyMJ, f32 mechaMaxEnergyMJ, const Vec3& sunDir, f32 time);

    f32 TotalGenerationKW() const { return mTotalGenKW; }
    f32 TotalDemandKW() const { return mTotalDemandKW; }
    f32 SatisfactionRatio() const { return mSatisfaction; }
    f32 AccumulatorChargeMJ() const { return mAccumChargeMJ; }
    f32 AccumulatorMaxMJ() const { return mAccumMaxMJ; }
    bool IsMechaCharging() const { return mMechaCharging; }

    const Array<PowerNode>& Nodes() const { return mNodes; }

private:
    Array<PowerNode> mNodes;
    f32 mTotalGenKW = 0.0f;
    f32 mTotalDemandKW = 0.0f;
    f32 mSatisfaction = 1.0f;
    f32 mAccumChargeMJ = 200.0f;
    f32 mAccumMaxMJ = 500.0f;
    bool mMechaCharging = false;
};

}
