#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"
#include "Include/DeferredPassTex.incl"
#include "Include/Shadow.incl"

in VS_OUT
{
	vec3 positionVS;
} fs_in;


layout (location = 0) out vec4 color;
//layout (location = 1) out vec4 debugValue;

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
	if(light.shadowParamA >= 0)
	{
		vec3 lightDir = normalize(light.positionInvR.xyz - position);
		if(light.attenParams.y > 0.5f)
			shadowFactor = CalcShadow(position, normal, lightDir, shadowMat, shadowMap, 0.00015, 0.0001);
		else
			shadowFactor = CalcShadowCube(position, normal, lightDir, lightProjRemapMat, shadowMat, shadowMapCube, 0.0005, 0.0003);
	}
	
	vec3 result = CalcLight(light, normal, position, view, albedo.rgb, metallic, roughness) * min(shadowFactor, 1-ao);
	
	const float fogIntensity = 0.1;
	const int sampleCount = 6;
	float fog = 0;
	vec3 startPos = (fs_in.positionVS.z > position.z) ? fs_in.positionVS : position;
	//vec3 startPos = fs_in.positionVS;
	vec3 endPos = startPos * (-resolution.z / startPos.z);
	vec3 sampleStep = (endPos-startPos) / sampleCount;	
	float randOffset = GetRandom(vec4(position * 100, time));	
	vec3 samplePoint = startPos - sampleStep * randOffset;
	for(int i = 0; i < sampleCount; ++i)
	{
		samplePoint += sampleStep;
		float sampleShadowFactor = 1;
		float atten;
		vec3 lightDir;
		GetAttenuationAndLightDir(light, samplePoint, atten, lightDir);
		if(light.shadowParamA >= 0)
		{
			if(light.attenParams.y > 0.5f)
				shadowFactor = CalcShadowFog(samplePoint, lightDir, shadowMat, shadowMap, 0.000002);
			else
				shadowFactor = CalcShadowCubeFog(samplePoint, lightDir, lightProjRemapMat, shadowMat, shadowMapCube, 0.0005);
		}
		//debugValue.a = min(shadowFactor, debugValue.a);
		fog += atten * shadowFactor;
	}
	fog = fog * fogIntensity / sampleCount;
	
	color = vec4(result + fog * light.color.rgb, 1.0f);
	//color = vec4(light.color, 1.0);
	//color = vec4(vec3(shadowFactor), 1.f);
}