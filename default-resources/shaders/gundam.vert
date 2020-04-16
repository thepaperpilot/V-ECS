#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in int inMatIndex;

layout(location = 0) out vec3 outColor;

layout(binding = 2) uniform Materials {
	vec4[9] diffuseColors;
} materials;

layout(push_constant) uniform PushConsts {
	mat4 MVP;
	vec3 cameraPos;
} pushConsts;

void main() {
    gl_Position = pushConsts.MVP * vec4(inPosition, 1.0);

    // Calculate some quick hard-coded phong directional lighting
    // hard-coded values:
    vec3 lightDirection = vec3(-1, -1, -1);
    vec3 lightAmbient = vec3(.5, .5, .5);
    vec3 lightDiffuse = vec3(.6, .6, .6);
    vec3 lightSpecular = vec3(0, 0, 0);
    float materialShininess = 32;

    vec3 normal = normalize(inNormal);
    vec3 viewDir = normalize(pushConsts.cameraPos - inPosition);

    vec3 lightDir = normalize(-lightDirection);
    float diffuse = max(dot(normal, lightDir), 0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specular = pow(max(dot(halfwayDir, normal), 0), materialShininess);

    vec3 lightColor = lightAmbient + lightDiffuse * diffuse + lightSpecular * specular;
    outColor = materials.diffuseColors[inMatIndex].rgb * (lightColor + .1);
}
