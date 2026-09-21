#pragma once

//
// Shader source bundles shared by renderer modules. The string contents are
// verbatim WGSL (web) / HLSL (native) sources; see Shaders.cpp.
//

namespace Aether::Engine::Shaders {

struct ShaderSources
{
    const char* Vertex;
    const char* Fragment;
};

const ShaderSources& UnlitColor();
const ShaderSources& UnlitTexture();
const ShaderSources& Starfield();

}
