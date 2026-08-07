#include "engine/shaders.h"

namespace aether::engine::shaders {

namespace {

#ifdef __EMSCRIPTEN__

const char* kUnlitColorVS = R"(
struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

struct Uniforms {
    mvp: mat4x4f,
};

@group(0) @binding(0)
var<uniform> u_globals : Uniforms;

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = u_globals.mvp * vec4f(input.position, 1.0);
    output.color = input.color;
    return output;
}
)";

const char* kUnlitColorFS = R"(
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4f {
    return vec4f(input.color, 1.0);
}
)";

const char* kUnlitTextureVS = R"(
struct VertexInput {
    @location(0) position: vec2f,
    @location(1) uv: vec2f,
    @location(2) color: vec4f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
};

struct Uniforms {
    vp: mat4x4f,
};

@group(0) @binding(0)
var<uniform> u_globals : Uniforms;

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = u_globals.vp * vec4f(input.position, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
)";

const char* kUnlitTextureFS = R"(
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) color: vec4f,
};

@group(0) @binding(1)
var u_tex: texture_2d<f32>;
@group(0) @binding(2)
var u_sampler: sampler;

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4f {
    let texel = textureSample(u_tex, u_sampler, input.uv);
    return vec4f(texel.rgb * input.color.rgb, texel.a * input.color.a);
}
)";

const char* kStarfieldVS = R"(
struct VertexInput {
    @location(0) position: vec2f,
};

@vertex
fn vs_main(input: VertexInput) -> @builtin(position) vec4f {
    return vec4f(input.position, 0.0, 1.0);
}
)";

const char* kStarfieldFS = R"(
struct Params {
    time: f32,
    speed: f32,
    resolution: vec2f,
};

@group(0) @binding(0)
var<uniform> u_params : Params;

fn hash2(p: vec2f) -> vec2f {
    let q = vec2f(dot(p, vec2f(127.1, 311.7)), dot(p, vec2f(269.5, 183.3)));
    var r = fract(sin(q) * 43758.5453);
    return r;
}

fn hash1(p: vec2f) -> f32 {
    return fract(sin(dot(p, vec2f(127.1, 311.7))) * 43758.5453);
}

@fragment
fn fs_main(@builtin(position) frag: vec4f) -> @location(0) vec4f {
    let uv = frag.xy / u_params.resolution;
    let aspect = u_params.resolution.x / u_params.resolution.y;
    let t = u_params.time * u_params.speed;

    var col = vec3f(0.006, 0.008, 0.022);

    let n1 = sin(uv.x * 6.28318 + t * 0.4);
    let n2 = cos(uv.y * 4.0 * aspect - t * 0.3);
    col += vec3f(0.012, 0.008, 0.035) * (0.5 + 0.5 * n1);
    col += vec3f(0.02, 0.006, 0.015) * (0.5 + 0.5 * n2);

    let layer_colors = array<vec3f, 3>(
        vec3f(0.45, 0.55, 1.0),
        vec3f(0.75, 0.85, 1.0),
        vec3f(1.0, 0.95, 0.8)
    );

    for (var i = 0; i < 3; i++) {
        let f = f32(i);
        let grid = 48.0 + f * 46.0;
        let coord = vec2f(uv.x * grid * aspect, uv.y * grid + t * (2.0 + f * 2.4));
        let cell = floor(coord);
        let r = hash2(cell);
        let pos = fract(coord);
        let d = distance(pos, r);
        let b = max(0.0, 1.0 - d * 18.0);
        let tw = 0.6 + 0.4 * sin(t * (3.0 + r.x * 6.0) + cell.x * 0.7);
        col += layer_colors[i] * b * b * tw * (0.05 + f * 0.035);
    }

    let m = hash1(floor(vec2f(uv.x * aspect * 150.0, uv.y * 150.0)));
    if (m > 0.9975) {
        let fall_y = fract(uv.y * 150.0 - t * 2.5);
        let streak = smoothstep(0.0, 0.05, fall_y) * smoothstep(1.0, 0.95, fall_y);
        col += vec3f(0.7, 0.8, 1.0) * streak * (0.5 + 0.5 * m) * 0.15;
    }

    return vec4f(col, 1.0);
}
)";

#else

const char* kUnlitColorVS = R"(
cbuffer Globals : register(b0) {
    float4x4 u_mvp;
};

struct VSIn {
    float3 position : ATTRIB0;
    float3 color : ATTRIB1;
};

struct VSOut {
    float4 position : SV_Position;
    float3 color : COLOR;
};

VSOut vs_main(VSIn input) {
    VSOut output;
    output.position = mul(u_mvp, float4(input.position, 1.0));
    output.color = input.color;
    return output;
}
)";

const char* kUnlitColorFS = R"(
struct VSOut {
    float4 position : SV_Position;
    float3 color : COLOR;
};

float4 fs_main(VSOut input) : SV_Target {
    return float4(input.color, 1.0);
}
)";

const char* kUnlitTextureVS = R"(
cbuffer Globals : register(b0) {
    float4x4 u_vp;
};

struct VSIn {
    float2 position : ATTRIB0;
    float2 uv : ATTRIB1;
    float4 color : ATTRIB2;
};

struct VSOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

VSOut vs_main(VSIn input) {
    VSOut output;
    output.position = mul(u_vp, float4(input.position, 0.0, 1.0));
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
)";

const char* kUnlitTextureFS = R"(
Texture2D u_tex : register(t1);
SamplerState u_sampler : register(s2);

struct VSOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 fs_main(VSOut input) : SV_Target {
    float4 texel = u_tex.Sample(u_sampler, input.uv);
    return float4(texel.rgb * input.color.rgb, texel.a * input.color.a);
}
)";

const char* kStarfieldVS = R"(
struct VSIn {
    float2 position : ATTRIB0;
};

float4 vs_main(VSIn input) : SV_Position {
    return float4(input.position, 0.0, 1.0);
}
)";

const char* kStarfieldFS = R"(
cbuffer Params : register(b0) {
    float u_time;
    float u_speed;
    float2 u_resolution;
};

float2 hash2(float2 p) {
    float2 q = float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)));
    return frac(sin(q) * 43758.5453);
}

float hash1(float2 p) {
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

float4 fs_main(float4 frag : SV_Position) : SV_Target {
    float2 uv = frag.xy / u_resolution;
    float aspect = u_resolution.x / u_resolution.y;
    float t = u_time * u_speed;

    float3 col = float3(0.006, 0.008, 0.022);

    float n1 = sin(uv.x * 6.28318 + t * 0.4);
    float n2 = cos(uv.y * 4.0 * aspect - t * 0.3);
    col += float3(0.012, 0.008, 0.035) * (0.5 + 0.5 * n1);
    col += float3(0.02, 0.006, 0.015) * (0.5 + 0.5 * n2);

    float3 layer_colors[3];
    layer_colors[0] = float3(0.45, 0.55, 1.0);
    layer_colors[1] = float3(0.75, 0.85, 1.0);
    layer_colors[2] = float3(1.0, 0.95, 0.8);

    for (int i = 0; i < 3; i++) {
        float f = (float)i;
        float grid = 48.0 + f * 46.0;
        float2 coord = float2(uv.x * grid * aspect, uv.y * grid + t * (2.0 + f * 2.4));
        float2 cell = floor(coord);
        float2 r = hash2(cell);
        float2 pos = frac(coord);
        float d = distance(pos, r);
        float b = max(0.0, 1.0 - d * 18.0);
        float tw = 0.6 + 0.4 * sin(t * (3.0 + r.x * 6.0) + cell.x * 0.7);
        col += layer_colors[i] * b * b * tw * (0.05 + f * 0.035);
    }

    float m = hash1(floor(float2(uv.x * aspect * 150.0, uv.y * 150.0)));
    if (m > 0.9975) {
        float fall_y = frac(uv.y * 150.0 - t * 2.5);
        float streak = smoothstep(0.0, 0.05, fall_y) * smoothstep(1.0, 0.95, fall_y);
        col += float3(0.7, 0.8, 1.0) * streak * (0.5 + 0.5 * m) * 0.15;
    }

    return float4(col, 1.0);
}
)";

#endif

}

const ShaderSources& unlit_color() {
    static const ShaderSources sources = {kUnlitColorVS, kUnlitColorFS};
    return sources;
}

const ShaderSources& unlit_texture() {
    static const ShaderSources sources = {kUnlitTextureVS, kUnlitTextureFS};
    return sources;
}

const ShaderSources& starfield() {
    static const ShaderSources sources = {kStarfieldVS, kStarfieldFS};
    return sources;
}

}
