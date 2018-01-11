#version 430 core

#include "Include/Common.incl"
#include "Include/DeferredLighting.incl"
#include "Include/Shadow.incl"
#include "Include/BasicFrag.incl"

in VS_OUT
{
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
	vec4 posVS;
} fs_in;


out vec4 color;

uniform sampler2D shadowMap[MAX_DIRECTIONAL_LIGHT_COUNT * MAX_CSM_COUNT];

uniform samplerCube skyTex;

void main() 
{	
	vec2 uv;
	vec3 albedo;
	vec3 normal;
	vec4 material;
	
	GetBasicValue(fs_in.texCoords, fs_in.normal, fs_in.tangent, uv, albedo, normal, material);
	
	// recover normal
	normal = normal * 2.f - 1.f;
	
	vec3 position = fs_in.posVS.rgb;
	vec3 view = normalize(-position);
	float metallic = material.r;
	float roughness = material.g;
	
	// directional lights
	vec3 ambient = albedo * (0.035);
	vec3 result = ambient;
	int shadowCount = 0;
	for(int i = 0; i < globalLightCount; ++i)
	{
		float shadowFactor = 1;
		int shadowDataCount = globalLights[i].shadowParamA;
		for(int c = 0; c < shadowDataCount; ++c)
		{
			if(-fs_in.posVS.z <= globalShadowData[shadowCount+c].bounds.z)
			{
				shadowFactor = CalcShadow(position, normal, -globalLights[i].directionRAB.xyz, globalShadowData[shadowCount+c].shadowMat, shadowMap[shadowCount+c], 0.0025, 0.002);
				break;
			}
		}
		shadowCount += shadowDataCount;
		//shadowFactor = 1;
		result += CalcLight(globalLights[i], normal, position, view, albedo, metallic, roughness) * shadowFactor;
		//result = vec3(shadowFactor);
	}
	
	// reflection
	vec3 dir = normalize(reflect(position, normal));
	vec4 reflectColor = texture(skyTex, dir);
	
	vec3 brdf = CalcReflectBRDF(dir, normal, albedo, metallic, roughness);
	
	const float roughnessCutoff = 0.8;
	float reflectRatio = (1-roughness);
	result += reflectColor.rgb * brdf * clamp((roughnessCutoff - roughness) / 0.2, 0.0, 1.0);

	
	color = vec4(result, tintColor.a);
}