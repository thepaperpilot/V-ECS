#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(push_constant) uniform PushConsts {
	mat4 model;
    mat4 view;
    mat4 projection;
} pushConsts;

void main() {
    gl_Position = pushConsts.projection * pushConsts.view * pushConsts.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
