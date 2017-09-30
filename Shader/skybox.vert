#version 330 core

#include "Include/Common.incl"
#include "Include/CommonUBO.incl"
#include "Include/CommonVertexInput.incl"

out VS_OUT
{
	vec3 texCoords;
} vs_out;

void main()
{	
	vec4 posCS = projMat * vec4(mat3(viewMat) * position, 1.0f);
	// unjitter
	posCS.xy += projMat[2].xy * posCS.w;
	gl_Position = posCS.xyww;
	vs_out.texCoords = GetCubeMapCoord(position);
}