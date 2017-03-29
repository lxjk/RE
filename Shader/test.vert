#version 330 core

#include "Include/CommonUBO.incl"

in vec3 position;
in vec3 normal;
in vec4 tangent;
in vec2 texCoords;

out VS_OUT
{
	vec3 position;
	vec3 normal;
	vec4 tangent;
	vec2 texCoords;
} vs_out;

uniform mat4 modelMat;
uniform mat3 normalMat;

void main()
{
	
	//gl_Position = vec4(inPosition.x, inPosition.y, 0, 1);
	vec4 worldPos = modelMat * vec4(position, 1.0f);
	gl_Position = projMat * viewMat * worldPos;
	//gl_Position.xy = inPosition.xy * 0.5f;
	//gl_Position.zw = vec2(0, 1);
	vs_out.position = worldPos.xyz;
	vs_out.normal = normalMat * normal;
	vs_out.tangent = vec4(normalMat * tangent.xyz, tangent.w);
	vs_out.texCoords = texCoords;
}