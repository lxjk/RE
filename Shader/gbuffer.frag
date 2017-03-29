#version 330 core

in VS_OUT
{
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
} fs_in;

out vec3 gNormal;
out vec3 gAlbedo;
out vec2 gMaterial;

uniform sampler2D diffuseTex;
uniform sampler2D normalTex;
uniform float metallic;
uniform float roughness;

void main() 
{	
	vec3 faceNormal = normalize(fs_in.normal);
	vec3 faceTangent = normalize(fs_in.tangent.xyz);
	vec3 faceBitangent = cross(faceNormal, faceTangent) * fs_in.tangent.w;
	//faceTangent = cross(faceBitangent, faceNormal) * fs_in.tangent.w;
	mat3 TBN = mat3(faceTangent, faceBitangent, faceNormal);
	gNormal = (TBN * normalize(texture(normalTex, fs_in.texCoords).rgb * 2.0f - 1.0f)) * 0.5f + 0.5f;
	//gNormal = faceNormal * 0.5f + 0.5f;
	
	gAlbedo = texture(diffuseTex, fs_in.texCoords).rgb;
	//gAlbedo = vec3(fs_in.tangent.w * 0.5 + 0.5, 0.0, 0.0);
	
	gMaterial.r = metallic;
	gMaterial.g = roughness;
}