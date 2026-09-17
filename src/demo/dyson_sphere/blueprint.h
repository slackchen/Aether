#pragma once

#include "core/platform.h"
#include "demo/dyson_sphere/factory.h"
#include "demo/dyson_sphere/mecha.h"
#include <vector>
#include <string>

namespace dsp {

using namespace aether;

struct BlueprintItem {
    BuildingKind kind = BuildingKind::None;
    i32 dx = 0;   // 瓦片 u 偏移
    i32 dy = 0;   // 瓦片 v 偏移
    i32 dir = 0;  // 朝向 0=+u 1=-u 2=+v 3=-v
    ItemKind recipe = ItemKind::None;
};

struct Blueprint {
    std::string name;
    std::string desc;
    std::vector<BlueprintItem> items;
};

class BlueprintManager {
public:
    BlueprintManager();

    const std::vector<Blueprint>& presets() const { return presets_; }

    // 以 origin 为锚点铺贴; 只放置合法空闲瓦片, 返回成功数量
    u32 paste_blueprint(const Blueprint& bp, u32 origin_key, Planet* planet,
                        FactorySystem& factory, Mecha& mecha);

private:
    void init_default_presets();
    std::vector<Blueprint> presets_;
};

} // namespace dsp
