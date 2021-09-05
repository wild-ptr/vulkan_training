#version 450
#extension GL_ARB_sepatrate_shader_objects : enable

layout (location = 0) in vec3 vNormal;
layout (location = 1) in vec2 texCoords;

layout (location = 0) out vec4 outColor;

layout(binding = 0, set = 0) uniform texture2D textures[4096];
layout(binding = 1, set = 0) uniform sampler samp;

layout(binding = 0, set = 1) uniform UboPerObject
{
    mat4 model;
    float time;
} ubosek;

layout( push_constant ) uniform constants
{
	uint diffuse_idx;
	uint normal_idx;
	uint specular_idx;
} PushConstants;

void main()
{
    outColor = texture(sampler2D(textures[PushConstants.diffuse_idx], samp), texCoords);
    //outColor = vec4(vNormal.xyz, 1.0);
}
