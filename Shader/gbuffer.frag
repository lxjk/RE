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
layout (location = 1) out vec4 gAlbedo_ao;
layout (location = 2) out vec2 gMaterial;
layout (location = 3) out vec2 gVelocity;

uniform sampler2D diffuseTex;
uniform sampler2D normalTex;
uniform float metallic;
uniform float roughness;

void main() 
{	
	vec3 faceNormal = normalize(fs_in.normal);
	vec3 faceTangent = normalize(fs_in.tangent.xyz);
	vec3 faceBitangent = cross(faceNormal, faceTangent) * fs_in.tangent.w;
	//faceTangent = cross(faceBitangent, faceNormal) * fs_in.tangent.w;
	mat3 TBN = mat3(faceTangent, faceBitangent, faceNormal);
	gNormal = (TBN * normalize(texture(normalTex, fs_in.texCoords).rgb * 2.0f - 1.0f)) * 0.5f + 0.5f;
	//gNormal = faceNormal * 0.5f + 0.5f;
	
	gAlbedo_ao = vec4(texture(diffuseTex, fs_in.texCoords).rgb, 0);
	//gAlbedo_ao = vec4(1,1,1,0);
	//gAlbedo = vec3(fs_in.tangent.w * 0.5 + 0.5, 0.0, 0.0);
	
	gMaterial.r = metallic;
	gMaterial.g = roughness;
	
	vec2 velocity = fs_in.posCS.xy / fs_in.posCS.w - fs_in.prevPosCS.xy / fs_in.prevPosCS.w;
	gVelocity = EncodeVelocityToTexture(velocity);
}