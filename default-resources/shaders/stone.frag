#version 450

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 inPos;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inCameraPos;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    // Calculate some quick hard-coded phong directional lighting
    // hard-coded values:
    vec3 lightPosition = vec3(0, 3, 0);
    vec3 lightAmbient = vec3(.5, .5, .5);
    vec3 lightDiffuse = vec3(.6, .6, .6);
    vec3 lightSpecular = vec3(1, 1, 1);
    float diffuseStrength = 5;
    float materialShininess = 32;
    float specularStrength = 2;

    vec3 normal = normalize(inNormal);
    vec3 lightDir = lightPosition - inPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);

    vec3 diffuse = diffuseStrength * max(dot(normal, lightDir), 0) * lightDiffuse / distance;

    vec3 viewDir = normalize(inCameraPos - inPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 specular = specularStrength * pow(max(dot(normal, halfwayDir), 0), materialShininess) * lightSpecular / distance;

    vec3 lightColor = lightAmbient + diffuse + specular;
    outColor = vec4(vec3(texture(texSampler, UV)) * lightColor, 1);
}
