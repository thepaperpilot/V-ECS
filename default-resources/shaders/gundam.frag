#version 450

layout(location = 0) in float inMatIndex;

layout(location = 0) out vec4 outColor;

layout(binding = 3) uniform Materials {
	vec4[9] diffuseColors;
} materials;

void main() {
    //outColor = vec4(materials.diffuseColors[int(round(inMatIndex))], 1.0);
    //outColor = vec4(.96, .26, .21, 1.0);
    outColor = vec4(materials.diffuseColors[int(round(inMatIndex))].xyz, 1.0);
}
