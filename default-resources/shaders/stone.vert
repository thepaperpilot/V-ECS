#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outCameraPos;

layout(push_constant) uniform PushConsts {
    mat4 M;
} pushConsts;

layout (binding = 2) uniform UBO {
    mat4 VP;
	vec3 cameraPos;
} ubo;

void main() {
    gl_Position = ubo.VP * pushConsts.M * vec4(inPosition, 1.0);
    outUV = inUV;
    vec4 vertPos4 = pushConsts.M * vec4(inPosition, 1.0);
    outPos = vec3(vertPos4) / vertPos4.w;
    outNormal = normalize(mat3(transpose(inverse(pushConsts.M))) * inNormal);
    outCameraPos = ubo.cameraPos;
}
