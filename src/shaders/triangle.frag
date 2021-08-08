#version 450
#extension GL_ARB_sepatrate_shader_objects : enable

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;

layout(binding = 0, set = 1) uniform UboPerObject
{
    mat4 model;
    float time;
} ubosek;

void main()
{
    outColor = vec4(fragColor.x * ubosek.time/5, fragColor.yz / ubosek.time , 1.0);
}
