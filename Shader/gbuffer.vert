#version 330 core

in vec3 inPosition;
in vec3 inNormal;
in vec2 inTexCoords;

out vec3 fragWorldPos;
out vec3 fragWorldNormal;
out vec2 fragTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec4 worldPos = model * vec4(inPosition, 1.0f);
	outWorldPos = worldPos.xyz;
	gl_Position = projection * view * worldPos;
	outTexCoords = inTexCoords;
	mat3 normalMat = transpose(inverse(mat(model)));
	outWorldNormal = normalMat * inNormal;
}