#version 330 core

#include "Include/CommonUBO.incl"

in vec3 position;
in vec2 texCoords;

out VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} vs_out;

void main()
{	
	gl_Position = vec4(position.x, position.y, -1, 1);
	vs_out.texCoords = texCoords;
	// z = -1 plane in view space for full screen quad
	vs_out.positionVS = vec3(position.x / projMat[0][0], position.y / projMat[1][1], -1);
}