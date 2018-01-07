#version 430 core

#include "Include/CommonUBO.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform vec3 depthParams; // x: near, y: far, z: use depth
uniform vec2 texSize;
uniform sampler2D tex;

void main() 
{
	float texAR = texSize.y / texSize.x;
	float screenAR = resolution.y / resolution.x;
	vec2 size;
	if(texAR > screenAR)
	{
		size = vec2(screenAR / texAR, 1);
	}
	else
	{
		size = vec2(1, texAR / screenAR);
	}
	vec2 offset = 0.5 + (1-size) * 0.5;
	
	vec2 uv = fs_in.texCoords;
	uv.y = 1-uv.y;
	
	if(uv.x < offset.x || uv.y < offset.y)
		discard;
		
	uv = (uv - offset) / size * 2;
	uv.y = 1-uv.y;
		
	//color = vec4(uv.x, uv.y, 0.0f, 1.0f);
	if(depthParams.z > 0.5)
	{
		float z = texture(tex, uv).r * 2 - 1;
		float linearDepth = (2 * depthParams.x * depthParams.y) / (depthParams.y + depthParams.x - z * (depthParams.y - depthParams.x));
		linearDepth = linearDepth / (depthParams.y - depthParams.x);
		color = vec4(vec3(linearDepth), 1.0);
	}
	else
	{
		color = vec4(texture(tex, uv).rgb, 1.0);
	}
}