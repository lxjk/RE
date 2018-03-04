#version 430 core

#include "Include/Common.incl"
#include "Include/DeferredLighting.incl"
#include "Include/Shadow.incl"
#include "Include/BasicFrag.incl"
// TODO: change this to alpha blend tex
#include "Include/DeferredPassTex.incl"

in VS_OUT
{
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
	vec4 posVS;
} fs_in;


out vec4 outColor;

uniform samplerCube skyTex;

void main() 
{	
	vec2 uv;
	vec4 color;
	vec3 normal;
	vec4 material;
	
	GetBasicValue(fs_in.texCoords, fs_in.normal, fs_in.tangent, uv, color, normal, material);
	vec3 albedo = color.rgb;
	float alpha = color.a;
	
	// recover normal
	normal = normal * 2.f - 1.f;
	
	vec3 position = fs_in.posVS.rgb;	
	float pixelDist = length(position);
	vec3 view = -position / pixelDist; //normalize(-position);
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
				shadowFactor = CalcShadowArray(position, normal, -globalLights[i].directionRAB.xyz, globalShadowData[shadowCount+c].shadowMat, gCSMTexArray, shadowCount+c, 0.0025, 0.002);
				break;
			}
		}
		shadowCount += shadowDataCount;
		//shadowFactor = 1;
		result += CalcLight(globalLights[i], normal, position, view, albedo, metallic, roughness) * shadowFactor;
		//result = vec3(shadowFactor);
	}
	
	// local lights
	// get tile idx
	uvec2 tileCoord = uvec2(gl_FragCoord.xy / LIGHT_TILE_SIZE);
	uint tileIdx = tileCoord.y * tileCountX + tileCoord.x;	
	uint tileInfoDataBase = tileIdx * LIGHT_TILE_INFO_STRIDE;
	float maxTileDepth = uintBitsToFloat(lightTileInfoData[tileInfoDataBase]);
	float minTileDepth = uintBitsToFloat(lightTileInfoData[tileInfoDataBase+1]);
	uint lightStartOffset = lightTileInfoData[tileInfoDataBase + 2];
	uint lightCount = lightTileInfoData[tileInfoDataBase + 3];
		
	uint slice = 0;
	if(pixelDist < minTileDepth)
		slice = uint(clamp((minTileDepth - pixelDist) / minTileDepth * (LIGHT_SLICES_PER_TILE - 1) + 1, 1, LIGHT_SLICES_PER_TILE-1));
	uint pixelMask = 1 << slice;
	if(pixelDist > maxLocalLightDist)
		lightCount = 0;
	
	for(uint i = 0; i < lightCount; ++i)
	{
#if LIGHT_SLICES_PER_TILE <= 16
		uint baseIdx = LIGHT_TILE_CULLING_RESULT_OFFSET + lightStartOffset + i;
		uint cullingResult = lightTileCullingResultInfo[baseIdx];
		if((cullingResult & pixelMask) == 0)
			continue;
		uint lightIdx = (cullingResult >> 16);
#else
		uint baseIdx = LIGHT_TILE_CULLING_RESULT_OFFSET + (lightStartOffset + i) * 2;
		uint lightIdx = lightTileCullingResultInfo[baseIdx];
		uint mask = lightTileCullingResultInfo[baseIdx + 1];
		if((mask & pixelMask) == 0)
			continue;
#endif
		Light light = localLights[lightIdx];
		
		float shadowFactor = 1;
		vec3 localLightResult = CalcLight(light, normal, position, view, albedo.rgb, metallic, roughness);
		int shadowParamA = light.shadowParamA;
		if(light.shadowParamA >= 0)
		{
			mat4 shadowMat = localLightsShadowMatrices[light.shadowParamA];
			vec3 lightDir = normalize(light.positionInvR.xyz - position);
			if(light.attenParams.y > 0.5f)
			{
				// spot light
				shadowFactor = CalcShadow(position, normal, lightDir, shadowMat, gShadowTiledTex, 0.00015, 0.0001);
			}
			else if(light.shadowParamB >= 0)
			{
				// point light cube map
				shadowFactor = CalcShadowCubeArray(position, normal, lightDir, 
					localLightsShadowMatrices[light.shadowParamA + 1], 
					shadowMat, gShadowCubeTexArray, light.shadowParamB, 0.0002, 0.0003);
			}
			else
			{
				// point light tetrahedron map
				vec4 posLVS = shadowMat * vec4(position, 1.f);
				int faceIndex = GetTetrahedronIndex(posLVS);
				shadowFactor = CalcShadowTetrahedronSingle(posLVS, normal, lightDir, 
					localLightsShadowMatrices[light.shadowParamA + 1 + faceIndex], 
					shadowMat, gShadowTiledTex, 0.00025, 0.0004);
			}
		}
		
		result += localLightResult * shadowFactor;
	}
	
	// reflection
	vec3 dir = normalize(reflect(position, normal));
	vec4 reflectColor = texture(skyTex, dir);
	
	vec3 brdf = CalcReflectBRDF(dir, normal, albedo, metallic, roughness);
	
	const float roughnessCutoff = 0.8;
	float reflectRatio = (1-roughness);
	result += reflectColor.rgb * brdf * clamp((roughnessCutoff - roughness) / 0.2, 0.0, 1.0);
	
	outColor = vec4(result, alpha);
}