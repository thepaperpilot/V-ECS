#version 450

layout(location = 0) in vec2 UV;
layout(location = 1) in vec4 inTint;
layout(location = 2) in vec4 inTileSize;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
	vec2 texCoord = inTileSize.xy + mod(UV, 1) * inTileSize.zw;

    outColor = texture(texSampler, texCoord) * inTint;
}
