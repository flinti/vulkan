#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragUv;

layout(push_constant) uniform pushConstants
{
    mat4 mvp;
    vec4 time;
};

void main() {
    gl_Position = mvp * vec4(position, 1.f);
    fragColor = color;
}
