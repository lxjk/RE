#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"
#include "Include/DeferredPassTex.incl"
#include "Include/Shadow.incl"

#define MAX_LIGHT_COUNT 4
#define MAX_CSM_COUNT 3

struct ShadowData
{
	mat4 shadowMat;
	vec3 bounds; // x cascade width, y cascade height, z far plane
};

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform int lightCount;
uniform Light lights[MAX_LIGHT_COUNT];
uniform ShadowData shadowData[MAX_LIGHT_COUNT * MAX_CSM_COUNT];
//uniform sampler2DShadow shadowMap[MAX_LIGHT_COUNT * MAX_CSM_COUNT];
uniform sampler2D shadowMap[MAX_LIGHT_COUNT * MAX_CSM_COUNT];

const vec3 csmColor[MAX_CSM_COUNT] =
{
	vec3(1, 0, 0),
	vec3(0, 1, 0),
	vec3(0, 0, 1),
};


void main() 
{	
	vec2 uv = fs_in.texCoords;
	vec4 depthStencil = texture(gDepthStencilTex, uv);
	float depth = depthStencil.r;
	if(depth == 1)
		discard;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	//vec3 normal = texture(gNormalTex, fs_in.texCoords).rgb;
	vec3 position = GetPositionVSFromDepth(depth, projMat, fs_in.positionVS);
	vec3 view = normalize(-position);	
	vec3 albedo = texture(gAlbedoTex, uv).rgb;
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	
	vec3 ambient = vec3(0.01f) * albedo;
	vec3 result = ambient;
	vec3 csmColorCode = vec3(0);
	int shadowCount = 0;
	int clampedLightCount = min(lightCount, MAX_LIGHT_COUNT);
	for(int i = 0; i < clampedLightCount; ++i)
	{
		float shadowFactor = 1;
		for(int c = 0; c < lights[i].shadowDataCount; ++c)
		{
			if(-position.z <= shadowData[shadowCount+c].bounds.z)
			{
				shadowFactor = CalcShadow(position, normal, -lights[i].directionRAB.xyz, shadowData[shadowCount+c].shadowMat, shadowMap[shadowCount+c], 0.0025);
				csmColorCode = csmColor[c];
				break;
			}
		}
		shadowCount += lights[i].shadowDataCount;
		//shadowFactor = 0;
		result += CalcLight(lights[i], normal, position, view, albedo, metallic, roughness) * shadowFactor;
		//result = vec3(shadowFactor);
	}
	//result = mix(result, csmColorCode, 0.05f);
	color = vec4(result, 1.0f);
	//color = vec4(albedo, 1.0f);
	//color = vec4(metallic, roughness, 0.0f, 1.0f);
	gl_FragDepth = depth;

}