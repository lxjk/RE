#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/PostProcessPassTex.incl"
#include "Include/Common.incl"
#include "Include/DeferredLighting.incl";

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

uniform sampler2D materialTex;

layout (location = 0) out vec4 color;

void main() 
{	
	vec2 uv = fs_in.texCoords;
	vec4 sceneColor = texture(gSceneColorTex, uv);	
	vec4 depthStencil = texture(gDepthStencilTex, uv);
	float depth = depthStencil.r;
	vec4 material = texture(materialTex, uv);
	float roughness = material.g;
	
	const float roughnessCutoff = 0.8;
	if(depth < 1 && roughness <= roughnessCutoff)
	{
		discard;
	}
	
	color = sceneColor;
}