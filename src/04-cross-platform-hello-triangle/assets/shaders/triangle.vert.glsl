#version 310 es

layout(binding = 0, std140) uniform UBO
{
    mat4 projectionMatrix;
    mat4 modelMatrix;
    mat4 viewMatrix;
} ubo;

layout(location = 0) out vec3 outColor;
layout(location = 1) in vec3 inColor;
layout(location = 0) in vec3 inPos;

void main()
{
    outColor = inColor;
    gl_Position = ((ubo.projectionMatrix * ubo.viewMatrix) * ubo.modelMatrix) * vec4(inPos, 1.0);
}

