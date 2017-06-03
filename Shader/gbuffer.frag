#version 330 core

#include "Include/Common.incl"

in VS_OUT
{
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
	vec4 posCS;
	vec4 prevPosCS;
} fs_in;

layout (location = 0) out vec3 gNormal;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec4 gMaterial;
layout (location = 3) out vec2 gVelocity;

uniform vec3 color;
uniform int hasDiffuseTex;
uniform sampler2D diffuseTex;
uniform int hasNormalTex;
uniform sampler2D normalTex;
uniform float metallic;
uniform float roughness;
uniform int hasRoughnessTex;
uniform sampler2D roughnessTex;
uniform vec4 tile;

void main() 
{	
	vec2 uv = fs_in.texCoords * tile.xy + tile.zw;
	vec3 faceNormal = normalize(fs_in.normal);
	vec3 faceTangent = normalize(fs_in.tangent.xyz);
	vec3 faceBitangent = cross(faceNormal, faceTangent) * fs_in.tangent.w;
	//faceTangent = cross(faceBitangent, faceNormal) * fs_in.tangent.w;
	mat3 TBN = mat3(faceTangent, faceBitangent, faceNormal);
	if(hasNormalTex > 0)
		gNormal = (TBN * normalize(texture(normalTex, uv).rgb * 2.0f - 1.0f)) * 0.5f + 0.5f;
	else
		gNormal = faceNormal * 0.5f + 0.5f;
	
	if(hasDiffuseTex > 0)
		gAlbedo = vec4(texture(diffuseTex, uv).rgb, 0);
	else
		gAlbedo = vec4(color, 0);
	//gAlbedo = vec4(1,1,1,0);
	//gAlbedo = vec3(fs_in.tangent.w * 0.5 + 0.5, 0.0, 0.0);
	
	gMaterial = vec4(metallic, hasRoughnessTex > 0 ? 1 - texture(roughnessTex, uv).r : roughness, 0, 0);	
	
	vec2 velocity = fs_in.posCS.xy / fs_in.posCS.w - fs_in.prevPosCS.xy / fs_in.prevPosCS.w;
	gVelocity = EncodeVelocityToTexture(velocity);
}