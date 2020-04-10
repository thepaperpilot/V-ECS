#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in int inMatIndex;

layout(location = 0) flat out int outMatIndex;

layout(push_constant) uniform PushConsts {
	mat4 MVP;
} pushConsts;

void main() {
	outMatIndex = inMatIndex;
    gl_Position = pushConsts.MVP * vec4(inPosition, 1.0);
}
