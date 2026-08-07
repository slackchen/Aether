#pragma once

#include "core/platform.h"

namespace aether::engine::shaders {

struct ShaderSources {
    const char* vertex;
    const char* fragment;
};

const ShaderSources& unlit_color();
const ShaderSources& unlit_texture();
const ShaderSources& starfield();

}
