#version 330 core

#include "Include/CommonUBO.incl"
#include "Include/CommonVertexInput.incl"

uniform vec4 clippingValue; // -x, +x, -y, +y
uniform mat4 modelMat;

void main()
{	
	gl_Position = viewProjMat * modelMat * vec4(position, 1.0f);
	vec4 clipping = clippingValue * gl_Position.w;
	gl_ClipDistance[0] = gl_Position.x - clipping.x;
	gl_ClipDistance[1] = clipping.y - gl_Position.x;
	gl_ClipDistance[2] = gl_Position.y - clipping.z;
	gl_ClipDistance[3] = clipping.w - gl_Position.y;
}