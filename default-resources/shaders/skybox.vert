#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 pos;

layout(push_constant) uniform PushConsts {
	mat4 MVP;
} pushConsts;

void main() {
	mat4 MVPWithoutTranslation = pushConsts.MVP;
    MVPWithoutTranslation[3][0] = 0.0f;
    MVPWithoutTranslation[3][1] = 0.0f;
    MVPWithoutTranslation[3][2] = 0.0f;
    gl_Position = MVPWithoutTranslation * vec4(inPosition, 1.0);
    pos = inPosition;
}
