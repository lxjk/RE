#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/CommonVertexInput.incl"

out VS_OUT
{
	vec3 positionVS;
} vs_out;

uniform mat4 modelMat;

void main()
{	
	vec4 posVS = viewMat * modelMat * vec4(position, 1.0f);
	gl_Position = projMat * posVS;
	vs_out.positionVS = posVS.xyz;
}