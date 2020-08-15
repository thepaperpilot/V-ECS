#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec2 outResolution;
layout(location = 2) out float outTime;
layout(location = 3) out float outAlpha;

layout(push_constant) uniform PushConsts {
    vec2 iResolution;
    float iTime;
    float alpha;
} pushConsts;

void main() {
    outUV = inUV;
    outResolution = pushConsts.iResolution;
    outTime = pushConsts.iTime;
    outAlpha = pushConsts.alpha;
    gl_Position = vec4(inPos, 0, 1);
}
