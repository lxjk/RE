#version 330 core

in VS_OUT
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 texCoords;
} fs_in;

out vec4 outColor;

uniform vec3 color;

void main() 
{
	outColor = vec4(color, 1.f);
}