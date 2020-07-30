#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTileSize;

layout(location = 0) out vec2 outFragCoord;
layout(location = 1) out vec4 outTileSize;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outView;
layout(location = 4) out vec3 outLight;
layout(location = 5) out vec3 outAmbient;

layout(push_constant) uniform PushConsts {
	mat4 MVP;
	vec3 cameraPos;
} pushConsts;


layout (binding = 2) uniform UBO {
    vec3 lightPos;
    float ambientColor;
} ubo;

void main() {
    gl_Position = pushConsts.MVP * vec4(inPosition, 1.0);
    outFragCoord = inTexCoord;
    outTileSize = inTileSize;
    outNormal = inNormal;
    outView = pushConsts.cameraPos - inPosition;
    outLight = ubo.lightPos - inPosition;
    outAmbient = vec3(ubo.ambientColor, ubo.ambientColor, ubo.ambientColor);
}
