#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/CommonVertexInput.incl"

out VS_OUT
{
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
	vec3 posVS;
	vec4 posCS;
	vec4 prevPosCS;
} vs_out;

uniform mat4 prevModelMat;
uniform mat4 modelMat;

void main()
{
	// output everything in view space	
	vs_out.posVS = (viewMat * modelMat * vec4(position, 1.0f)).xyz;
	vs_out.posCS = viewProjMat * modelMat * vec4(position, 1.0f);
	vs_out.prevPosCS = prevViewProjMat * prevModelMat * vec4(position, 1.0f);
	gl_Position = vs_out.posCS;
	
	// unjitter
	vs_out.posCS.xy += projMat[2].xy * vs_out.posCS.w;
	vs_out.prevPosCS.xy += prevProjMat[2].xy * vs_out.prevPosCS.w;
		
	vec3 normalScalar = vec3(
		dot(modelMat[0], modelMat[0]), 
		dot(modelMat[1], modelMat[1]), 
		dot(modelMat[2], modelMat[2]));		
	
	mat3 viewNormalMat = mat3(viewMat) * mat3(modelMat);	
	vs_out.normal = viewNormalMat * (normal / normalScalar);
	vs_out.tangent = vec4(viewNormalMat * tangent.xyz, tangent.w);
	
	vs_out.texCoords = texCoords;
}