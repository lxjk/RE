#version 330 core

in VS_OUT
{
	vec3 texCoords;
} fs_in;

out vec4 color;

uniform samplerCube cubeMap;

void main() 
{
	color = texture(cubeMap, fs_in.texCoords);
	//float shadowZ = texture(cubeMap, fs_in.texCoords).r;
	//float shadowViewZ = -0.101010107 / (-shadowZ + 1.01010108);
	//color = vec4(-shadowViewZ / 10);
}