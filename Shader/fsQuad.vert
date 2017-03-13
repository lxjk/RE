#version 330 core

in vec3 position;
in vec2 texCoords;

out VS_OUT
{
	vec3 position;
	vec3 viewDir;
	vec2 texCoords;
} vs_out;

layout(std140) uniform RenderMatrices
{
	mat4 viewMat;
	mat4 projMat;
};

void main()
{	
	gl_Position = vec4(position.x, position.y, -1, 1);
	vs_out.texCoords = texCoords;
	// z = -1 plane in view space
	vs_out.viewDir = vec3(position.x / projMat[0][0], position.y / projMat[1][1], -1);
}