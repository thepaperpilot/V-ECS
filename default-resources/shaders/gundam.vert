#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in int inMatIndex;

layout(location = 0) out vec3 outDiffuse;
layout(location = 1) out vec3 outPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outCameraPos;

layout(binding = 2) uniform Materials {
	vec4[9] diffuseColors;
} materials;

layout(push_constant) uniform PushConsts {
	mat4 VP;
    mat4 M;
	vec3 cameraPos; // TODO pass to frag shader directly?
} pushConsts;

void main() {
    gl_Position = pushConsts.VP * pushConsts.M * vec4(inPosition, 1.0);
    outDiffuse = vec3(materials.diffuseColors[inMatIndex]);
    vec4 vertPos4 = pushConsts.M * vec4(inPosition, 1.0);
    outPos = vec3(vertPos4) / vertPos4.w;
    outNormal = normalize(mat3(transpose(inverse(pushConsts.M))) * inNormal);
    outCameraPos = pushConsts.cameraPos;
}
