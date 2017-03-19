#version 330 core

in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec2 texCoords;

out VS_OUT
{
	//vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoords;
} vs_out;

layout(std140) uniform RenderInfo
{
	mat4 viewMat;
	mat4 projMat;
	vec2 resolution;
};

uniform mat4 modelMat;
uniform mat3 normalMat;

void main()
{
	// output everything in view space
	
	vec4 posVS = viewMat * modelMat * vec4(position, 1.0f);
	gl_Position = projMat * posVS;
	
	//vs_out.position = posVS.xyz;
	
	// we can do this because mat3(viewMat) is guaranteed to be orthogonal (no scale)
	mat3 viewNormalMat = mat3(viewMat) * normalMat;	
	vs_out.normal = viewNormalMat * normal;
	vs_out.tangent = viewNormalMat * tangent;
	
	vs_out.texCoords = texCoords;
}