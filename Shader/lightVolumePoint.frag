#version 330 core

struct Light 
{
	vec3 position;
	vec3 direction;
	vec3 color;
	float radius;
};

in VS_OUT
{
	vec3 positionVS;
} fs_in;

out vec4 color;

layout(std140) uniform RenderInfo
{
	mat4 viewMat;
	mat4 projMat;
	vec2 resolution;
};

uniform sampler2D gPositionTex;
uniform sampler2D gNormalTex;
uniform sampler2D gAlbedoTex;
uniform sampler2D gMaterialTex;
uniform sampler2D gDepthStencilTex;

uniform float specPower;

uniform Light light;

const float PI = 3.14159265359;


float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a*a;
	float NdotH = max(dot(N, H), 0.f);
	float dnom = (NdotH * NdotH) * (a2 - 1) + 1;
	return a2 / (PI * dnom * dnom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float k = (roughness + 1) * (roughness + 1) / 8.f;
	return NdotV / (NdotV * (1-k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float ggx1 = GeometrySchlickGGX(max(dot(N, L), 0.f), roughness);
	float ggx2 = GeometrySchlickGGX(max(dot(N, V), 0.f), roughness);
	return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 CalcLightPBR(vec3 L, vec3 N, vec3 V, vec3 lightColor, vec3 albedo, float metallic, float roughness, float attenuation)
{
	vec3 H = normalize(L + V);
	
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo, metallic);
	
	vec3 radiance = lightColor * attenuation;
	
	// cook-torrance BRDF
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = FresnelSchlick(max(dot(H, V), 0.f), F0);
	
	vec3 kS = F;
	vec3 kD = vec3(1.0f) - kS;
	kD *= (1.0f - metallic);
	
	vec3 brdf = (NDF * G * F) / (4 * max(dot(N, V), 0.f) * max(dot(N, L), 0.f) + 0.001f);
	
	return (kD * albedo / PI + brdf) * radiance * max(dot(N, L), 0.f);
}

vec3 CalcLight(vec3 light, vec3 normal, vec3 view, vec3 lightColor, vec3 albedo, float attenuation)
{
	vec3 halfVec = normalize(light + view);
	
	vec3 diff = albedo * max(dot(normal, light), 0.f) * lightColor * attenuation;
	vec3 spec = pow(max(dot(normal, halfVec), 0.f), specPower) * lightColor * attenuation;
	//spec = vec3(0,0,0);
	
	//return vec3(specV, specV, specV);
	return diff + spec;
}

vec3 CalcPointLight(Light lightData, vec3 normal, vec3 pos, vec3 view, vec3 albedo, float metallic, float roughness)
{
	vec3 light = vec3(viewMat * vec4(lightData.position, 1.f)) - pos;
	float dist = length(light);
	float attRatio = min(dist / lightData.radius, 1.f);
	//float attenuation = 1.f - attRatio;
	//float attenuation = (1.f - attRatio) * (1.f - attRatio);
	// based on unreal
	float attenuation = (1 - attRatio * attRatio * attRatio * attRatio);
	attenuation = (attenuation * attenuation) / (dist * dist + 1);
	//float attenuation = 1.f / (dist * dist);
	//float attenuation = 1.f;
	light /= dist;

	//return CalcLight(light, normal, view, lightData.color, albedo, attenuation);
	return CalcLightPBR(light, normal, view, lightData.color, albedo, metallic, roughness, attenuation);
}

vec3 GetPosition(float depth)
{
	float ndcZ = depth * 2.f - 1.f;
	float viewZ = projMat[3][2] / (projMat[2][3] * ndcZ - projMat[2][2]);
	return fs_in.positionVS / fs_in.positionVS.z * viewZ;
	
	// ref:
	// https://www.khronos.org/opengl/wiki/Compute_eye_space_from_window_space
	// https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
	// https://mynameismjp.wordpress.com/2010/03/22/attack-of-the-depth-buffer/
}

void main() 
{	
	vec2 uv = gl_FragCoord.xy / resolution;
	vec3 normal = normalize(texture(gNormalTex, uv).rgb * 2.0f - 1.0f);
	float depth = texture(gDepthStencilTex, uv).r;
	vec3 position = GetPosition(depth);
	vec3 view = normalize(-position);	
	vec3 albedo = texture(gAlbedoTex, uv).rgb;
	vec4 material = texture(gMaterialTex, uv);
	float metallic = material.r;
	float roughness = material.g;
	
	vec3 result = CalcPointLight(light, normal, position, view, albedo, metallic, roughness);
	color = vec4(result, 1.0f);
}