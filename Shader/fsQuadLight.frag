#version 330 core

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
	vec3 viewDir;
	vec2 texCoords;
} fs_in;

out vec4 color;

layout(std140) uniform RenderMatrices
{
	mat4 viewMat;
	mat4 projMat;
};

uniform sampler2D gPositionTex;
uniform sampler2D gNormalTex;
uniform sampler2D gAlbedoSpecTex;
uniform sampler2D gDepthStencilTex;

uniform float specPower;

//uniform vec3 viewPos;
uniform Light lights[LIGHT_COUNT];

vec3 CalcPointLight(Light lightData, vec3 normal, vec3 pos, vec3 view)
{
	vec3 light = vec3(viewMat * vec4(lightData.position, 1.f)) - pos;
	float dist = length(light);
	float attRatio = min(dist / lightData.radius, 1.f);
	float attenuation = 1.f - attRatio * attRatio;
	//float attenuation = 1.f;
	light /= dist;
	vec3 halfVec = normalize(light + view);
	
	vec4 albedoSpec = texture(gAlbedoSpecTex, fs_in.texCoords);
	
	vec3 ambient = lightData.ambient * attenuation  * albedoSpec.a; // hack! using spec to exclude pixel we don't draw on
	vec3 diff = albedoSpec.rgb * max(dot(normal, light), 0.f) * lightData.diffuse * attenuation;
	vec3 spec = pow(max(dot(normal, halfVec), 0.f), specPower) * lightData.specular * attenuation * albedoSpec.a;
	//spec = vec3(0,0,0);
	
	//return vec3(specV, specV, specV);
	return ambient + diff + spec;
}

vec3 GetPosition(float depth)
{
	float ndcZ = depth * 2.f - 1.f;
	float viewZ = projMat[3][2] / (projMat[2][3] * ndcZ - projMat[2][2]);
	return -fs_in.viewDir * viewZ;
}

void main() 
{	
	vec3 normal = normalize(texture(gNormalTex, fs_in.texCoords).rgb * 2.0f - 1.0f);
	float depth = texture(gDepthStencilTex, fs_in.texCoords).r;
	//vec3 normal = texture(gNormalTex, fs_in.texCoords).rgb;
	vec3 position = texture(gPositionTex, fs_in.texCoords).rgb;
	//vec3 position = GetPosition(depth);
	//vec3 view = normalize(viewPos - position);
	vec3 view = normalize(-position);
	vec3 result = vec3(0,0,0);
	for(int i = 0; i < LIGHT_COUNT; ++i)
	{
		result += CalcPointLight(lights[i], normal, position, view);
	}
	//result += CalcPointLight(lights[0], normal, position, view);
	color = vec4(result, 1.0f);
	//color = vec4((rposition - position), 1.0f);
	//color = vec4(normal, 1.0f);
	gl_FragDepth = depth;
}