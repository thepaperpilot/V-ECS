#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConsts {
	mat4 MVP;
} pushConsts;

void main() {
    gl_Position = pushConsts.MVP * vec4(inPosition, 1.0);
}
