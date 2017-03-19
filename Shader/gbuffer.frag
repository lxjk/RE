#version 330 core

in VS_OUT
{
	//vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoords;
} fs_in;

//out vec3 gPosition;
out vec3 gNormal;
out vec4 gAlbedoSpec;

uniform sampler2D diffuseTex;
uniform sampler2D normalTex;

void main() 
{
	//gPosition = fs_in.position;
	
	vec3 faceNormal = normalize(fs_in.normal);
	vec3 faceTangent = normalize(fs_in.tangent);
	mat3 TBN = mat3(faceTangent, cross(faceNormal, faceTangent), faceNormal);
	gNormal = (TBN * normalize(texture(normalTex, fs_in.texCoords).rgb * 2.0f - 1.0f)) * 0.5f + 0.5f;
	//gNormal = faceNormal * 0.5f + 0.5f;
	
	gAlbedoSpec.rgb = texture(diffuseTex, fs_in.texCoords).rgb;
	gAlbedoSpec.a = 1.f;
}