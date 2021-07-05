#version 450
#extension GL_ARB_sepatrate_shader_objects : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangents;
layout (location = 3) in vec2 vTexCoords;

layout (location = 0) out vec3 fragColor;

// test one
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;


void main()
{
    gl_Position = vec4(vPosition, 1.0);
    fragColor = vNormal;
}
