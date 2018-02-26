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

void main() 
{	
	vec2 uv = fs_in.texCoords;
	
	vec4 depthStencil = texture(gDepthStencilTex, uv);
	float depth = depthStencil.r;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	vec3 position = GetPositionVSFromDepth(depth, projMat, fs_in.positionVS);
	vec3 view = normalize(-position);
	vec4 albedo = texture(gAlbedoTex, uv);
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	float ao = material.a;
	
	vec3 lightingResult = vec3(0);
	
	// global lights
	vec3 ambient = albedo.rgb * (0.035 * (1-ao));
	lightingResult = ambient;
	int shadowCount = 0;
	for(int i = 0; i < globalLightCount; ++i)
	{
		float shadowFactor = 1;
		int shadowDataCount = globalLights[i].shadowParamA;
		for(int c = 0; c < shadowDataCount; ++c)
		{
			if(-position.z <= globalShadowData[shadowCount+c].bounds.z)
			{
				shadowFactor = CalcShadowArray(position, normal, -globalLights[i].directionRAB.xyz, globalShadowData[shadowCount+c].shadowMat, gCSMTexArray, shadowCount+c, 0.0025, 0.002);
				break;
			}
		}
		shadowCount += shadowDataCount;
		lightingResult += CalcLight(globalLights[i], normal, position, view, albedo.rgb, metallic, roughness) * min(shadowFactor, 1-ao);
	}
	
	// local lights
	// get tile idx
	uvec2 tileCoord = uvec2(uv * (resolution.xy - 1) / LIGHT_TILE_SIZE);
	uint tileIdx = tileCoord.y * tileCountX + tileCoord.x;	
	uint tileInfoDataBase = tileIdx * LIGHT_TILE_INFO_STRIDE;
	float maxTileDepth = uintBitsToFloat(lightTileInfoData[tileInfoDataBase]);
	float minTileDepth = uintBitsToFloat(lightTileInfoData[tileInfoDataBase+1]);
	uint lightStartOffset = lightTileInfoData[tileInfoDataBase + 2];
	uint lightCount = lightTileInfoData[tileInfoDataBase + 3];
	
	float pixelDepth = length(position);
		
	uint slice = 0;
	if(pixelDepth < minTileDepth)
		slice = uint(clamp((minTileDepth - pixelDepth) / minTileDepth * (LIGHT_SLICES_PER_TILE - 1) + 1, 1, LIGHT_SLICES_PER_TILE-1));
	uint pixelMask = 1 << slice;
	if(pixelDepth > maxLocalLightDist)
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
		
		lightingResult += localLightResult * min(shadowFactor, 1-ao);
	}

	color = vec4(lightingResult, 0.0);
	
	//float firstSliceMinDepth = maxTileDepth - maxTileDepth / float(LIGHT_SLICES_PER_TILE);
	//float ratio = (minTileDepth > firstSliceMinDepth) ? 1 : 0;
	//ratio = (slice == 0) ? 1 : 0;
	//ratio = (maxLocalLightDist > 127) ? 1 : 0;
	//color = vec4(1-ratio, ratio, 0, 0);
}