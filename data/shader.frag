#version 450

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 uv;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inColor, 1.0);
}
