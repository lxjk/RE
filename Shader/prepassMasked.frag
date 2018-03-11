#version 430 core

in VS_OUT
{
	vec2 texCoords;
} fs_in;

out vec4 color;

uniform sampler2D maskTex;
uniform vec4 tile;

void main() 
{
	vec2 uv = fs_in.texCoords * tile.xy + tile.zw;		
	if(texture(maskTex, uv).r < 0.5)
		discard;
		
	color = vec4(0.f);
}