#version 330 core

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"
#include "Include/DeferredPassTex.incl"

#define MAX_LIGHT_COUNT 4
#define MAX_CSM_COUNT 3

struct ShadowData
{
	mat4 shadowMat;
	float farPlane;
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
uniform sampler2DShadow shadowMap[MAX_LIGHT_COUNT * MAX_CSM_COUNT];

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
	int shadowCount = 0;
	int clampedLightCount = min(lightCount, MAX_LIGHT_COUNT);
	for(int i = 0; i < clampedLightCount; ++i)
	{
		float shadowFactor = 1;
		for(int c = 0; c < lights[i].shadowDataCount; ++c)
		{
			if(-position.z <= shadowData[shadowCount+c].farPlane)
			{
				// convert to light clip space
				vec4 posLCS = shadowData[shadowCount+c].shadowMat * vec4(position, 1.f);
				// bias calculation from http://bryanlawsmithblog.blogspot.com/2014/12/rendering-post-stable-cascaded-shadow.html
				float cosAlpha = dot(normal, -lights[i].directionRAB.xyz);
				float sinAlpha = sqrt(1.0 - cosAlpha * cosAlpha);
				float bias = (0.001 + 0.003 * c) * sinAlpha / cosAlpha;
				//float bias = 0.004 * (1.0 - abs(dot(normal, -lights[i].directionRAB.xyz)));  
				//float bias = 0.008;
				posLCS.z -= posLCS.w * bias;
				//posLCS /= posLCS.w;
				//posLCS = posLCS * 0.5 + 0.5;
				//float posDepth = posLCS.z * 0.5 + 0.5;
				//float shadowMapDepth = texture(shadowMap[shadowCount+c], (posLCS.xy * 0.5 + 0.5)).r;
				//shadowFactor = posDepth;
				//shadowFactor = posDepth < shadowMapDepth ? 0 : 1;
				shadowFactor = textureProj(shadowMap[shadowCount+c], posLCS);
				break;
			}
		}
		shadowCount += lights[i].shadowDataCount;
		result += CalcLight(lights[i], normal, position, view, albedo, metallic, roughness) * shadowFactor;
		//result = vec3(shadowFactor);
	}
	color = vec4(result, 1.0f);
	//color = vec4(albedo, 1.0f);
	//color = vec4(metallic, roughness, 0.0f, 1.0f);
	gl_FragDepth = depth;

}