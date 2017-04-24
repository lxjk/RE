#version 330 core

#include "Include/CommonUBO.incl"
#include "Include/PostProcessPassTex.incl"
#include "Include/DeferredLighting.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform sampler2D historyTex;

vec3 RGBToYCoCg(vec3 c)
{
	// Y = R/4 + G/2 + B/4
	// Co = R/2 - B/2
	// Cg = -R/4 + G/2 - B/4
	return vec3(
		 c.x/4.0 + c.y/2.0 + c.z/4.0,
		 c.x/2.0 - c.z/2.0,
		-c.x/4.0 + c.y/2.0 - c.z/4.0
	);
}

vec4 RGBToYCoCg(vec4 c)
{
	//return c;
	return vec4(RGBToYCoCg(c.xyz), c.w);
}

// https://software.intel.com/en-us/node/503873
vec3 YCoCgToRGB(vec3 c)
{
	// R = Y + Co - Cg
	// G = Y + Cg
	// B = Y - Co - Cg
	return max(vec3(
		c.x + c.y - c.z,
		c.x + c.z,
		c.x - c.y - c.z
	), vec3(0));
}

vec4 YCoCgToRGB(vec4 c)
{
	//return c;
	return vec4(YCoCgToRGB(c.xyz), c.w);
}

vec3 FindClosestFragment_3x3(vec2 uv)
{
	vec2 scale = 1.0 / textureSize(gDepthStencilTex, 0);
	
	vec3 d00 = vec3(-1, -1, texture(gDepthStencilTex, uv + vec2(-1, -1) * scale).r);
	vec3 d10 = vec3( 0, -1, texture(gDepthStencilTex, uv + vec2( 0, -1) * scale).r);
	vec3 d20 = vec3( 1, -1, texture(gDepthStencilTex, uv + vec2( 1, -1) * scale).r);
	vec3 d01 = vec3(-1,  0, texture(gDepthStencilTex, uv + vec2(-1,  0) * scale).r);
	vec3 d11 = vec3( 0,  0, texture(gDepthStencilTex, uv + vec2( 0,  0) * scale).r);
	vec3 d21 = vec3( 1,  0, texture(gDepthStencilTex, uv + vec2( 1,  0) * scale).r);
	vec3 d02 = vec3(-1,  1, texture(gDepthStencilTex, uv + vec2(-1,  1) * scale).r);
	vec3 d12 = vec3( 0,  1, texture(gDepthStencilTex, uv + vec2( 0,  1) * scale).r);
	vec3 d22 = vec3( 1,  1, texture(gDepthStencilTex, uv + vec2( 1,  1) * scale).r);
	
	vec3 dmin = d00;
	if(dmin.z < d10.z) dmin = d10;
	if(dmin.z < d20.z) dmin = d20;
	if(dmin.z < d01.z) dmin = d01;
	if(dmin.z < d11.z) dmin = d11;
	if(dmin.z < d21.z) dmin = d21;
	if(dmin.z < d02.z) dmin = d02;
	if(dmin.z < d12.z) dmin = d12;
	if(dmin.z < d22.z) dmin = d22;
	
	return vec3(uv + dmin.xy * scale, dmin.z);
}

vec4 clip_aabb(vec3 aabb_min, vec3 aabb_max, vec4 source, vec4 target)
{
#define CLIP_CENTER 1

#if CLIP_CENTER
	vec3 center = (aabb_max + aabb_min) * 0.5;
	vec3 extent = (aabb_max - aabb_min) * 0.5;
	
	vec4 sourceToTarget = target - vec4(center, source.w);
	vec3 ratio = abs(sourceToTarget.xyz / extent);
	float maxRatio = max(ratio.x, max(ratio.y, ratio.z));
	
	if(maxRatio > 1)
		return vec4(center, source.w) + sourceToTarget / maxRatio;
	return target; // inside aabb
#else
	vec3 extentPos = min((source.xyz - aabb_min), vec3(0.001));
	vec3 extentNeg = -min((aabb_max - source.xyz), vec3(0.001));
	
	vec4 sourceToTarget = target - source;
	vec3 ratioPos = sourceToTarget.xyz / extentPos;
	vec3 ratioNeg = sourceToTarget.xyz / extentNeg;
	float maxRatio = max(max(ratioPos.x, max(ratioPos.y, ratioPos.z)), 
						max(ratioNeg.x, max(ratioNeg.y, ratioNeg.z))) * 0.5;
	
	if(maxRatio > 1)
		return source + sourceToTarget / maxRatio;
	return target; // inside aabb
#endif

#undef CLIP_CENTER
}

void main() 
{	
	vec2 curUV = fs_in.texCoords;// - projMat[2].xy * 0.5; // un-jitter
	
	vec4 depthStencil = texture(gDepthStencilTex, curUV);
	float depth = depthStencil.r;
	vec3 posVS = GetPositionVSFromDepth(depth, projMat, fs_in.positionVS);
	vec4 prevPosCS = prevViewProjMat * invViewMat * vec4(posVS, 1);
	
	vec2 prevUV = prevPosCS.xy / prevPosCS.w * 0.5 + 0.5;
	vec2 velocity = curUV - prevUV;
	
	vec2 historySize = textureSize(historyTex, 0);
	prevUV = (floor(prevUV * historySize) + 0.5) / historySize; // map to texel center
	
	vec4 curColor = texture(gSceneColorTex, curUV);
	vec4 prevColor = texture(historyTex, prevUV);
		
	//vec4 nc = curColor;
	vec4 nc = RGBToYCoCg(curColor);
	vec4 prev = RGBToYCoCg(prevColor);
	
	// neighborhood check	
	vec2 sceneColorSize = textureSize(gSceneColorTex, 0);
	vec2 veloDiff = abs(velocity) - 0.5 / sceneColorSize;
	if(veloDiff.x > 0 || veloDiff.y > 0 || depth == 1)
	{
	#define NEIGHBOR_TAP4 1
	#if NEIGHBOR_TAP4
		vec2 neighborScale = 0.75 / sceneColorSize;
		vec4 n00 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2(-1, -1) * neighborScale));
		vec4 n10 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 1, -1) * neighborScale));
		vec4 n11 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 1,  1) * neighborScale));
		vec4 n01 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2(-1,  1) * neighborScale));
		
		vec4 n_min = min(min(n00, min(n10, min(n11, n01))), nc);
		vec4 n_max = max(max(n00, max(n10, max(n11, n01))), nc);
	#else
		vec2 neighborScale = 1.5 / sceneColorSize;
		vec4 n00 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2(-1, -1) * neighborScale));
		vec4 n10 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 0, -1) * neighborScale));
		vec4 n20 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 1, -1) * neighborScale));
		vec4 n01 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2(-1,  0) * neighborScale));
		vec4 n21 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 1,  0) * neighborScale));
		vec4 n02 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2(-1,  1) * neighborScale));
		vec4 n12 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 0,  1) * neighborScale));
		vec4 n22 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 1,  1) * neighborScale));
		
		vec4 n_min = min(nc, min(n00, min(n10, min(n20, min(n01, min(n21, min(n02, min(n12, n22))))))));
		vec4 n_max = max(nc, max(n00, max(n10, max(n20, max(n01, max(n21, max(n02, max(n12, n22))))))));
		
		vec4 n_avg = (nc+n00+n10+n20+n01+n21+n02+n12+n22) / 9.0;
	#endif
	#undef NEIGHBOR_TAP4
		
		// shrink chroma min-max
		//float chromaExtent = clamp(length(n_max - n_min) * 5, 0, 1);
		//chromaExtent = clamp((chromaExtent) * 400, 0, 1);
		// float chromaExtent = 0.25 * 0.5 * (n_max.x - n_min.x);
		// n_min.yz = n_avg.yz - chromaExtent;
		// n_max.yz = n_avg.yz + chromaExtent;
		
		//prevColor = clip_aabb(n_min.xyz, n_max.xyz, nc, prevColor);
		prev = clip_aabb(n_min.xyz, n_max.xyz, nc, prev);
		prevColor = YCoCgToRGB(prev);
	}
	
	float curLum = nc.x;
	float prevLum = prev.x;
	float feedbackRate = 1.0 - abs(curLum - prevLum) / max(curLum, max(prevLum, 0.2));
	feedbackRate *= feedbackRate;
	float feedback = mix(0.88, 0.97, feedbackRate);
	
	color = mix(curColor, prevColor, feedback);
}