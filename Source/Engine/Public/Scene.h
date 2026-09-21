#pragma once

#include "Container/Array.h"
#include "Container/RefPtr.h"
#include "Core.h"
#include "Mesh.h"
#include "Transform.h"

namespace Aether::Engine {

struct Entity
{
    RefPtr<Mesh> Mesh;
    Transform Transform;
};

class Scene
{
public:
    Entity& AddEntity(RefPtr<Mesh> mesh, const Transform& transform = {})
    {
        mEntities.EmplaceAdd();
        mEntities.Last().Mesh = std::move(mesh);
        mEntities.Last().Transform = transform;
        return mEntities.Last();
    }

    Array<Entity>& Entities() { return mEntities; }
    const Array<Entity>& Entities() const { return mEntities; }

private:
    Array<Entity> mEntities;
};

}
