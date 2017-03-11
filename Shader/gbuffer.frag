#version 330 core

in vec3 fragWorldPos;
in vec3 fragWorldNormal;
in vec2 fragTexCoords;

out vec3 gPosition;
out vec3 gNormal;
out vec4 gAlbedoSpec;

void main() 
{
	gPosition = fragWorldPos;
	gNormal = fragWorldNormal;
	gAlbedoSpec.rgb = vec3(1.0f, 1.0f, 0.0f);
	gAlbedoSpec.a = 1.0f;
}