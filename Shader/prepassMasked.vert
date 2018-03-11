#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/CommonVertexInput.incl"

out VS_OUT
{
	vec2 texCoords;
} vs_out;

uniform mat4 modelMat;

void main()
{	
	gl_Position = viewProjMat * modelMat * vec4(position, 1.0f);
	vs_out.texCoords = texCoords;
}