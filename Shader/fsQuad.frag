#version 430 core

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform sampler2D screenTex;

void main() 
{
	//color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	color = vec4(texture(screenTex, fs_in.texCoords).rgb, 1.0f);
}