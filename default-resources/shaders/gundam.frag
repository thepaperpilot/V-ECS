#version 450

layout(location = 0) flat in int inMatIndex;

layout(location = 0) out vec4 outColor;

layout(binding = 3) uniform Materials {
	vec4[9] diffuseColors;
} materials;

void main() {
    outColor = vec4(materials.diffuseColors[inMatIndex].xyz, 1.0);
}
