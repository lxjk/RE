#version 330 core

#include "Include/CommonUBO.incl"
#include "Include/CommonVertexInput.incl"

out VS_OUT
{
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
} vs_out;

uniform mat4 modelMat;
uniform mat3 normalMat;

void main()
{
	// output everything in view space
	
	gl_Position = viewProjMat * modelMat * vec4(position, 1.0f);
	
	// we can do this because mat3(viewMat) is guaranteed to be orthogonal (no scale)
	mat3 viewNormalMat = mat3(viewMat) * normalMat;	
	vs_out.normal = viewNormalMat * normal;
	vs_out.tangent = vec4(viewNormalMat * tangent.xyz, tangent.w);
	
	vs_out.texCoords = texCoords;
}