#version 330 core

#include "Include/CommonUBO.incl"
#include "Include/CommonVertexInput.incl"

out VS_OUT
{
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
	vec4 posCS;
	vec4 prevPosCS;
} vs_out;

uniform mat4 prevModelMat;
uniform mat4 modelMat;
uniform mat3 normalMat;

void main()
{
	// output everything in view space
	
	vs_out.posCS = viewProjMat * modelMat * vec4(position, 1.0f);
	vs_out.prevPosCS = prevViewProjMat * prevModelMat * vec4(position, 1.0f);
	gl_Position = vs_out.posCS;
	
	// unjitter
	vs_out.posCS.xy += projMat[2].xy * vs_out.posCS.w;
	vs_out.prevPosCS.xy += prevProjMat[2].xy * vs_out.prevPosCS.w;
		
	// we can do this because mat3(viewMat) is guaranteed to be orthogonal (no scale)
	mat3 viewNormalMat = mat3(viewMat) * normalMat;	
	vs_out.normal = viewNormalMat * normal;
	vs_out.tangent = vec4(viewNormalMat * tangent.xyz, tangent.w);
	
	vs_out.texCoords = texCoords;
}