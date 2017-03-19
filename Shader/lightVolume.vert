#version 330 core

in vec3 position;

out VS_OUT
{
	vec3 positionVS;
} vs_out;

layout(std140) uniform RenderInfo
{
	mat4 viewMat;
	mat4 projMat;
	vec2 resolution;
};

uniform mat4 modelMat;

void main()
{	
	vec4 posVS = viewMat * modelMat * vec4(position, 1.0f);
	gl_Position = projMat * posVS;
	vs_out.positionVS = posVS.xyz;
}