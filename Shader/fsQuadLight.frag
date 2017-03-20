#version 330 core

#define LIGHT_COUNT 4

#include "Include/CommonUBO.incl"
#include "Include/DeferredLighting.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform sampler2D gPositionTex;
uniform sampler2D gNormalTex;
uniform sampler2D gAlbedoTex;
uniform sampler2D gMaterialTex;
uniform sampler2D gDepthStencilTex;

//uniform vec3 viewPos;
uniform Light lights[LIGHT_COUNT];

void main() 
{	
	vec2 uv = fs_in.texCoords;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	vec4 depthStencil = texture(gDepthStencilTex, uv);
	float depth = depthStencil.r;
	//vec3 normal = texture(gNormalTex, fs_in.texCoords).rgb;
	//vec3 rposition = texture(gPositionTex, fs_in.texCoords).rgb;
	vec3 position = GetGBufferPositionVS(depth, projMat, fs_in.positionVS);
	//vec3 view = normalize(viewPos - position);
	vec3 view = normalize(-position);	
	vec3 albedo = texture(gAlbedoTex, uv).rgb;
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	
	vec3 ambient = vec3(0.01f) * albedo;
	vec3 result = ambient;
	for(int i = 0; i < LIGHT_COUNT; ++i)
	{
		result += CalcDirectionalLight(lights[i], normal, position, view, albedo, metallic, roughness);
	}
	//result += CalcPointLight(lights[0], normal, position, view, albedoSpec.rgb, albedoSpec.a);
	color = vec4(result, 1.0f);
	//color = vec4(abs((rposition - position) / rposition) * 100, 1.0f);
	//color = vec4(normal, 1.0f);
	//color = vec4(metallic, roughness, 0.0f, 1.0f);
	//gl_FragDepth = depth;

}