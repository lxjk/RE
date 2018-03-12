#version 430 core

#include "Include/DeferredPassTex.incl"
#include "Include/CommonLighting.incl"

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
	vec3 albedo = texture(gAlbedoTex, uv).rgb;
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	float ao = material.a;
	
	vec3 lightingResult = vec3(0);
	
	// global lights
	CalcGlobalLights(lightingResult, position, normal, view, albedo, metallic, roughness, ao);
	
	// local lights
	CalcTiledLocalLights(lightingResult, uv * (resolution.xy), position, normal, view, albedo, metallic, roughness, ao);

	color = vec4(lightingResult, 0.0);
	
	//float firstSliceMinDepth = maxTileDepth - maxTileDepth / float(LIGHT_SLICES_PER_TILE);
	//float ratio = (minTileDepth > firstSliceMinDepth) ? 1 : 0;
	//ratio = (slice == 0) ? 1 : 0;
	//ratio = (maxLocalLightDist > 127) ? 1 : 0;
	//color = vec4(1-ratio, ratio, 0, 0);
}