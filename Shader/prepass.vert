#version 330 core

in vec3 position;

layout(std140) uniform RenderInfo
{
	mat4 viewMat;
	mat4 projMat;
	vec2 resolution;
};

uniform mat4 modelMat;

void main()
{	
	gl_Position = projMat * viewMat * modelMat * vec4(position, 1.0f);
}