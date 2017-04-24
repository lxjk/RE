#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"
#include "Include/DeferredPassTex.incl"

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
/*
const vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
 );
 */
 
 const vec2 poissonDisk[9] = vec2[](
	vec2(0.6701438f, 0.0797466f),
	vec2(-0.3487887f, 0.1811505f),
	vec2(-0.1736667f, -0.394125f),
	vec2(0.7539324f, -0.6149955f),
	vec2(-0.02059382f, -0.9826667f),
	vec2(-0.9730584f, 0.001460005f),
	vec2(-0.1840728f, 0.8050078f),
	vec2(0.4536706f, 0.7639304f),
	vec2(-0.7440495f, -0.5962256f)
 );
 

const float W5[5][5] =
{
    { 0.0,0.5,1.0,0.5,0.0 },
    { 0.5,1.0,1.0,1.0,0.5 },
    { 1.0,1.0,1.0,1.0,1.0 },
    { 0.5,1.0,1.0,1.0,0.5 },
    { 0.0,0.5,1.0,0.5,0.0 }
};

float GetRandom(vec4 seed4)
{
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float GetBias(vec3 normal, vec3 light, int cascade)
{
	float cosAlpha = dot(normal, light);
	float sinAlpha = sqrt(1.0 - cosAlpha * cosAlpha);
	return (0.0025 /*+ 0.0015 * cascade*/) * sinAlpha / cosAlpha;
	//float bias = 0.004 * (1.0 - abs(dot(normal, -lights[i].directionRAB.xyz)));  
	//float bias = 0.008;
}

float CalcShadow(vec3 viewPos, mat4 shadowMat, sampler2D shadowMap, float bias)
{
	// convert to light clip space
	vec4 posLCS = shadowMat * vec4(viewPos, 1.f);
	posLCS.z -= posLCS.w * bias;
	posLCS /= posLCS.w;
	return posLCS.z <= texture(shadowMap, posLCS.xy).r ? 1 : 0;
} 

float CalcShadowLerp(vec3 viewPos, mat4 shadowMat, sampler2DShadow shadowMap, float bias)
{
	// convert to light clip space
	vec4 posLCS = shadowMat * vec4(viewPos, 1.f);
	posLCS.z -= posLCS.w * bias;
	return textureProj(shadowMap, posLCS);
} 

float CalcShadowTap5(vec3 viewPos, mat4 shadowMat, sampler2D shadowMap, float bias)
{
	// convert to light clip space
	vec4 posLCS = shadowMat * vec4(viewPos, 1.f);
	posLCS.z -= posLCS.w * bias;
	posLCS /= posLCS.w;
	vec2 mapSize = textureSize(shadowMap, 0);
	//vec2 sampleScale = 1.f * posLCS.w / mapSize;
	vec2 sampleScale = 1.f / mapSize;
	const int steps = 2;
	const float depthRange = bias;
	float sumWeights = 0;
	float depthDiff = 0;
	for(int xIdx = -steps; xIdx <= steps; ++xIdx)
	{
		for(int yIdx = -steps; yIdx <= steps; ++yIdx)
		{
			float weight = W5[xIdx + steps][yIdx + steps];
			vec2 uv = posLCS.xy + vec2(xIdx, yIdx) * sampleScale;
			//if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
			//	continue;				
			//depthDiff += (texture(shadowMap, uv).r - posLCS.z) * weight;
			depthDiff += clamp(texture(shadowMap, uv).r - posLCS.z, -depthRange, depthRange) * weight;
			//shadowFactor += posLCS.z <= texture(shadowMap, uv).r ? weight : 0;
			sumWeights += weight;
		}
	}
	depthDiff /= sumWeights;
	//return depthDiff >= 0 ? 1 : 0;
	return smoothstep(-depthRange * 0.5f, depthRange * 0.5f, depthDiff);
} 

float CalcShadowPoisson(vec3 viewPos, mat4 shadowMat, sampler2D shadowMap, float bias)
{
	// convert to light clip space
	vec4 posLCS = shadowMat * vec4(viewPos, 1.f);
	posLCS.z -= posLCS.w * bias;
	posLCS /= posLCS.w;
	vec2 mapSize = textureSize(shadowMap, 0);
	vec2 sampleScale = 1.5 / mapSize;
	const int sampleCount = 9;
	const float depthRange = bias;
	float depthDiff = 0;
	float randV = GetRandom(vec4(viewPos * 100, time));
	float randCos = abs(randV * 2 - 1) * 2 - 1; // [0,1] -> [-1, 0 ,1] -> [1, -1, 1]
	float randSin = sqrt(1 - randCos*randCos) * (randV > 0.5 ? -1 : 1);
	for(int sampleIdx = 0; sampleIdx < sampleCount; ++sampleIdx)
	{
		vec2 offset = poissonDisk[sampleIdx];
		// rotate
		offset = vec2(
			offset.x * randCos - offset.y * randSin,
			offset.x * randSin + offset.y * randCos
		);
		vec2 uv = posLCS.xy + offset * sampleScale;
		//depthDiff += (texture(shadowMap, uv).r - posLCS.z);
		depthDiff += clamp(texture(shadowMap, uv).r - posLCS.z, -depthRange, depthRange);
	}
	depthDiff /= float(sampleCount);
	//return depthDiff >= 0 ? 1 : 0;
	return smoothstep(-depthRange * 0.5f, depthRange * 0.5f, depthDiff);
} 

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
				// convert to light clip space
				float bias = GetBias(normal, -lights[i].directionRAB.xyz, c);
				//shadowFactor = CalcShadowTap5(position, shadowData[shadowCount+c].shadowMat, shadowMap[shadowCount+c], bias);
				shadowFactor = CalcShadowPoisson(position, shadowData[shadowCount+c].shadowMat, shadowMap[shadowCount+c], bias);
				//shadowFactor = CalcShadow(position, shadowData[shadowCount+c].shadowMat, shadowMap[shadowCount+c], bias);
				//shadowFactor = CalcShadowLerp(position, shadowData[shadowCount+c].shadowMat, shadowMap[shadowCount+c], bias);
				csmColorCode = csmColor[c];
				break;
			}
		}
		shadowCount += lights[i].shadowDataCount;
		result += CalcLight(lights[i], normal, position, view, albedo, metallic, roughness) * shadowFactor;
		//result = vec3(shadowFactor);
	}
	//result = mix(result, csmColorCode, 0.05f);
	color = vec4(result, 1.0f);
	//color = vec4(albedo, 1.0f);
	//color = vec4(metallic, roughness, 0.0f, 1.0f);
	gl_FragDepth = depth;

}