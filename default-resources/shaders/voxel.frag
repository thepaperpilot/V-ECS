#version 450

layout(location = 0) in vec2 inFragTexCoord;
layout(location = 1) in vec4 inTileSize;
layout(location = 2) in vec3 inPos;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inView;
layout(location = 5) in vec3 inAmbient;
layout(location = 6) in vec3 inLightPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
	// tileSize contains the info about the texture to tile
	// x,y are the UV coordinates of where the texture starts inside the atlas
	// z,w are the width and height of the textures
	// fragTexCoord will be in the range 0 to the number of tiles to repeat the texture for
	// so we modulus it with 1 to find how far we are in the current iteration of the repeating texture,
	// multiply that by the size of the texture, and add the starting UV coordinates
	vec2 texCoord = vec2(inTileSize.x, inTileSize.y) + mod(inFragTexCoord, 1) * vec2(inTileSize.z, inTileSize.w);

    // Calculate some quick hard-coded phong directional lighting
    // hard-coded values:
    vec3 lightDiffuse = vec3(1, 1, 1);
    vec3 lightSpecular = vec3(1, 1, 1);
    float diffuseStrength = 0.4;
    float materialShininess = 32;
    float specularStrength = 2;

    vec3 normal = normalize(inNormal);
    vec3 lightDir = normalize(inLightPos);//inLightPos - inPos;
    //float distance = length(lightDir);
    //lightDir = normalize(lightDir);

	vec3 diffuse = diffuseStrength * max(dot(normal, lightDir), 0) * lightDiffuse;
    
    //vec3 viewDir = normalize(inView - inPos);
    //vec3 halfwayDir = normalize(lightDir + viewDir);
    //vec3 specular = specularStrength * pow(max(dot(normal, halfwayDir), 0), materialShininess) * lightSpecular / distance;

	vec3 lightColor = inAmbient + diffuse;// + specular;

    outColor = vec4(vec3(texture(texSampler, texCoord)) * lightColor, 1);
}
