#version 330 core

#include "Include/CommonUBO.incl"

in vec3 position;

uniform mat4 modelMat;

void main()
{	
	gl_Position = projMat * viewMat * modelMat * vec4(position, 1.0f);
}