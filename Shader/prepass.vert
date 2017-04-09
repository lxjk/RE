#version 330 core

#include "Include/CommonUBO.incl"
#include "Include/CommonVertexInput.incl"

uniform mat4 modelMat;

void main()
{	
	gl_Position = projMat * viewMat * modelMat * vec4(position, 1.0f);
}