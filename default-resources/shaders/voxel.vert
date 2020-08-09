#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTileSize;

layout(location = 0) out vec2 outFragCoord;
layout(location = 1) out vec4 outTileSize;
layout(location = 2) out vec3 outPos;
layout(location = 3) out vec3 outNormal;
layout(location = 4) out vec3 outView;
layout(location = 5) out vec3 outAmbient;
layout(location = 6) out vec3 outLightPos;

layout(push_constant) uniform PushConsts {
    mat4 VP;
    mat4 M;
	vec3 cameraPos;
} pushConsts;

layout (binding = 2) uniform UBO {
    vec3 lightPos;
    float ambientColor;
} ubo;

void main() {
    gl_Position = pushConsts.VP * pushConsts.M * vec4(inPosition, 1.0);
    outFragCoord = inTexCoord;
    outTileSize = inTileSize;
    vec4 vertPos4 = pushConsts.M * vec4(inPosition, 1.0);
    outPos = vec3(vertPos4) / vertPos4.w;
    outNormal = normalize(mat3(transpose(inverse(pushConsts.M))) * inNormal);
    outView = pushConsts.cameraPos;
    outAmbient = vec3(ubo.ambientColor, ubo.ambientColor, ubo.ambientColor);
    outLightPos = ubo.lightPos;
}
