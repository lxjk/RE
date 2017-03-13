
// sdl
#define SDL_MAIN_HANDLED
#include "sdl.h"
#include "SDL_image.h"

// glew
#include "gl\glew.h"

// opengl
#include "SDL_opengl.h"

// glm
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// std
#include <stdlib.h>
#include <stdio.h>
#include <vector>

// local
#include "Engine/Shader.h"
#include "Engine/Mesh.h"
#include "Engine/Texture2D.h"
#include "Engine/Light.h"
#include "Engine/Viewpoint.h"
#include "Engine/Camera.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

const int LIGHT_COUNT = 4;

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

// opengl context
SDL_GLContext gContext;

// ubo
GLuint gUBO_Matrices = 0;

// frame buffer
GLuint gBuffer = 0;
Texture2D gPositionTex, gNormalTex, gAlbedoSpecTex, gDepthStencilTex;

// shader
Shader gTestShader;
Shader gBufferShader;
Shader gLightDebugShader;
Shader gFSQuadShader;

// mesh data
MeshData gCubeMeshData;
MeshData gSphereMeshData;
MeshData gQuadMeshData;

// mesh
Mesh gCubeMesh;
Mesh gSphereMesh;
Mesh gLightDebugMesh;
Mesh gFSQuadMesh;

// texture
Texture2D gDiffuseMap;
Texture2D gNormalMap;

// light
Light gLights[LIGHT_COUNT];

// camera
Camera gCamera;

struct RenderMatrices
{
	glm::mat4 View;
	glm::mat4 Proj;
} gRenderMatrices;

// input
float gMouseWheel;
const Uint8* gKeyStates;

bool init();
bool initGL();

//Shader loading utility programs
void printProgramLog(GLuint program);
void printShaderLog(GLuint shader);

bool init()
{
	// init sdl
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

	// init sdl_image
	if (IMG_Init(IMG_INIT_JPG) == 0)
	{
		printf("SDL_Image could not initialize! SDL_IMAGE_Error: %s\n", IMG_GetError());
		return false;
	}

	// use opengl 3.1 core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// create window with opengl
	gWindow = SDL_CreateWindow("Test",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
	if (gWindow == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

	// create opengl context
	gContext = SDL_GL_CreateContext(gWindow);
	if (gContext == NULL)
	{
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	// init GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
	}

	//Use Vsync
	if (SDL_GL_SetSwapInterval(1) < 0)
	{
		printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	}

	//Initialize OpenGL
	if (!initGL())
	{
		printf("Unable to initialize OpenGL!\n");
		return false;
	}

	// capture mouse
	SDL_CaptureMouse(SDL_TRUE);

	// set key states
	gKeyStates = SDL_GetKeyboardState(NULL);

	return true;
}

void MakeCube()
{
	std::vector<Vertex>& cubeVert = gCubeMeshData.vertices;
	std::vector<GLuint>& cubeIdx = gCubeMeshData.indices;
	glm::vec2 uv[4] =
	{
		glm::vec2(0, 0),
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(1, 0)
	};
	glm::vec3 normal[6] =
	{
		glm::vec3(1, 0, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(-1, 0, 0),
		glm::vec3(0, -1, 0),
		glm::vec3(0, 0, -1),
	};
	glm::vec3 up[6] =
	{
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, -1),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, 1),
		glm::vec3(0, 1, 0),
	};
	cubeVert.reserve(24);
	cubeIdx.reserve(36);
	// +x -> +y -> +z -> -x -> -y -> -z
	for (int i = 0; i < 6; ++i)
	{
		glm::vec3 right = glm::cross(up[i], normal[i]);
		for (int j = 0; j < 4; ++j)
		{
			glm::vec2 ext = uv[j] * 2.f - 1.f;
			cubeVert.push_back(Vertex(normal[i] + ext.x * right - ext.y * up[i], normal[i], right, uv[j]));
		}
		cubeIdx.push_back(i * 4 + 0);
		cubeIdx.push_back(i * 4 + 1);
		cubeIdx.push_back(i * 4 + 2);
		cubeIdx.push_back(i * 4 + 0);
		cubeIdx.push_back(i * 4 + 2);
		cubeIdx.push_back(i * 4 + 3);
	}
}

void MakeSphere()
{
	std::vector<Vertex>& sphereVert = gSphereMeshData.vertices;
	std::vector<GLuint>& sphereIdx = gSphereMeshData.indices;
	
	int div = 32;
	int latDiv = div / 2 + 1;

	sphereVert.reserve(2 + (div+1) * (latDiv - 2));
	sphereIdx.reserve(div * ((latDiv - 2) * 2 + 2) * 3);
	
	// add first vert
	sphereVert.push_back(Vertex(glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec2(0, 0)));
	int prevRingStart = 0;
	int curRingStart = 1;
	// add rings
	for (int iLat = 1; iLat < latDiv - 1; ++iLat)
	{
		// (-1, 1)
		float latRatio = (float)iLat / (float)(latDiv - 1);
		float latAngle = (1 - latRatio * 2) * 0.5f * glm::pi<float>();
		float cosLat = glm::cos(latAngle);
		float sinLat = glm::sin(latAngle);
		// add vert on a ring
		for (int iLon = 0; iLon < div+1; ++iLon)
		{
			float lonRatio = (float)iLon / (float)div;
			float lonAngle = lonRatio * 2 * glm::pi<float>();
			float cosLon = glm::cos(lonAngle);
			float sinLon = glm::sin(lonAngle);
			glm::vec3 pos(cosLon * cosLat, sinLat, sinLon * cosLat);
			glm::vec3 tangent(sinLon, 0, -cosLon);
			sphereVert.push_back(Vertex(pos, pos, tangent, glm::vec2(lonRatio, latRatio)));
		}
		// add idx on a ring with prev ring
		for (int iLon = 0; iLon < div; ++iLon)
		{
			int next = iLon+1;
			if (iLat == 1)
			{
				// first ring
				sphereIdx.push_back(prevRingStart);
				sphereIdx.push_back(curRingStart + next);
				sphereIdx.push_back(curRingStart + iLon);			
			}
			else
			{
				sphereIdx.push_back(prevRingStart + iLon);
				sphereIdx.push_back(curRingStart + next);
				sphereIdx.push_back(curRingStart + iLon);
				sphereIdx.push_back(prevRingStart + iLon);
				sphereIdx.push_back(prevRingStart + next);
				sphereIdx.push_back(curRingStart + next);
			}
		}
		prevRingStart = curRingStart;
		curRingStart += (div+1);
	}
	// add last vert
	sphereVert.push_back(Vertex(glm::vec3(0, -1, 0), glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec2(1, 1)));
	// add idx for last ring
	for (int iLon = 0; iLon < div; ++iLon)
	{
		int next = (iLon + 1);
		sphereIdx.push_back(prevRingStart + iLon);
		sphereIdx.push_back(prevRingStart + next);
		sphereIdx.push_back(curRingStart);
	}

	assert(sphereVert.size() == (2 + (div+1) * (latDiv - 2)));
	assert(sphereIdx.size() == (div * ((latDiv - 3) * 2 + 2)) * 3);
}


void MakeQuad()
{
	std::vector<Vertex>& quadVert = gQuadMeshData.vertices;
	std::vector<GLuint>& quadIdx = gQuadMeshData.indices;

	quadVert.reserve(4);
	quadIdx.reserve(6);

	quadVert.push_back(Vertex(glm::vec3(-1, 1, 0), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0), glm::vec2(0, 1)));
	quadVert.push_back(Vertex(glm::vec3(-1, -1, 0), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0), glm::vec2(0, 0)));
	quadVert.push_back(Vertex(glm::vec3(1, -1, 0), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0), glm::vec2(1, 0)));
	quadVert.push_back(Vertex(glm::vec3(1, 1, 0), glm::vec3(0, 0, -1), glm::vec3(1, 0, 0), glm::vec2(1, 1)));

	quadIdx.push_back(0);
	quadIdx.push_back(1);
	quadIdx.push_back(2);
	quadIdx.push_back(0);
	quadIdx.push_back(2);
	quadIdx.push_back(3);
}

void MakeLights()
{
	memset(gLights, 0, sizeof(Light) * LIGHT_COUNT);

	gLights[0].position = glm::vec3(0, 10, 10);
	gLights[0].ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	gLights[0].diffuse = glm::vec3(1.f, 1.f, 1.f);
	gLights[0].specular = glm::vec3(1.f, 1.f, 1.f);
	gLights[0].radius = 20.f;


	gLights[1].position = glm::vec3(0, 3, 3);
	gLights[1].ambient = glm::vec3(0.1f, 0.1f, 0.f);
	gLights[1].diffuse = glm::vec3(1.f, 1.f, 0.f);
	gLights[1].specular = glm::vec3(1.f, 1.f, 0.f);
	gLights[1].radius = 10.f;


	gLights[2].position = glm::vec3(10, 4, 0);
	gLights[2].ambient = glm::vec3(0.f, 0.1f, 0.f);
	gLights[2].diffuse = glm::vec3(0.f, 1.f, 0.f);
	gLights[2].specular = glm::vec3(0.f, 1.f, 0.f);
	gLights[2].radius = 10.f;	
}

void AllocateFrameBufferTexture(GLuint& textureID, int width, int height, GLint internalFormat, GLenum format, GLenum type)
{
	if (textureID)
		glDeleteTextures(1, &textureID);
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void SetupFrameBuffers()
{
	// G-Buffer
	// position(RGB)
	// normal(RGB)
	// color(RGB) + spec(A)
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	// position
	gPositionTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
	gPositionTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT0);
	// normal
	//gNormalTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
	gNormalTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
	gNormalTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT1);
	// albedo + spec
	gAlbedoSpecTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	gAlbedoSpecTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT2);
	// depth
	gDepthStencilTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
	gDepthStencilTex.AttachToFrameBuffer(GL_DEPTH_STENCIL_ATTACHMENT);

	GLuint attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(_countof(attachments), attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Error: Frame buffer not complete!\n");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool initGL()
{
	// frame buffers
	SetupFrameBuffers();

	// ubo
	glGenBuffers(1, &gUBO_Matrices);
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderMatrices), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	// binding point
	glBindBufferBase(GL_UNIFORM_BUFFER, Shader::renderMatricesBP, gUBO_Matrices);

	// shader
	gTestShader.Load("Shader\\test.vert", "Shader\\test.frag");
	gBufferShader.Load("Shader\\gbuffer.vert", "Shader\\gbuffer.frag");
	gLightDebugShader.Load("Shader\\test.vert", "Shader\\lightDebug.frag");
	gFSQuadShader.Load("Shader\\fsQuad.vert", "Shader\\fsQuadLight.frag");

	// mesh
	MakeCube();
	MakeSphere();
	MakeQuad();

	// model
	gCubeMesh.Init(&gCubeMeshData, &gBufferShader);
	gSphereMesh.Init(&gSphereMeshData, &gBufferShader);
	gLightDebugMesh.Init(&gSphereMeshData, &gLightDebugShader);
	gFSQuadMesh.Init(&gQuadMeshData, &gFSQuadShader);

	// texture
	gDiffuseMap.Load("Content\\Texture\\154.jpg");
	gNormalMap.Load("Content\\Texture\\154_norm.jpg");

	// light
	MakeLights();
	
	// camera
	gCamera.fov = glm::radians(90.f);
	gCamera.position = glm::vec3(0.f, 0.f, 10.f);
	gCamera.euler = glm::vec3(0.f, 0.f, 0.f);

	return true;
}

void close()
{
	SDL_GL_DeleteContext(gContext);

	//Destroy window
	SDL_DestroyWindow(gWindow);

	//Quit SDL_Image
	IMG_Quit();

	//Quit SDL subsystems
	SDL_Quit();
}

void updateMouseInput()
{
	int x = 0, y = 0;
	Uint32 mouseState = SDL_GetRelativeMouseState(&x, &y);

	float deltaX = (float)x;
	float deltaY = (float)y;

	const static float rotSpeed = 0.5f;
	const static float moveSpeed = 0.2f;
	const static float scrollSpeed = 2.f;
	
	if (mouseState & (SDL_BUTTON(SDL_BUTTON_LEFT) | SDL_BUTTON(SDL_BUTTON_RIGHT)))
	{
		gCamera.euler.y -= rotSpeed * deltaX;
		gCamera.euler.x -= rotSpeed * deltaY;
		gCamera.euler.x = glm::clamp(gCamera.euler.x, -89.f, 89.f);
	}

	glm::quat cameraRot(glm::radians(gCamera.euler));
	glm::vec3 cameraForward = cameraRot * glm::vec3(0, 0, -1);
	glm::vec3 cameraUp = cameraRot * glm::vec3(0, 1, 0);
	glm::vec3 cameraRight = cameraRot * glm::vec3(1, 0, 0);

	if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
	{
		gCamera.position -= cameraRight * deltaX * moveSpeed;
		gCamera.position += cameraUp * deltaY * moveSpeed;
	}
	// mouse wheel?
	if (gMouseWheel != 0)
	{
		gCamera.position += cameraForward * gMouseWheel * scrollSpeed;
		gMouseWheel = 0;
	}
}

void updateKeyboardInput()
{
	int x = 0, y = 0;
	Uint32 mouseState = SDL_GetRelativeMouseState(&x, &y);

	glm::quat cameraRot(glm::radians(gCamera.euler));
	glm::vec3 cameraForward = cameraRot * glm::vec3(0, 0, -1);
	glm::vec3 cameraRight = cameraRot * glm::vec3(1, 0, 0);

	glm::vec3 up = glm::vec3(0, 1, 0);

	const static float moveSpeed = 0.2f;

	if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
	{
		if (gKeyStates[SDL_SCANCODE_W])
		{
			gCamera.position += cameraForward * moveSpeed;
		}
		if (gKeyStates[SDL_SCANCODE_S])
		{
			gCamera.position -= cameraForward * moveSpeed;
		}
		if (gKeyStates[SDL_SCANCODE_D])
		{
			gCamera.position += cameraRight * moveSpeed;
		}
		if (gKeyStates[SDL_SCANCODE_A])
		{
			gCamera.position -= cameraRight * moveSpeed;
		}
		if (gKeyStates[SDL_SCANCODE_E])
		{
			gCamera.position += up * moveSpeed;
		}
		if (gKeyStates[SDL_SCANCODE_Q])
		{
			gCamera.position -= up * moveSpeed;
		}
	}

}

void update()
{
	updateMouseInput();
	updateKeyboardInput();
}

void GeometryPass(const Viewpoint& mainViewpoint)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// bind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	// clear frame buffer
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// bind texture
	gDiffuseMap.Bind(Shader::diffuseTexUnit);
	gNormalMap.Bind(Shader::normalTexUnit);

	// draw models
	gBufferShader.Use();
	
	for (int i = 0; i < 3; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(-10 + i * 10, 0, 0));
		modelMat = glm::rotate(modelMat, 45.f, glm::vec3(0, 1, 0));
		modelMat = glm::scale(modelMat, glm::vec3(1.f, 1.2f, 1.5f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gBufferShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gBufferShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		gCubeMesh.Draw();
	}

	for (int i = 0; i < 3; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(-10 + i * 10, 0, 5));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gBufferShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gBufferShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		gSphereMesh.Draw();
	}

	// unbind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightPass(const Viewpoint& mainViewpoint)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glEnable(GL_CULL_FACE);

	// clear default buffer
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// bind textures
	gPositionTex.Bind(Shader::gPositionTexUnit);
	gNormalTex.Bind(Shader::gNormalTexUnit);
	gAlbedoSpecTex.Bind(Shader::gAlbedoSpecTexUnit);
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);

	gFSQuadShader.Use();

	//glUniform3fv(gFSQuadShader.GetUniformLocation("viewPos"), 1, glm::value_ptr(mainViewpoint.position));
	glUniform1f(gFSQuadShader.GetUniformLocation("specPower"), 32.f);

	// set light
	for (int i = 0; i < LIGHT_COUNT; ++i)
	{
		glUniform3fv(gFSQuadShader.GetUniformLocation("lights", i, "position"), 1, glm::value_ptr(gLights[i].position));
		glUniform3fv(gFSQuadShader.GetUniformLocation("lights", i, "ambient"), 1, glm::value_ptr(gLights[i].ambient));
		glUniform3fv(gFSQuadShader.GetUniformLocation("lights", i, "diffuse"), 1, glm::value_ptr(gLights[i].diffuse));
		glUniform3fv(gFSQuadShader.GetUniformLocation("lights", i, "specular"), 1, glm::value_ptr(gLights[i].specular));
		glUniform1f(gFSQuadShader.GetUniformLocation("lights", i, "radius"), gLights[i].radius);
	}
	
	// draw quad
	gFSQuadMesh.Draw();
}

void render()
{
	const Viewpoint& mainViewpoint = gCamera.ProcessCamera((GLfloat)SCREEN_WIDTH, (GLfloat)SCREEN_HEIGHT, 0.1f, 1000.f);

	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//glClearColor(0, 0, 0, 1);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// update ubo
	gRenderMatrices.View = mainViewpoint.viewMat;
	gRenderMatrices.Proj = mainViewpoint.projMat;
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	// maybe use glBufferSubData later?
	glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderMatrices), &gRenderMatrices, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	GeometryPass(mainViewpoint);
	
	LightPass(mainViewpoint);

	// why not working ??
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	// draw debug
	gLightDebugShader.Use();

	for (int i = 0; i < LIGHT_COUNT; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, gLights[i].position);
		modelMat = glm::scale(modelMat, glm::vec3(0.3f, 0.3f, 0.3f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gLightDebugShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gLightDebugShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));
		glUniform3fv(gLightDebugShader.GetUniformLocation("color"), 1, glm::value_ptr(gLights[i].diffuse));

		gLightDebugMesh.Draw();
	}

}

int main(int argc, char **argv)
{
	if (!init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Main loop flag
		bool quit = false;

		SDL_Event event;

		while (!quit) {
			int shouldQuit = 0;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_QUIT) {
					quit = true;
					break;
				}
				/* handle your event here */
				else if (event.type == SDL_MOUSEWHEEL)
				{
					gMouseWheel = (float)event.wheel.y;
				}
			}

			update();

			render();

			SDL_GL_SwapWindow(gWindow);
		}
	}

	close();

	return EXIT_SUCCESS;
}
