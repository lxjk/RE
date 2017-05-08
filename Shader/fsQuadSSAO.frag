#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/DeferredPassTex.incl"
#include "Include/Common.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

layout (location = 0) out vec4 color;
//layout (location = 1) out vec4 debugValue;

// ref: https://www.gamedev.net/resources/_/technical/graphics-programming-and-theory/a-simple-and-practical-approach-to-ssao-r2753
float CalcSampleAO(vec2 offset, vec3 position, vec3 normal, vec2 uv, float sampleRadius, vec2 sampleScale)
{	
	float sampleDepth = texture(gDepthStencilTex, uv + offset * sampleScale).r;
	vec3 samplePos = GetPositionVSFromDepth(sampleDepth, projMat, position + vec3(offset, 0) * sampleRadius);	
	
	vec3 diff = samplePos - position;
	vec3 dir = normalize(diff);
	float d = length(diff);
	float rangeCheck = smoothstep(0.0, 1.0, 32 * abs(position.z - samplePos.z) / sampleRadius);
	//rangeCheck = 1;
	float dp = max(dot(normal, dir), 0);
	return dp * dp * sampleRadius * 3.5 / (d + 0.01) * rangeCheck;
}

void main() 
{	
	vec2 uv = fs_in.texCoords;
	vec4 depthStencil = texture(gDepthStencilTex, uv);
	float depth = depthStencil.r;
	if(depth == 1)
		discard;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	vec3 position = GetPositionVSFromDepth(depth, projMat, fs_in.positionVS);
	
	float ao = 0;
	float sampleRadius = 0.08 ; // in z = -1 plane
	vec2 sampleScale = sampleRadius / -position.z * vec2(projMat[0][0], projMat[1][1]) * 0.5; // in [-0.5, 0.5] space
	const int sampleCount = 9;
	float randV = GetRandom(vec4(position * 100, time));
	float randCos = abs(randV * 2 - 1) * 2 - 1; // [0,1] -> [-1, 0 ,1] -> [1, -1, 1]
	float randSin = sqrt(1 - randCos*randCos) * (randV > 0.5 ? -1 : 1);
	for(int sampleIdx = 0; sampleIdx < sampleCount; ++sampleIdx)
	{
		vec2 offset = PoissonDisk9[sampleIdx];
		// rotate
		offset = vec2(
			offset.x * randCos - offset.y * randSin,
			offset.x * randSin + offset.y * randCos
		);
		
		ao += CalcSampleAO(offset, position, normal, uv, sampleRadius, sampleScale);
	}
	
	//color.a= ao;
	color.a = ao / sampleCount;
	//color.a = smoothstep(0.1, 0.75, ao / sampleCount);
	//debugValue.a = color.a;
	//color.a = -position.z / 20;
}