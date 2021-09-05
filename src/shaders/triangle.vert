#version 450
#extension GL_ARB_sepatrate_shader_objects : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vTangents;
layout (location = 3) in vec2 vTexCoords;

// test one
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

layout (location = 0) out vec3 normal;
layout (location = 1) out vec2 texCoords;

void main()
{
    gl_Position =  ubosek.model * vec4(vPosition, 1.0);
    texCoords = vTexCoords;
    normal = vNormal;
}
