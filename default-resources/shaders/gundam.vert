#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inMatIndex;

layout(location = 0) out float outMatIndex;

layout(push_constant) uniform PushConsts {
	mat4 MVP;
} pushConsts;

void main() {
	outMatIndex = inMatIndex;
    gl_Position = pushConsts.MVP * vec4(inPosition, 1.0);
}
