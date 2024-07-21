#version 450

layout(set = 0, binding = 0) uniform Ubo {
    mat4 vp;
    vec3 viewPos;
    vec4 time;
    vec3 lightPos;
    vec3 lightColor;
};

layout(set = 0, binding = 1) uniform sampler2D tex0;

layout(location = 0) in vec3 positionWorld;
layout(location = 1) in vec3 normalWorld;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec4 outColor;

vec3 phong(vec3 color, vec4 ambDiffSpecShiny)
{
    vec3 normal = normalize(normalWorld);
    vec3 lightDirection = normalize(lightPos - positionWorld);
    vec3 reflection = normalize(reflect(-lightDirection, normal));
    vec3 viewDir = normalize(viewPos - positionWorld);

    float diffuse = max(dot(lightDirection, normal) * ambDiffSpecShiny.y, 0.0);
    float specular = max(pow(max(dot(reflection, viewDir), 0.0), ambDiffSpecShiny.w) * ambDiffSpecShiny.z, 0.0);
    return clamp(ambDiffSpecShiny.x + abs(diffuse) + specular, 0.0, 1.0) * color;
}

void main()
{
    vec4 texColor = texture(tex0, uv);
    texColor = vec4(phong(texColor.xyz, vec4(0.05, 0.3, 0.8, 30.0)), texColor.a);
    outColor = texColor;
}
