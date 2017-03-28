#version 330 core

#include "Include/PostProcessPassTex.incl"

in VS_OUT
{
	vec3 positionVS;
	vec2 texCoords;
} fs_in;

out vec4 color;

void main() 
{
	const float invGamma = 1.0 / 2.2;
	const float exposure = 1.0;
	
	vec3 hdrColor = texture(gSceneColorTex, fs_in.texCoords).rgb;
	//vec3 mapped = hdrColor;
	//vec3 mapped = hdrColor / (hdrColor + vec3(1.0f));
	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
	mapped = pow(mapped, vec3(invGamma));
	
	//color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	color = vec4(mapped, 1.0f);
	
	gl_FragDepth = texture(gDepthStencilTex, fs_in.texCoords).r;
}