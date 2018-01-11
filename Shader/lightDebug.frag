#version 430 core

in VS_OUT
{
	vec3 position;
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
} fs_in;

out vec4 outColor;

uniform vec3 color;

void main() 
{
	outColor = vec4(color, 1.f);
}