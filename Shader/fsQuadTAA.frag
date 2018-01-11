#version 430 core

#include "Include/Common.incl"
#include "Include/CommonUBO.incl"
#include "Include/PostProcessPassTex.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform sampler2D historyColorTex;
//uniform sampler2D historyDepthStencilTex;
uniform sampler2D velocityTex;
uniform float plusWeights[5];


float HDRWeightY(float inColor, float inExposure)
{
	return 1.0 / (inColor * inExposure + 1.0);
}

float HDRWeightInvY(float inColor, float inExposure)
{
	return 1.0 / (inColor * -inExposure + 1.0);
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

// https://github.com/playdeadgames/temporal
vec4 clip_aabb(vec3 aabb_min, vec3 aabb_max, vec4 source, vec4 target)
{
#define CLIP_CENTER 0

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

vec4 ProcessAA(vec2 curUV, vec2 prevUV, float depth, float depthDiff)
{
	vec2 velocity = curUV - prevUV;
	
	vec2 historySize = textureSize(historyColorTex, 0);
	prevUV = (floor(prevUV * historySize) + 0.5) / historySize; // map to texel center
	
	vec4 curColor = texture(gSceneColorTex, curUV);
	vec4 prevColor = texture(historyColorTex, prevUV);
		
	//vec4 nc = curColor;
	vec4 cur = RGBToYCoCg(curColor);
	vec4 prev = RGBToYCoCg(prevColor);
	
	// neighborhood check	
	vec2 sceneColorSize = textureSize(gSceneColorTex, 0);
	vec2 veloDiff = abs(velocity) - 0.5 / sceneColorSize;
	//if(veloDiff.x > 0 || veloDiff.y > 0 || depth == 1)
	{
		#define NEIGHBOR_TAP4 1
		#if NEIGHBOR_TAP4
			vec2 neighborScale = 1 / sceneColorSize;
			vec4 n00 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2(-1, -1) * neighborScale));
			vec4 n10 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 1, -1) * neighborScale));
			vec4 n11 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 1,  1) * neighborScale));
			vec4 n01 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2(-1,  1) * neighborScale));
			
			vec4 n_min = min(min(n00, min(n10, min(n11, n01))), cur);
			vec4 n_max = max(max(n00, max(n10, max(n11, n01))), cur);
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
			
			vec4 n_min = min(cur, min(n00, min(n10, min(n20, min(n01, min(n21, min(n02, min(n12, n22))))))));
			vec4 n_max = max(cur, max(n00, max(n10, max(n20, max(n01, max(n21, max(n02, max(n12, n22))))))));
			
			vec4 n_avg = (cur+n00+n10+n20+n01+n21+n02+n12+n22) / 9.0;
			
		#endif
		#undef NEIGHBOR_TAP4
			
		// shrink chroma min-max
		//float chromaExtent = clamp(length(n_max - n_min) * 5, 0, 1);
		//chromaExtent = clamp((chromaExtent) * 400, 0, 1);
		// float chromaExtent = 0.25 * 0.5 * (n_max.x - n_min.x);
		// n_min.yz = n_avg.yz - chromaExtent;
		// n_max.yz = n_avg.yz + chromaExtent;
		
		//prevColor = clip_aabb(n_min.xyz, n_max.xyz, cur, prevColor);
		prev = clip_aabb(n_min.xyz, n_max.xyz, cur, prev);
		prevColor = YCoCgToRGB(prev);
	}
	
	float curLum = cur.x;
	float prevLum = prev.x;
	float feedbackRate = 1.0 - abs(curLum - prevLum) / max(curLum, max(prevLum, 0.2));
	feedbackRate *= feedbackRate;
	float feedback = mix(0.88, 0.97, feedbackRate);
	
	return mix(curColor, prevColor, feedback);
	//return max(YCoCgToRGB(mix(cur, prev, feedback)), 0.0);
}

vec4 ProcessAA2(vec2 curUV, vec2 prevUV, vec2 velocity, float depth, float depthDiff)
{
#define AA_TONE 0
#define AA_FILTER 0
	bool bOffScreen = max(abs(prevUV.x - 0.5), abs(prevUV.y - 0.5)) >= 0.5;
	
	//vec2 historySize = textureSize(historyColorTex, 0);
	//prevUV = (floor(prevUV * historySize) + 0.5) / historySize; // map to texel center
			
	vec4 cur = RGBToYCoCg(texture(gSceneColorTex, curUV));
	vec4 prev = RGBToYCoCg(texture(historyColorTex, prevUV));
	vec4 filtered = cur;
	vec4 history = prev;
	
	const float currentFrameWeight = 0.08;
	float blendFinal = currentFrameWeight;
		
	vec2 sceneColorSize = textureSize(gSceneColorTex, 0);
	
	vec2 velocityPixel = velocity * sceneColorSize;
	float historyBlurAmp = 2;
	float historyBlur = clamp(abs(velocityPixel.x) * historyBlurAmp + abs(velocityPixel.y) * historyBlurAmp, 0.0, 1.0);
	vec2 veloDiff = abs(velocityPixel) - 0.1;
	
	if(bOffScreen)
	{
		blendFinal = 1;
	}
	
	bool bDoNeighborCheck = !bOffScreen;
	
	//bDoNeighborCheck = bDoNeighborCheck && (abs(depthDiff) > 0.1);
	//bDoNeighborCheck = bDoNeighborCheck && (veloDiff.x > 0 || veloDiff.y > 0);
	//bDoNeighborCheck = false;
	// neighborhood check	
	if(bDoNeighborCheck || depth == 1)
	{
		vec2 neighborScale = 1 / sceneColorSize;
		vec4 n00 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2(-1,  0) * neighborScale));
		vec4 n10 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 1,  0) * neighborScale));
		vec4 n11 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 0, -1) * neighborScale));
		vec4 n01 = RGBToYCoCg(texture(gSceneColorTex, curUV + vec2( 0,  1) * neighborScale));
		
	#if AA_TONE
		float luma_m1 = 0, luma_m2 = 0;
		luma_m1 += cur.x; luma_m2 += cur.x * cur.x;
		luma_m1 += n00.x; luma_m2 += n00.x * n00.x;
		luma_m1 += n10.x; luma_m2 += n10.x * n10.x;
		luma_m1 += n11.x; luma_m2 += n11.x * n11.x;
		luma_m1 += n01.x; luma_m2 += n01.x * n01.x;
		
		float lumaAvg = luma_m1 / 5.0;
		float lumaVar = abs(luma_m2 / 5.0 - lumaAvg * lumaAvg);
		
		float curWeight = HDRWeightY(cur.x, exposure);
		float n00Weight = HDRWeightY(n00.x, exposure);
		float n10Weight = HDRWeightY(n10.x, exposure);
		float n11Weight = HDRWeightY(n11.x, exposure);
		float n01Weight = HDRWeightY(n01.x, exposure);
		cur.xyz *= curWeight;
		n00.xyz *= n00Weight;
		n10.xyz *= n10Weight;
		n11.xyz *= n11Weight;
		n01.xyz *= n01Weight;
	#else
		float lumaVar = 0;
	#endif	
	
		vec4 n_min = min(min(n00, min(n10, min(n11, n01))), cur);
		vec4 n_max = max(max(n00, max(n10, max(n11, n01))), cur);
		
		float luma_min = n_min.x;
		float luma_max = n_max.x;
		float luma_prev = prev.x;
		float luma_contrast = luma_max - luma_min;
		
		
	#if AA_FILTER
		filtered = cur * plusWeights[0]
				+ n00 * plusWeights[1]
				+ n10 * plusWeights[2]
				+ n11 * plusWeights[3]
				+ n01 * plusWeights[4];
				
		if(lumaVar > 1)
			filtered = cur;
	#else
		filtered = cur;
	#endif
		
	#if AA_TONE
		float prevWeight = HDRWeightY(prev.x, exposure);
		prev.xyz *= prevWeight;
	#endif
		
		float motionFactor = historyBlur;// * clamp((abs(depthDiff) - 0) / 0.002, 0, 1);
	#if 0
		float distToClamp = min(abs(luma_min - luma_prev), abs(luma_max - luma_prev));
		float historyAmount = currentFrameWeight * 3.125f + historyBlur / 8.0;
		float historyFactor = distToClamp * historyAmount * (1.0 + historyBlur * historyAmount * 8.0);
		//float historyFactor = currentFrameWeight * 3.125f * distToClamp;
		blendFinal = clamp(historyFactor / (distToClamp + luma_max - luma_min), 0.0, 1.0);
	#else	
		float distToClamp = max(max(luma_prev - luma_max, luma_min - luma_prev), 0);
		float lumaFactor = (distToClamp + 1) / (luma_contrast + 1);
		blendFinal = clamp(currentFrameWeight * lumaFactor * (1 + motionFactor * 4), 0.0, 1.0);
	#endif
		
	#if AA_TONE
		//blendFinal = currentFrameWeight;
	#endif
			
		//history = clip_aabb(n_min.xyz, n_max.xyz, filtered, prev);
		history = vec4(clamp(prev.xyz, n_min.xyz, n_max.xyz), prev.w);
		
		history = mix(prev, history, motionFactor);
		
	#if AA_FILTER
		float addAliasing = clamp(historyBlur, 0.0, 1.0) * 0.5;
		float lumaConstrastFactor = 32.0 * 4.0;
		addAliasing = clamp(addAliasing + 1.0 / (1.0 + luma_contrast * lumaConstrastFactor), 0.0, 1.0);
		filtered.xyz = mix(filtered.xyz, cur.xyz, addAliasing);
	#endif
	}
	
	vec4 result = mix(history, filtered, blendFinal);
	
	#if AA_TONE
	if(bDoNeighborCheck)
		result.xyz *= HDRWeightInvY(result.x, exposure);
	#endif
	
#undef AA_TONE
#undef AA_FILTER

	//return max(vec3(abs(depthDiff)), 0);
	return max(YCoCgToRGB(result), 0.0);
}

void main() 
{	
	vec2 curUV = fs_in.texCoords;// + projMat[2].xy * 0.5; // un-jitter
	vec2 prevUV;
	
	vec4 depthStencil = texture(gDepthStencilTex, curUV);
	float depth = depthStencil.r;
	vec3 posVS = GetPositionVSFromDepth(depth, projMat, fs_in.positionVS);
	vec4 prevPosVS = prevViewMat * invViewMat * vec4(posVS, 1);
	
	vec2 velocity = texture(velocityTex, curUV).rg;
	if(velocity.x > 0)
	{
		// need to convert from [-2, 2] -> [-1, 1]
		velocity = DecodeVelocityFromTexture(velocity) * 0.5;
		prevUV = curUV - velocity;
	}
	else
	{
		vec4 prevPosCS = prevProjMat * prevPosVS;
		vec3 prevPosNS = prevPosCS.xyz / prevPosCS.w * 0.5 + 0.5;
		
		prevUV = prevPosNS.xy - prevProjMat[2].xy * 0.5;
		velocity = curUV - prevUV; // calcualte velocity;
		prevUV = curUV; // don't change uv
	}
	
	//float prevDepth = texture(historyDepthStencilTex, prevUV).r;
	//vec3 prevPosVSFromDepth = GetPositionVSFromDepth(prevDepth, prevProjMat, prevPosVS.xyz);
	
	color = ProcessAA2(curUV, prevUV, velocity, depth, 0);
	//color = ProcessAA2(curUV, prevUV, velocity, depth, prevPosVSFromDepth.z - prevPosVS.z);
	//color = ProcessAA2(curUV, prevUV, depth, 0);
	//color = ProcessAA(curUV, prevUV, depth, 0);
	
	//color = vec4(abs(velocity.x), abs(velocity.y), 0, 1);
}