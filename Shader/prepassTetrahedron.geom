#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices=12) out;

uniform mat4 lightViewProjMat[4]; // forward(-z), backward(+z), right(+x), left(-x)

#define constA 0.471404523
#define constB 0.666666687
// in view space
//const vec3 cullingPlanes[4][3] = vec3[][](
//  vec3[](vec3(-constA, constA, -constB), 	vec3(constA, constA, -constB),	vec3(0, 1, 0)	),
//  vec3[](vec3(constA, -constA, -constB), 	vec3(-constA, -constA, -constB),vec3(0, -1, 0)	),
//  vec3[](vec3(constA, -constA, constB), 	vec3(constA, constA, constB),	vec3(1, 0, 0)	),
//  vec3[](vec3(-constA, constA, constB), 	vec3(-constA, -constA, constB),	vec3(-1, 0, 0)	) 
// );
const vec3 cullingPlanes[4][3] = vec3[][](
  vec3[](vec3(-constA, -constB, -constA), 	vec3(constA, -constB, -constA),	vec3(0, 0, -1)	),
  vec3[](vec3(constA, -constB, constA), 	vec3(-constA, -constB, constA),	vec3(0, 0, 1)	),
  vec3[](vec3(constA, constB, constA), 		vec3(constA, constB, -constA),	vec3(1, 0, 0)	),
  vec3[](vec3(-constA, constB, -constA), 	vec3(-constA, constB, constA),	vec3(-1, 0, 0)	) 
 );

void main()
{	
	// for point light, gl_in[i].gl_Position is in point light view space (same as light local space)
	vec3 normal = cross((gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz), (gl_in[2].gl_Position.xyz - gl_in[1].gl_Position.xyz));
	// back-face culling
	if(dot(normal, gl_in[0].gl_Position.xyz) > 0.0)
		return;
		
	for(int face = 0; face < 4; ++face)
	{
		//int face = 0;
		// culling for each side
		float cullingDis[3][3];
		float inside = 1.0;
		for(int planeIdx = 0; planeIdx < 3; ++planeIdx)
		{
			vec3 plane = cullingPlanes[face][planeIdx];
			
			cullingDis[planeIdx][0] = dot(plane, gl_in[0].gl_Position.xyz);
			cullingDis[planeIdx][1] = dot(plane, gl_in[1].gl_Position.xyz);
			cullingDis[planeIdx][2] = dot(plane, gl_in[2].gl_Position.xyz);
			
			// result > 0 if any vert is inside of this plane
			float result = clamp(cullingDis[planeIdx][0], 0.0, 1.0) +
				clamp(cullingDis[planeIdx][1], 0.0, 1.0) +
				clamp(cullingDis[planeIdx][2], 0.0, 1.0);
				
			inside *= result;
		}
		
		// inside == 0 if all 3 verts are outside of any plane
		if(inside > 0)
		{
			for(int i = 0; i < 3; ++i)
			{
				gl_Position = lightViewProjMat[face] * gl_in[i].gl_Position;
				gl_ClipDistance[0] = cullingDis[0][i];
				gl_ClipDistance[1] = cullingDis[1][i];
				gl_ClipDistance[2] = cullingDis[2][i];
				EmitVertex();
			}
			EndPrimitive();
		}
	}
}