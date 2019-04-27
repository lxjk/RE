#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/Lighting.incl"
#include "Include/DeferredPassTex.incl"
#include "Include/Shadow.incl"
#include "Include/BasicLightVolumeFrag.incl"

in VS_OUT
{
	vec3 positionVS;
} fs_in;

out vec4 color;

//uniform int lightIndex;
uniform Light light;
uniform mat4 shadowMat;

void main() 
{	
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	
	vec3 normal, position, view;
	vec4 albedo;
	float metallic, roughness, ao;	
	GetGBufferValue(uv, fs_in.positionVS, normal, position, view, albedo, metallic, roughness, ao);
	
	//Light light = localLights[lightIndex];
	//int matIdx = light.shadowParamA;
	//mat4 shadowMat = localLightsShadowMatrices[matIdx];
	
	float shadowFactor = 1;
	if(light.shadowParamA >= 0)
	{
		vec3 lightDir = normalize(light.positionInvR.xyz - position);
		shadowFactor = CalcShadow(position, normal, lightDir, shadowMat, gShadowTiledTex, 0.00015, 0.0001);
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
			shadowFactor = CalcShadowFog(samplePoint, lightDir, shadowMat, gShadowTiledTex, 0.000002);
		}
		//debugValue.a = min(shadowFactor, debugValue.a);
		fog += atten * shadowFactor;
	}
	fog = fog * fogIntensity / sampleCount;
	

	color = vec4(result + fog * light.color.rgb, 1.0f);
	
	//color = vec4(vec3(shadowFactor), 1.f);
}

