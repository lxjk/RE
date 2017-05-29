#version 430 core

#include "Include/CommonUBO.incl"
#include "Include/PostProcessPassTex.incl"
#include "Include/Common.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

uniform sampler2D albedoTex;
uniform sampler2D normalTex;
uniform sampler2D materialTex;
uniform samplerCube skyTex;

layout (location = 0) out vec4 color;
//layout (location = 1) out vec4 debugValue;

vec4 GetReflectColor1(vec3 position, vec3 dir)
{
	float reflectStep = 0.1;
	
	vec4 reflectColor = texture(skyTex, dir);
	//debugValue.xyz = vec3(-1);
	for(int i = 0; i < 64; ++i)
	{
		vec3 newPos = position + dir * i * reflectStep;
		vec4 newPosNS = projMat * vec4(newPos, 1);
		newPosNS.xyz = newPosNS.xyz / newPosNS.w * 0.5 + 0.5;
		if(newPosNS.x > 1 || newPosNS.x < 0 || newPosNS.y > 1 || newPosNS.y < 0 || newPosNS.z > 1 || newPosNS.z < 0)
		{
			break;
		}
		float newDepth = texture(gDepthStencilTex, newPosNS.xy).r;
		//debugValue.x = newDepth;
		//debugValue.y = newPosNS.z;
		if(newDepth < newPosNS.z - 0.000001)
		{
			vec3 newVS = GetPositionVSFromDepth(newDepth, projMat, newPos);
			if(newVS.z - newPos.z < 0.3)
			{
				reflectColor = texture(gSceneColorTex, newPosNS.xy);
				//debugValue.xyz = newPosNS.xyz;
				//debugValue.w = newDepth;
				break;
			}
		}
		
		//reflectColor = texture(gSceneColorTex, newPosNS.xy);
	}
	
	return reflectColor;
}

// ref: http://casual-effects.blogspot.com/2014/08/screen-space-ray-tracing.html
vec4 GetReflectColor2(vec2 startPosSS, vec3 startPosVS, vec3 dir, vec3 normal, float maxDist, float rand)
{	
	vec4 reflectColor = texture(skyTex, mat3(invViewMat) * dir);
	
	// clamp ray length to near plane
	float nearPlanZ = -resolution.z;
	float rayLength = (startPosVS.z + dir.z * maxDist) > nearPlanZ ? (nearPlanZ - startPosVS.z) / dir.z : maxDist;
	
	vec3 endPosVS = startPosVS + dir * rayLength;
	vec4 endPosCS = projMat * vec4(endPosVS, 1);
	vec2 endPosSS = (endPosCS.xy / endPosCS.w) * 0.5 + 0.5;
	
	float startK = 1.0 / startPosVS.z;
	float endK = 1.0 / endPosVS.z;
	float deltaK = (endK - startK);
	
	vec2 size = resolution.xy;
	vec2 deltaSS = endPosSS - startPosSS;
	vec2 deltaPS = deltaSS.xy * size;
	if(dot(deltaPS, deltaPS) < 0.01)
		return reflectColor;
		
	bool permute = false;
	if(abs(deltaPS.x) < abs(deltaPS.y))
	{
		permute = true;
		deltaPS = deltaPS.yx;
		deltaSS = deltaSS.yx;
		startPosSS = startPosSS.yx;
		endPosSS = endPosSS.yx;
		size = size.yx;
	}
	
	// calculate stride, use incremental stride to get better result near contact point
	float xDistSS = abs(clamp(endPosSS.x, 0.0, 1.0) - startPosSS.x);
	const int maxSteps = 8;
	//const float strideRate = 1.25;
	const float strideRate = 0.15;
	float dStride = xDistSS * strideRate / maxSteps;
	float stride = max(xDistSS * (1 - 0.5 * strideRate * (maxSteps - 1)) / maxSteps, 1.0 / size.x);
	//float stride = 2.0 / size.x;
	//float dStride = 2 * (xDistSS - stride * maxSteps) / maxSteps / (maxSteps - 1);
	//float stride = max(xDistSS * (1-strideRate) / (1-pow(strideRate, maxSteps)), 1.0 / size.x);
	//float stride = max(xDistSS / maxSteps, 1.0 / size.x);
	//debugValue.x = stride * size.x;
	
	float stepDir = sign(deltaSS.x);
	float invdx = stepDir / deltaSS.x;
	
	float halfStepRatio = 0.5 * invdx;
	vec2 dStepSS = deltaSS * invdx;
	
	float end = endPosSS.x * stepDir;
	
	float prevZVS = startPosVS.z;
	vec2 newPosSS = startPosSS + rand * dStepSS * (stride + dStride * maxSteps * 0.75); //* pow(strideRate, maxSteps * 0.6);
	float zMinVS, zMaxVS, sceneZMaxVS;
	float rayRatio = 0;
	bool bFound = false;
	vec2 newUV;
	for(int i = 0; (i < maxSteps) && (newPosSS.x * stepDir <= end); ++i)
	{
		zMinVS = prevZVS;
		float rayRatioSS = (newPosSS.x - startPosSS.x) / deltaSS.x;
		// 0.5 here is to offset depth forward half step, with prevZVS, we have a range of half step ahead and half step behind
		prevZVS = 1.0 / (startK + deltaK * min(rayRatioSS + halfStepRatio * stride, 1.0));
		zMaxVS = prevZVS;
		if(zMinVS > zMaxVS)
		{
			float tmp = zMinVS;
			zMinVS = zMaxVS;
			zMaxVS = tmp;
		}
		if(i == 0)
		{
			// compensate for the first test to avoid falsely reflect on itself
			zMinVS = zMinVS + (zMaxVS - zMinVS) * max(max(-dir.z, normal.z), 0);
		}
		
		if(newPosSS.x < 0 || newPosSS.x > 1 || newPosSS.y < 0 || newPosSS.y > 1)
			break;
				
		newUV = permute ? newPosSS.yx : newPosSS.xy;
		float sceneDepth = texture(gDepthStencilTex, newUV).r;
		if(sceneDepth == 1)
			break;
		sceneZMaxVS = GetViewZFromDepth(sceneDepth, projMat);

		// adjust thickness based on how far we are from the camera, this helps reflection far away
		float thickness = 0.65 + (-sceneZMaxVS) * 0.06;
		if((zMaxVS >= sceneZMaxVS - thickness) && (zMinVS < sceneZMaxVS - 0.02)) // add a const on zMinVS test to avoid banding
		{
			bFound = true;
			// convert ray ratio to view space
			rayRatio = rayRatioSS * endK / (startK + deltaK * rayRatioSS);
			//debugValue.w = i;
			//debugValue.x = rayRatioSS;
			//debugValue.y = rayRatio;
			break;
		}
		
		newPosSS += dStepSS * stride;
		//stride *= strideRate;
		stride += dStride;
	}
	
	if(bFound)	
		reflectColor = mix(texture(gSceneColorTex, newUV), reflectColor, rayRatio * rayRatio);
	
	//debugValue.x = zMinVS;
	//debugValue.y = zMaxVS;
	//debugValue.z = sceneZMaxVS;
	
	return reflectColor;
}

void main() 
{	
	vec2 uv = fs_in.texCoords;
	vec4 sceneColor = texture(gSceneColorTex, uv);
	vec4 depthStencil = texture(gDepthStencilTex, uv);
	float depth = depthStencil.r;
	if(depth == 1)
	{
		color = sceneColor;
		return;
	}
	vec3 albedo = texture(albedoTex, uv).rgb;
	vec3 normal = normalize(texture(normalTex, uv).rgb * 2.0f - 1.0f);
	vec3 position = GetPositionVSFromDepth(depth, projMat, fs_in.positionVS);
	vec4 material = texture(materialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	float ao = material.a;
	
	vec3 dir = normalize(reflect(position, normal));
	
	float randOffset = GetRandom(vec4(position * 100, time));	
	float randV = GetRandom(vec4(position.yxz * 100, time));	
	float randCos = abs(randV * 2 - 1) * 2 - 1; // [0,1] -> [-1, 0 ,1] -> [1, -1, 1]
	float randSin = sqrt(1 - randCos*randCos) * (randV > 0.5 ? -1 : 1);
	
	//vec4 reflectColor = GetReflectColor1(position, dir);
	//vec4 reflectColor = GetReflectColor2(uv, position, dir, normal, 4, randOffset);
	
	vec4 reflectColor = vec4(0,0,0,0);
	
	vec3 up = normal;
	vec3 right = cross(dir, up);
	up = cross(right, dir);	
	const int sampleCount = 2;
	for(int sampleIdx = 0; sampleIdx < sampleCount; ++sampleIdx)
	{
		vec2 offset = PoissonDisk4[sampleIdx];
		// rotate
		offset = vec2(
			offset.x * randCos - offset.y * randSin,
			offset.x * randSin + offset.y * randCos
		) * (roughness * 0.25);
		vec3 sampleDir = normalize(dir + offset.x * right + offset.y * up);
		reflectColor += GetReflectColor2(uv, position, sampleDir, normal, 6, randOffset);
	}
	reflectColor /= sampleCount;
	
	float reflectRatio = (1-roughness) * (1-ao);
	color = reflectColor * mix(vec3(0.2), albedo, metallic) * reflectRatio + sceneColor;
}