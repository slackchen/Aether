#pragma once

#include "Core.h"
#include "Factory.h"
#include "Mecha.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace DSP {

using Aether::i32;
using Aether::u32;
using Aether::Array;
using Aether::String;

struct BlueprintItem {
    BuildingKind Kind = BuildingKind::None;
    i32 Dx = 0;   // 瓦片 u 偏移
    i32 Dy = 0;   // 瓦片 v 偏移
    i32 Dir = 0;  // 朝向 0=+u 1=-u 2=+v 3=-v
    ItemKind Recipe = ItemKind::None;
};

struct Blueprint {
    String Name;
    String Desc;
    Array<BlueprintItem> Items;
};

class BlueprintManager {
public:
    BlueprintManager();

    const Array<Blueprint>& Presets() const { return mPresets; }

    // 以 origin 为锚点铺贴; 只放置合法空闲瓦片, 返回成功数量
    u32 PasteBlueprint(const Blueprint& bp, u32 originKey, Planet* planet,
                       FactorySystem& factory, Mecha& mecha);

private:
    void InitDefaultPresets();
    Array<Blueprint> mPresets;
};

} // namespace DSP
