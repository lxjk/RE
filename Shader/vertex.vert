#version 330 core

in vec2 inPosition;
in vec3 inNormal;
in vec2 inTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
	gl_Position = vec4(inPosition.x, inPosition.y, 0, 1);
	//gl_position = vec4(inPosition.xy, 0, 1);
}