#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"
#include "Include/DeferredPassTex.incl"
#include "Include/Shadow.incl"

in VS_OUT
{
	vec3 positionVS;
} fs_in;

out vec4 color;

uniform Light light;
uniform mat4 shadowMat;
uniform sampler2D shadowMap;

uniform mat4 lightProjRemapMat;
uniform samplerCube shadowMapCube;

void main() 
{	
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	float depth = texture(gDepthStencilTex, uv).r;
	vec3 position = GetPositionVSFromDepth(depth, projMat, fs_in.positionVS);
	vec3 view = normalize(-position);	
	vec4 albedo = texture(gAlbedoTex, uv);
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	float ao = material.a;
	
	float shadowFactor = 1;
	if(light.shadowDataCount > 0)
	{
		vec3 lightDir = normalize(light.positionInvR.xyz - position);
		if(light.attenParams.y > 0.5f)
			shadowFactor = CalcShadow(position, normal, lightDir, shadowMat, shadowMap, 0.00015);
		else
			shadowFactor = CalcShadowCube(position, normal, lightDir, lightProjRemapMat, shadowMat, shadowMapCube, 0.0005);
	}
	
	vec3 result = CalcLight(light, normal, position, view, albedo.rgb, metallic, roughness) * min(shadowFactor, 1-ao);
	color = vec4(result, 1.0f);
	//color = vec4(vec3(shadowFactor), 1.f);
}