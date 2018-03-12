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
} fs_in;


out vec4 outColor;

uniform samplerCube skyTex;

void main() 
{	
	vec2 uv;
	vec4 color;
	vec3 normal;
	vec4 material;
	
	GetBasicValue(fs_in.texCoords, fs_in.normal, fs_in.tangent, uv, color, normal, material);
	vec3 albedo = color.rgb;
	float alpha = color.a;
	
	// recover normal
	normal = normal * 2.f - 1.f;
	
	vec3 position = fs_in.posVS;
	vec3 view = normalize(-position);
	float metallic = material.r;
	float roughness = material.g;
	
	vec3 result = vec3(0);
	
	// global lights
	CalcGlobalLights(result, position, normal, view, albedo, metallic, roughness, 0);	
	
	// local lights	
	CalcTiledLocalLights(result, gl_FragCoord.xy, position, normal, view, albedo, metallic, roughness, 0);
	
	// reflection
	vec3 dir = normalize(reflect(position, normal));
	vec4 reflectColor = texture(skyTex, dir);
	
	vec3 brdf = CalcReflectBRDF(dir, normal, albedo, metallic, roughness);
	
	const float roughnessCutoff = 0.8;
	float reflectRatio = (1-roughness);
	result += reflectColor.rgb * brdf * clamp((roughnessCutoff - roughness) / 0.2, 0.0, 1.0);
	
	outColor = vec4(result, alpha);
}