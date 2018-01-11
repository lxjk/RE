#version 430 core

struct Light 
{
	vec3 position;
	vec3 direction;
	vec3 diffuse;
	vec3 specular;
	vec3 ambient;
	float radius;
};

#define LIGHT_COUNT 4

in VS_OUT
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform float specPower;
uniform sampler2D diffuseTex;
uniform sampler2D normalTex;


uniform vec3 viewPos;
uniform Light lights[LIGHT_COUNT];


vec3 CalcPointLight(Light lightData, vec3 normal, vec3 pos, vec3 view)
{
	vec3 light = lightData.position - pos;
	float dist = length(light);
	float attRatio = min(dist / lightData.radius, 1.f);
	float attenuation = 1.f - attRatio * attRatio;
	//float attenuation = 1.f;
	light /= dist;
	vec3 halfVec = normalize(light + view);
	
	vec3 ambient = lightData.ambient * attenuation;
	vec3 diff = texture(diffuseTex, fs_in.texCoords).rgb * max(dot(normal, light), 0.f) * lightData.diffuse * attenuation;
	vec3 spec = pow(max(dot(normal, halfVec), 0.f), specPower) * lightData.specular * attenuation;
	//spec = vec3(0,0,0);
	
	//return vec3(specV, specV, specV);
	return ambient + diff + spec;
}

void main() 
{
	vec3 faceNormal = normalize(fs_in.normal);
	vec3 faceTangent = normalize(fs_in.tangent);
	mat3 TBN = mat3(faceTangent, cross(faceNormal, faceTangent), faceNormal);
	vec3 normal = TBN * normalize(texture(normalTex, fs_in.texCoords).rgb * 2.0f - 1.0f);
	//vec3 normal = (texture(normalTex, fs_in.texCoords).rgb) * 2.0f - 1.0f);
	vec3 view = normalize(viewPos - fs_in.position);
	vec3 result = vec3(0,0,0);
	for(int i = 0; i < LIGHT_COUNT; ++i)
	{
		result += CalcPointLight(lights[i], normal, fs_in.position, view);
	}
	//result += CalcPointLight(lights[3], normal, fs_in.position, view);
	color = vec4(result, 1.0f);
	//color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	//color = vec4(normal, 1.0f);
	//color = vec4(fs_in.texCoords.x, fs_in.texCoords.y, 0.f, 1.0f);
}