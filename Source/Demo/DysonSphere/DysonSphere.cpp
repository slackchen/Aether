#include "DysonSphere.h"
#include "Random.h"
#include "Math/Math.h"

#include <cmath>

namespace DSP {

using namespace Aether;
using namespace Aether::Math;

DysonSphereManager::DysonSphereManager()
{
    InitGeodesicShell();

    // Spawn starting swarm
    SpawnSails(300);
}

void DysonSphereManager::InitGeodesicShell()
{
    mNodes.Clear();
    mStruts.Clear();
    mPanels.Clear();

    // Golden ratio for icosahedron vertices
    f32 phi = (1.0f + sqrtf(5.0f)) * 0.5f;
    f32 invLen = 1.0f / sqrtf(1.0f + phi * phi);
    f32 a = 1.0f * invLen * mSphereRadius;
    f32 b = phi * invLen * mSphereRadius;

    Vec3 rawVerts[12] = {
        {-a,  b, 0.0f}, { a,  b, 0.0f}, {-a, -b, 0.0f}, { a, -b, 0.0f},
        {0.0f, -a,  b}, {0.0f,  a,  b}, {0.0f, -a, -b}, {0.0f,  a, -b},
        { b, 0.0f, -a}, { b, 0.0f,  a}, {-b, 0.0f, -a}, {-b, 0.0f,  a},
    };

    for (u32 i = 0; i < 12; i++)
    {
        DysonNode n;
        n.Id = i;
        n.Pos = rawVerts[i];
        n.RocketsInvested = (i < 3) ? 30 : 0; // First 3 nodes start completed
        n.Completed = (i < 3);
        mNodes.Add(n);
    }

    const u32 RAW_FACES[20][3] = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    for (u32 i = 0; i < 20; i++)
    {
        DysonShellPanel p;
        p.NodeIndices[0] = RAW_FACES[i][0];
        p.NodeIndices[1] = RAW_FACES[i][1];
        p.NodeIndices[2] = RAW_FACES[i][2];
        p.SailsAbsorbed = (i == 0) ? 120 : 0;
        p.Completed = (i == 0);
        p.FillRatio = p.Completed ? 1.0f : 0.0f;
        mPanels.Add(p);

        // Add 3 struts per face if not duplicate
        auto addStrut = [this](u32 na, u32 nb) {
            for (const DysonStrut& s : mStruts)
            {
                if ((s.NodeA == na && s.NodeB == nb) || (s.NodeA == nb && s.NodeB == na)) return;
            }
            DysonStrut s;
            s.NodeA = na;
            s.NodeB = nb;
            s.Completed = (na < 3 && nb < 3);
            s.Progress = s.Completed ? 1.0f : 0.0f;
            mStruts.Add(s);
        };
        addStrut(RAW_FACES[i][0], RAW_FACES[i][1]);
        addStrut(RAW_FACES[i][1], RAW_FACES[i][2]);
        addStrut(RAW_FACES[i][2], RAW_FACES[i][0]);
    }
}

void DysonSphereManager::SpawnSails(u32 count)
{
    Random rng(1337);

    for (u32 i = 0; i < count; i++)
    {
        SolarSailParticle s;
        s.OrbitRadius = rng.Range(mSphereRadius * 0.8f, mSphereRadius * 1.3f);
        s.Angle = rng.Range(0.0f, Math::TWO_PI);
        s.Inclination = rng.Range(-0.45f, 0.45f);
        s.Life = 1200.0f;
        s.MaxLife = 1200.0f;
        s.Absorbed = false;
        mSails.Add(s);
    }
}

void DysonSphereManager::InvestRockets(u32 count)
{
    u32 rem = count;
    for (DysonNode& n : mNodes)
    {
        if (!n.Completed)
        {
            u32 needed = n.RocketsRequired - n.RocketsInvested;
            u32 add = Math::Min(rem, needed);
            n.RocketsInvested += add;
            rem -= add;
            if (n.RocketsInvested >= n.RocketsRequired)
            {
                n.Completed = true;
            }
            if (rem == 0) break;
        }
    }
}

u32 DysonSphereManager::CompletedNodes() const
{
    u32 count = 0;
    for (const DysonNode& n : mNodes)
    {
        if (n.Completed) count++;
    }
    return count;
}

void DysonSphereManager::Update(f32 dt, u32 newLaunchedSails, u32 newLaunchedRockets)
{
    if (newLaunchedSails > 0)
    {
        SpawnSails(newLaunchedSails);
    }
    if (newLaunchedRockets > 0)
    {
        InvestRockets(newLaunchedRockets);
    }

    // Update solar sails
    mSwarmGenGW = 0.0f;
    f32 orbitAngularSpeed = 0.08f;

    for (u32 i = 0; i < mSails.Count();)
    {
        SolarSailParticle& sail = mSails[i];
        sail.Angle += orbitAngularSpeed * dt;
        if (sail.Angle > Math::TWO_PI) sail.Angle -= Math::TWO_PI;

        f32 cosA = cosf(sail.Angle);
        f32 sinA = sinf(sail.Angle);
        sail.Pos = {
            sail.OrbitRadius * cosA,
            sail.OrbitRadius * sinA * sinf(sail.Inclination),
            sail.OrbitRadius * sinA * cosf(sail.Inclination)
        };

        sail.Life -= dt;

        // Try absorption into incomplete shell panels
        if (!sail.Absorbed)
        {
            for (DysonShellPanel& p : mPanels)
            {
                if (!p.Completed && p.SailsAbsorbed < p.SailsRequired)
                {
                    const DysonNode& na = mNodes[p.NodeIndices[0]];
                    const DysonNode& nb = mNodes[p.NodeIndices[1]];
                    const DysonNode& nc = mNodes[p.NodeIndices[2]];
                    if (na.Completed && nb.Completed && nc.Completed)
                    {
                        p.SailsAbsorbed++;
                        p.FillRatio = (f32)p.SailsAbsorbed / (f32)p.SailsRequired;
                        if (p.SailsAbsorbed >= p.SailsRequired)
                        {
                            p.Completed = true;
                        }
                        sail.Absorbed = true;
                        break;
                    }
                }
            }
        }

        if (sail.Life <= 0.0f || sail.Absorbed)
        {
            mSails.RemoveAt(i);
        }
        else
        {
            mSwarmGenGW += 0.000036f; // 36 kW per solar sail
            ++i;
        }
    }

    // Update shell nodes and panels power
    mShellGenGW = 0.0f;
    for (const DysonNode& n : mNodes)
    {
        if (n.Completed)
        {
            mShellGenGW += n.EnergyOutputMW * 0.001f; // 96 MW = 0.096 GW
        }
    }
    for (const DysonShellPanel& p : mPanels)
    {
        if (p.Completed)
        {
            mShellGenGW += 0.120f; // 120 MW per complete panel
        }
        else if (p.FillRatio > 0.0f)
        {
            mShellGenGW += 0.120f * p.FillRatio;
        }
    }
}

}
