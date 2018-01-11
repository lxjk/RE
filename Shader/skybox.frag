#version 430 core

#include "Include/CommonUBO.incl"

in VS_OUT
{
	vec3 texCoords;
} fs_in;

out vec4 color;

uniform samplerCube cubeMap;

void main() 
{
	color = texture(cubeMap, fs_in.texCoords);
	//float z = texture(cubeMap, fs_in.texCoords).r * 2 - 1;
	//float linearDepth = (2 * resolution.z * resolution.w) / (resolution.w + resolution.z - z * (resolution.w - resolution.z));
	//linearDepth = linearDepth / (resolution.w - resolution.z);
	//color = vec4(vec3(linearDepth), 1.0);
}