#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 pos;

const vec4 skytop = vec4(0.0f, 0.0f, 1.0f, 1.0f);
const vec4 skyhorizon = vec4(0.3294f, 0.92157f, 1.0f, 1.0f);

void main() {
    vec3 pointOnSphere = normalize(pos);
    outColor = mix(skyhorizon, skytop, pointOnSphere.y);
}
