#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform int cubeMapArrayIndex;
uniform mat4 lightViewProjMat[6]; // right(+x), left(-x), top(+y), bottom(-y), backward(+z), forward(-z)

// since all culling plane pass origin, we just need to store plane normal
const float rh = 0.70710678;
// in view space
const vec3 cullingPlanes[6][4] = vec3[][](
  vec3[](vec3(rh, rh, 0), 	vec3(rh, -rh, 0), 	vec3(rh, 0, rh), 	vec3(rh, 0, -rh)	),
  vec3[](vec3(-rh, rh, 0), 	vec3(-rh, -rh, 0),	vec3(-rh, 0, rh), 	vec3(-rh, 0, -rh)	),
  vec3[](vec3(rh, rh, 0), 	vec3(-rh, rh, 0),	vec3(0, rh, rh), 	vec3(0, rh, -rh)	),
  vec3[](vec3(rh, -rh, 0), 	vec3(-rh, -rh, 0),	vec3(0, -rh, rh), 	vec3(0, -rh, -rh)	),
  vec3[](vec3(rh, 0, rh), 	vec3(-rh, 0, rh), 	vec3(0, rh, rh), 	vec3(0, -rh, rh)	),
  vec3[](vec3(rh, 0, -rh), 	vec3(-rh, 0, -rh), 	vec3(0, rh, -rh), 	vec3(0, -rh, -rh)	)
 );

void main()
{	
	// for point light, gl_in[i].gl_Position is in point light view space (same as light local space)
	vec3 normal = cross((gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz), (gl_in[2].gl_Position.xyz - gl_in[1].gl_Position.xyz));
	// back-face culling
	if(dot(normal, gl_in[0].gl_Position.xyz) > 0.0)
		return;
		
	for(int face = 0; face < 6; ++face)
	{
		// culling for each side
		float inside = 1.0;
		for(int planeIdx = 0; planeIdx < 4; ++planeIdx)
		{
			vec3 plane = cullingPlanes[face][planeIdx];
			
			// result > 0 if any vert is inside of this plane
			float result = clamp(dot(plane, gl_in[0].gl_Position.xyz), 0.0, 1.0) +
				clamp(dot(plane, gl_in[1].gl_Position.xyz), 0.0, 1.0) +
				clamp(dot(plane, gl_in[2].gl_Position.xyz), 0.0, 1.0);
				
			inside *= result;
		}
		
		// inside == 0 if all 3 verts are outside of any plane
		if(inside > 0)
		{
			gl_Layer = cubeMapArrayIndex * 6 + face;
			for(int i = 0; i < 3; ++i)
			{
				gl_Position = lightViewProjMat[face] * gl_in[i].gl_Position;
				EmitVertex();
			}
			EndPrimitive();
		}
	}
}