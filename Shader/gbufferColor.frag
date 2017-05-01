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

out vec3 gNormal;
out vec3 gAlbedo;
out vec2 gMaterial;
out vec2 gVelocity;

uniform vec3 color;
uniform sampler2D normalTex;
uniform float metallic;
uniform float roughness;

void main() 
{	
	vec3 faceNormal = normalize(fs_in.normal);
	vec3 faceTangent = normalize(fs_in.tangent.xyz);
	mat3 TBN = mat3(faceTangent, cross(faceNormal, faceTangent) * fs_in.tangent.w, faceNormal);
	//gNormal = (TBN * normalize(texture(normalTex, fs_in.texCoords).rgb * 2.0f - 1.0f)) * 0.5f + 0.5f;
	gNormal = faceNormal * 0.5f + 0.5f;
	
	gAlbedo = color;
	//gAlbedo = vec3(1.0);
	
	gMaterial.r = metallic;
	gMaterial.g = roughness;
	
	vec2 velocity = fs_in.posCS.xy / fs_in.posCS.w - fs_in.prevPosCS.xy / fs_in.prevPosCS.w;
	gVelocity = EncodeVelocityToTexture(velocity);
}