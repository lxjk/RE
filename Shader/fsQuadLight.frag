#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"
#include "Include/DeferredPassTex.incl"
#include "Include/Shadow.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

//uniform int lightCount;
//uniform Light lights[MAX_DIRECTIONAL_LIGHT_COUNT];
//uniform ShadowData shadowData[MAX_DIRECTIONAL_LIGHT_COUNT * MAX_CSM_COUNT];
//uniform sampler2DShadow shadowMap[MAX_DIRECTIONAL_LIGHT_COUNT * MAX_CSM_COUNT];
uniform sampler2D shadowMap[MAX_DIRECTIONAL_LIGHT_COUNT * MAX_CSM_COUNT];

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
	//if(depth == 1)
	//	discard;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	//vec3 normal = texture(gNormalTex, fs_in.texCoords).rgb;
	vec3 position = GetPositionVSFromDepth(depth, projMat, fs_in.positionVS);
	vec3 view = normalize(-position);
	vec4 albedo = texture(gAlbedoTex, uv);
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	float ao = material.a;
	
	vec3 ambient = albedo.rgb * (0.035 * (1-ao));
	vec3 result = ambient;
	vec3 csmColorCode = vec3(0);
	int shadowCount = 0;
	for(int i = 0; i < globalLightCount; ++i)
	{
		float shadowFactor = 1;
		int shadowDataCount = globalLights[i].shadowParamA;
		for(int c = 0; c < shadowDataCount; ++c)
		{
			if(-position.z <= globalShadowData[shadowCount+c].bounds.z)
			{
				shadowFactor = CalcShadow(position, normal, -globalLights[i].directionRAB.xyz, globalShadowData[shadowCount+c].shadowMat, shadowMap[shadowCount+c], 0.0025, 0.002);
				csmColorCode = csmColor[c];
				break;
			}
		}
		shadowCount += shadowDataCount;
		//shadowFactor = 0;
		result += CalcLight(globalLights[i], normal, position, view, albedo.rgb, metallic, roughness) * min(shadowFactor, 1-ao);
		//result = vec3(shadowFactor);
	}
	//result = mix(result, csmColorCode, 0.05f);
	color = vec4(result, 1.0f);
	//color = vec4(vec3(1-albedo_ao.a), 1.0f);
	//color = vec4(metallic, roughness, 0.0f, 1.0f);
	//gl_FragDepth = depth;

}