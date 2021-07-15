#version 450
#extension GL_ARB_sepatrate_shader_objects : enable

layout (location = 0) in vec3 fragColor;
layout (location = 0) out vec4 outColor;
layout(location = 1) in float time;

void main()
{
    outColor = vec4(fragColor.x * time/5, fragColor.yz / time , 1.0);
}
