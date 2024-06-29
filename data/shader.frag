#version 450

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(set = 0, binding = 0) uniform sampler2D tex0;

layout(location = 0) out vec4 outColor;


void main() {
    outColor = texture(tex0, uv);
}
