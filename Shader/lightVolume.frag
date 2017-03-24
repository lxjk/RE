#version 330 core

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"

in VS_OUT
{
	vec3 positionVS;
} fs_in;

out vec4 color;

uniform sampler2D gPositionTex;
uniform sampler2D gNormalTex;
uniform sampler2D gAlbedoTex;
uniform sampler2D gMaterialTex;
uniform sampler2D gDepthStencilTex;

uniform Light light;

void main() 
{	
	vec2 uv = gl_FragCoord.xy / resolution;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	float depth = texture(gDepthStencilTex, uv).r;
	vec3 position = GetGBufferPositionVS(depth, projMat, fs_in.positionVS);
	vec3 view = normalize(-position);	
	vec3 albedo = texture(gAlbedoTex, uv).rgb;
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	
	vec3 result = CalcLight(light, normal, position, view, albedo, metallic, roughness);
	color = vec4(result, 1.0f);
}