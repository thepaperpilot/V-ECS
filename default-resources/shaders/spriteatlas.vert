#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inTileSize;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outTint;
layout(location = 2) out vec4 outTileSize;

layout(push_constant) uniform PushConsts {
    mat4 M;
    vec4 tint;
} pushConsts;

void main() {
    outUV = inUV;
    outTint = pushConsts.tint;
    outTileSize = inTileSize;
    gl_Position = pushConsts.M * vec4(inPos, 1);
}
