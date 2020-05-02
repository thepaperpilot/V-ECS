#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 lightColor;
layout(location = 2) in vec4 tileSize;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
	// tileSize contains the info about the texture to tile
	// x,y are the UV coordinates of where the texture starts inside the atlas
	// z,w are the width and height of the textures
	// fragTexCoord will be in the range 0 to the number of tiles to repeat the texture for
	// so we modulus it with 1 to find how far we are in the current iteration of the repeating texture,
	// multiply that by the size of the texture, and add the starting UV coordinates
	vec2 texCoord = vec2(tileSize.x, tileSize.y) + mod(fragTexCoord, 1) * vec2(tileSize.z, tileSize.w);
	// We multiply the sampled texture by the lightColor modifier calculated per-vertex
    outColor = vec4(texture(texSampler, texCoord).rgb * lightColor, 1);
}
