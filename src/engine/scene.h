#pragma once

#include "core/platform.h"
#include "engine/mesh.h"
#include "engine/transform.h"
#include <memory>
#include <vector>

namespace aether::engine {

struct Entity {
    std::shared_ptr<Mesh> mesh;
    Transform transform;
};

class Scene {
public:
    Entity& add_entity(std::shared_ptr<Mesh> mesh, const Transform& transform = {}) {
        entities_.push_back(Entity{std::move(mesh), transform});
        return entities_.back();
    }

    std::vector<Entity>& entities() { return entities_; }
    const std::vector<Entity>& entities() const { return entities_; }

private:
    std::vector<Entity> entities_;
};

}
