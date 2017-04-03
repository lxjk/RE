#version 330 core

#define MAX_LIGHT_COUNT 4

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"
#include "Include/DeferredPassTex.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform int lightCount;
uniform Light lights[MAX_LIGHT_COUNT];

void main() 
{	
	vec2 uv = fs_in.texCoords;
	vec4 depthStencil = texture(gDepthStencilTex, uv);
	float depth = depthStencil.r;
	if(depth == 1)
		discard;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	//vec3 normal = texture(gNormalTex, fs_in.texCoords).rgb;
	vec3 position = GetGBufferPositionVS(depth, projMat, fs_in.positionVS);
	vec3 view = normalize(-position);	
	vec3 albedo = texture(gAlbedoTex, uv).rgb;
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	
	vec3 ambient = vec3(0.01f) * albedo;
	vec3 result = ambient;
	int clampedLightCount = min(lightCount, MAX_LIGHT_COUNT);
	for(int i = 0; i < clampedLightCount; ++i)
	{
		result += CalcLight(lights[i], normal, position, view, albedo, metallic, roughness);
	}
	color = vec4(result, 1.0f);
	//color = vec4(albedo, 1.0f);
	//color = vec4(metallic, roughness, 0.0f, 1.0f);
	gl_FragDepth = depth;

}