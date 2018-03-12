#version 430 core

// TODO: change this to alpha blend tex
#include "Include/DeferredPassTex.incl"
#include "Include/CommonLighting.incl"
#include "Include/BasicFrag.incl"

in VS_OUT
{
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
	vec3 posVS;
	vec4 posCS;
	vec4 prevPosCS;
} fs_in;


layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 gVelocity;
//layout (location = 1) out vec3 gNormal;
//layout (location = 2) out vec4 gMaterial;
//layout (location = 3) out vec2 gVelocity;

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
	vec2 uv;
	vec4 color;
	vec3 normal;
	vec4 material;
	
	GetBasicValue(fs_in.texCoords, fs_in.normal, fs_in.tangent, uv, color, normal, material);
	vec3 albedo = color.rgb;
	float alpha = color.a;
	// change uv to pixel in screen space
	uv = vec2(gl_FragCoord.xy) / (resolution.xy );
	
	//gNormal = normal;
	//gMaterial = material;
	
	vec2 velocity = fs_in.posCS.xy / fs_in.posCS.w - fs_in.prevPosCS.xy / fs_in.prevPosCS.w;
	gVelocity = EncodeVelocityToTexture(velocity);
	
	// recover normal
	normal = normal * 2.f - 1.f;
	
	vec3 position = fs_in.posVS;

	vec3 view = normalize(-position);
	float metallic = material.r;
	float roughness = material.g;
	
	
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
	ao = ao / sampleCount;
	//gMaterial.a = ao;
	
	vec3 result = vec3(0);
	
	// global lights
	CalcGlobalLights(result, position, normal, view, albedo, metallic, roughness, ao);	
	
	// local lights	
	CalcTiledLocalLights(result, gl_FragCoord.xy, position, normal, view, albedo, metallic, roughness, ao);
	
	outColor = vec4(result, alpha);
}