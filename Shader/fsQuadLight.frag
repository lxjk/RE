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
		vec4 posWS = (invViewMat * vec4(position, 1.f));
		float shadowFactor = 1;
		for(int c = 0; c < lights[i].shadowDataCount; ++c)
		{
			if(position.z <= lights[i].shadowData[c].dist)
			{
				// convert to light clip space
				vec4 posLCS = lights[i].shadowData[c].lightViewProjMat * posWS;
				posLCS /= posLCS.w;
				float posDepth = posLCS.z * 0.5 + 0.5;
				float shadowMapDepth = texture(lights[i].shadowData[c].shadowMap, (posLCS.xy * 0.5 + 0.5)).r;
				shadowFactor = posDepth < shadowMapDepth ? 0 : 1;
				break;
			}
		}
		result += CalcLight(lights[i], normal, position, view, albedo, metallic, roughness) * shadowFactor;
	}
	color = vec4(result, 1.0f);
	//color = vec4(albedo, 1.0f);
	//color = vec4(metallic, roughness, 0.0f, 1.0f);
	gl_FragDepth = depth;

}