
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
#include "Engine/Math.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

const int DIRECTIONAL_LIGHT_COUNT = 4;

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

// opengl context
SDL_GLContext gContext;

// ubo
GLuint gUBO_Matrices = 0;

// frame buffer
GLuint gGBuffer = 0;
Texture2D gNormalTex, gAlbedoTex, gMaterialTex, gDepthStencilTex;

GLuint gSceneBuffer = 0;
Texture2D gSceneColorTex;

// shader
Shader gTestShader;
Shader gGBufferShader;
Shader gPrepassShader;
Shader gDirectionalLightShader;
Shader gPointLightShader;
Shader gLightDebugShader;
Shader gFSQuadShader;
Shader gToneMapShader;

// mesh data
MeshData gCubeMeshData;
MeshData gSphereMeshData;
MeshData gConeMeshData;
MeshData gIcosahedronMeshData;
MeshData gQuadMeshData;

// mesh
Mesh gCubeMesh;
Mesh gSphereMesh;
Mesh gLightDebugMesh;
Mesh gFSQuadMesh;

Mesh gDirectionalLightMesh;
Mesh gPointLightMesh;
Mesh gPointLightPrepassMesh;

Mesh gToneMapMesh;

// texture
Texture2D gDiffuseMap;
Texture2D gNormalMap;

// light
Light gDirectionalLights[DIRECTIONAL_LIGHT_COUNT];
std::vector<Light> gPointLights;

// camera
Camera gCamera;

struct RenderInfo
{
	glm::mat4 View;
	glm::mat4 Proj;
	glm::vec2 Resolution;
} gRenderInfo;

struct RenderState
{
	bool bColorWrite;

	bool bDepthTest;
	GLenum depthTestFunc;
	bool bDepthWrite;

	bool bStencilTest;
	GLenum stencilTestFunc;
	GLint stencilTestRef;
	GLuint stencilTestMask;
	GLenum stencilWriteSFail;
	GLenum stencilWriteDFail;
	GLenum stencilWriteDPass;
	
	bool bCullFace;
	GLenum cullFaceMode;

	RenderState()
	{
		bColorWrite = true;

		bDepthTest = true;
		depthTestFunc = GL_LESS;
		bDepthWrite = true;

		bStencilTest = false;
		stencilTestFunc = GL_ALWAYS;
		stencilTestRef = 1;
		stencilTestMask = 0xFF;
		stencilWriteSFail = GL_KEEP;
		stencilWriteDFail = GL_KEEP;
		stencilWriteDPass = GL_KEEP;


		bCullFace = true;
		cullFaceMode = GL_BACK;
	}

	void Apply()
	{
		if (bColorWrite)
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		else
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		if (bDepthTest)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(depthTestFunc);
			glDepthMask(bDepthWrite);
		}
		else
			glDisable(GL_DEPTH_TEST);

		if (bStencilTest)
		{
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(stencilTestFunc, stencilTestRef, stencilTestMask);
			glStencilOp(stencilWriteSFail, stencilWriteDFail, stencilWriteDPass);
		}
		else
			glDisable(GL_STENCIL_TEST);

		if (bCullFace)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(cullFaceMode);
		}
		else
			glDisable(GL_CULL_FACE);
	}
};

// Render state
RenderState gGBufferState, gDebugForwardState;
RenderState gDirectionalLightState, gPointLightPrepassState, gPointLightState;
RenderState gPostProcessState, gPostProcessFinishState;

// input
float gMouseWheel;
const Uint8* gKeyStates;

bool init();
bool initGL();

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

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

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

void MakeLights()
{
	// directional lights
	memset(gDirectionalLights, 0, sizeof(Light) * DIRECTIONAL_LIGHT_COUNT);

	gDirectionalLights[0].direction = glm::normalize(glm::vec3(0, -1, -2));
	gDirectionalLights[0].color = glm::vec3(1.f, 1.f, 1.f);
	gDirectionalLights[0].intensity = 1;

	// point lights
	int plIdx = 0;
	//gPointLights.push_back(Light());
	//plIdx = gPointLights.size() - 1;
	//gPointLights[plIdx].position = glm::vec3(0, 10, 10);
	//gPointLights[plIdx].color = glm::vec3(1.f, 1.f, 1.f);
	//gPointLights[plIdx].intensity = 1;
	//gPointLights[plIdx].radius = 20.f;

	gPointLights.push_back(Light());
	plIdx = gPointLights.size() - 1;
	gPointLights[plIdx].position = glm::vec3(0, 3, 3);
	gPointLights[plIdx].color = glm::vec3(1.f, 1.f, 1.f);
	gPointLights[plIdx].intensity = 20;
	gPointLights[plIdx].radius = 12.f;

	gPointLights.push_back(Light());
	plIdx = gPointLights.size() - 1;
	gPointLights[plIdx].position = glm::vec3(10, 2, 0);
	gPointLights[plIdx].color = glm::vec3(0.f, 1.f, 0.f);
	gPointLights[plIdx].intensity = 20;
	gPointLights[plIdx].radius = 20.f;

	gPointLights.push_back(Light());
	plIdx = gPointLights.size() - 1;
	gPointLights[plIdx].position = glm::vec3(-10, 3, 3);
	gPointLights[plIdx].color = glm::vec3(1.f, 0.f, 0.f);
	gPointLights[plIdx].intensity = 20;
	gPointLights[plIdx].radius = 12.f;
}

void SetupFrameBuffers()
{
	// G-Buffer
	{
		// normal(RGB)
		// color(RGB)
		// matellic(R) + roughness(B)
		glGenFramebuffers(1, &gGBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gGBuffer);
		// normal
		//gNormalTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB16F, GL_RGB, GL_FLOAT);
		gNormalTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_BYTE);
		gNormalTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT0);
		// albedo
		gAlbedoTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
		gAlbedoTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT1);
		// material: metallic + roughness
		gMaterialTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
		gMaterialTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT2);
		// depth
		gDepthStencilTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		gDepthStencilTex.AttachToFrameBuffer(GL_DEPTH_STENCIL_ATTACHMENT);

		GLuint attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(_countof(attachments), attachments);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			printf("Error: G Buffer not complete!\n");
	}


	// Scene Buffer
	{
		glGenFramebuffers(1, &gSceneBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gSceneBuffer);
		// HDR scene color
		gSceneColorTex.AllocateForFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGBA16F, GL_RGBA, GL_FLOAT);
		gSceneColorTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT0);
		// re-use g-buffer depth
		gDepthStencilTex.AttachToFrameBuffer(GL_DEPTH_STENCIL_ATTACHMENT);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			printf("Error: Scene Buffer not complete!\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SetupRenderStates()
{
	// directional light
	{
		RenderState& s = gDirectionalLightState;
		// no depth test
		s.bDepthTest = false;
	}
	// point light prepass
	{
		RenderState& s = gPointLightPrepassState;
		// don't write color
		s.bColorWrite = false;
		// inverse depth test only, don't write depth
		s.bDepthTest = true;
		s.depthTestFunc = GL_GEQUAL;
		s.bDepthWrite = false;
		// stencil always pass, write stencil
		s.bStencilTest = true;
		s.stencilTestFunc = GL_ALWAYS;
		s.stencilTestRef = 1;
		s.stencilWriteDPass = GL_REPLACE;
		// draw backface
		s.bCullFace = true;
		s.cullFaceMode = GL_FRONT;
	}
	// point light
	{
		RenderState& s = gPointLightState;
		// depth test only, don't write depth
		s.bDepthTest = true;
		s.depthTestFunc = GL_LEQUAL;
		s.bDepthWrite = false;
		// stencil test equal only
		s.bStencilTest = true;
		s.stencilTestFunc = GL_EQUAL;
		s.stencilTestRef = 1;
	}
	// postprocess
	{
		RenderState& s = gPostProcessState;
		// no depth test
		s.bDepthTest = false;
	}
	// postprocess finish
	{
		RenderState& s = gPostProcessFinishState;
		// depth always pass, write depth
		s.bDepthTest = true;
		s.depthTestFunc = GL_ALWAYS;
		s.bDepthWrite = true;
	}
}

void LoadShaders(bool bReload)
{
	// clear cache
	gShaderFileCache.clear();

	//gTestShader.Load("Shader/test.vert", "Shader/test.frag", !bReload);
	gGBufferShader.Load("Shader/gbuffer.vert", "Shader/gbuffer.frag", !bReload);
	gPrepassShader.Load("Shader/prepass.vert", "Shader/prepass.frag", !bReload);
	gDirectionalLightShader.Load("Shader/fsQuad.vert", "Shader/fsQuadLight.frag", !bReload);
	gPointLightShader.Load("Shader/lightVolume.vert", "Shader/lightVolumePoint.frag", !bReload);
	gLightDebugShader.Load("Shader/test.vert", "Shader/lightDebug.frag", !bReload);
	gFSQuadShader.Load("Shader/fsQuad.vert", "Shader/fsQuadLight.frag", !bReload);
	gToneMapShader.Load("Shader/fsQuad.vert", "Shader/fsQuadToneMap.frag", !bReload);
}

bool initGL()
{
	// frame buffers
	SetupFrameBuffers();

	// ubo
	glGenBuffers(1, &gUBO_Matrices);
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderInfo), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	// binding point
	glBindBufferBase(GL_UNIFORM_BUFFER, Shader::RenderInfoBP, gUBO_Matrices);

	// render states
	SetupRenderStates();

	// shader
	LoadShaders(false);

	// mesh
	MakeCube(gCubeMeshData);
	MakeSphere(gSphereMeshData, 32);
	MakeIcosahedron(gIcosahedronMeshData, 2);
	MakeCone(gConeMeshData, 16, 2);
	MakeQuad(gQuadMeshData);

	// model
	gCubeMesh.Init(&gCubeMeshData, &gGBufferShader);
	gSphereMesh.Init(&gSphereMeshData, &gGBufferShader);
	gLightDebugMesh.Init(&gIcosahedronMeshData, &gLightDebugShader);
	gFSQuadMesh.Init(&gQuadMeshData, &gFSQuadShader);

	gDirectionalLightMesh.Init(&gQuadMeshData, &gDirectionalLightShader);
	gPointLightMesh.Init(&gIcosahedronMeshData, &gPointLightShader);
	gPointLightPrepassMesh.Init(&gIcosahedronMeshData, &gPrepassShader);

	gToneMapMesh.Init(&gQuadMeshData, &gToneMapShader);

	// texture
	gDiffuseMap.Load("Content/Texture/154.jpg", GL_SRGB, GL_RGB, GL_UNSIGNED_BYTE);
	gNormalMap.Load("Content/Texture/154_norm.jpg", GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);

	// light
	MakeLights();
	
	// camera
	gCamera.fov = glm::radians(90.f);
	gCamera.position = glm::vec3(0.f, 5.f, 20.f);
	gCamera.euler = glm::vec3(-10.f, 0.f, 0.f);

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
	gGBufferState.Apply();

	// clear frame buffer
	glClearColor(0, 0, 0, 0);
	glClearDepth(1);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// bind texture
	gDiffuseMap.Bind(Shader::diffuseTexUnit);
	gNormalMap.Bind(Shader::normalTexUnit);

	// draw models
	gGBufferShader.Use();
	
	glUniform1f(gGBufferShader.GetUniformLocation("metallic"), 1.0f);
	glUniform1f(gGBufferShader.GetUniformLocation("roughness"), 0.3f);
	
	for (int i = 0; i < 3; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(-10 + i * 10, 0, 0));
		modelMat = glm::rotate(modelMat, 45.f, glm::vec3(0, 1, 0));
		modelMat = glm::scale(modelMat, glm::vec3(1.5f, 1.f, 1.2f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gGBufferShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gGBufferShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		gCubeMesh.Draw();
	}

	// floor
	//{
	//	glm::mat4 modelMat(1);
	//	modelMat = glm::translate(modelMat, glm::vec3(0.f, -1.2f, 0.f));
	//	modelMat = glm::scale(modelMat, glm::vec3(16.f, 0.2f, 12.f));
	//	glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
	//	glUniformMatrix4fv(gGBufferShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
	//	glUniformMatrix3fv(gGBufferShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

	//	glUniform1f(gGBufferShader.GetUniformLocation("metallic"), 0.f);
	//	glUniform1f(gGBufferShader.GetUniformLocation("roughness"), 0.5f);

	//	gCubeMesh.Draw();
	//}

	for (int i = 0; i < 3; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(-10 + i * 10, 0, 5));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gGBufferShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gGBufferShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		glUniform1f(gGBufferShader.GetUniformLocation("metallic"), 1.f);
		glUniform1f(gGBufferShader.GetUniformLocation("roughness"), i * 0.45f + 0.1f);

		gSphereMesh.Draw();
	}

	for (int i = 0; i < 3; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(-10 + i * 10, 0, 7.5));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gGBufferShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gGBufferShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		glUniform1f(gGBufferShader.GetUniformLocation("metallic"), i * 0.5f);
		glUniform1f(gGBufferShader.GetUniformLocation("roughness"), 0.4f);

		gSphereMesh.Draw();
	}
}

void DirectionalLightPass(const Viewpoint& mainViewpoint)
{
	// directional light
	gDirectionalLightState.Apply();

	gDirectionalLightShader.Use();

	//glUniform3fv(gDirectionalLightShader.GetUniformLocation("viewPos"), 1, glm::value_ptr(mainViewpoint.position));

	// set light
	for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i)
	{
		//glUniform3fv(gDirectionalLightShader.GetUniformLocation("lights", i, "position"), 1, glm::value_ptr(gDirectionalLights[i].GetPositionViewSpace(mainViewpoint.viewMat)));
		glUniform3fv(gDirectionalLightShader.GetUniformLocation("lights", i, "direction"), 1, glm::value_ptr(gDirectionalLights[i].GetDirectionViewSpace(mainViewpoint.viewMat)));
		glUniform3fv(gDirectionalLightShader.GetUniformLocation("lights", i, "color"), 1, glm::value_ptr(gDirectionalLights[i].GetColorIntensity()));
		glUniform1f(gDirectionalLightShader.GetUniformLocation("lights", i, "radius"), gDirectionalLights[i].radius);
	}

	// draw quad
	gDirectionalLightMesh.Draw();
}

void PointLightPass(const Viewpoint& mainViewpoint)
{
	// point light
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	for (int i = 0; i < gPointLights.size(); ++i)
	{
		// model
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, gPointLights[i].position);
		modelMat = glm::scale(modelMat, glm::vec3(gPointLights[i].radius));
		
		// camera in light volume?
		if (Math::DistSquared(mainViewpoint.position, gPointLights[i].position) <
			gPointLights[i].radius * gPointLights[i].radius * 1.01f * 1.01f)
		{
			gPointLightPrepassState.Apply();
			// do back face draw directly
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			// no need for stencil test
			glDisable(GL_STENCIL_TEST);
		}
		else
		{
			// do 2-pass
			// clear stencil
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT);

			// prepass
			gPointLightPrepassState.Apply();
			gPrepassShader.Use();
			glUniformMatrix4fv(gPrepassShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
			gPointLightPrepassMesh.Draw();

			// draw light
			gPointLightState.Apply();
		}

		gPointLightShader.Use();
		
		glUniform3fv(gPointLightShader.GetUniformLocation("light", "position"), 1, glm::value_ptr(gPointLights[i].GetPositionViewSpace(mainViewpoint.viewMat)));
		//glUniform3fv(gPointLightShader.GetUniformLocation("light", "direction"), 1, glm::value_ptr(gPointLights[i].GetDirectionViewSpace(mainViewpoint.viewMat)));
		glUniform3fv(gPointLightShader.GetUniformLocation("light", "color"), 1, glm::value_ptr(gPointLights[i].GetColorIntensity()));
		glUniform1f(gPointLightShader.GetUniformLocation("light", "radius"), gPointLights[i].radius);
		
		glUniformMatrix4fv(gPointLightShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));

		gPointLightMesh.Draw();
		
	}

	glDisable(GL_BLEND);
}

void LightPass(const Viewpoint& mainViewpoint)
{
	// bind textures
	//gPositionTex.Bind(Shader::gPositionTexUnit);
	gNormalTex.Bind(Shader::gNormalTexUnit);
	gAlbedoTex.Bind(Shader::gAlbedoTexUnit);
	gMaterialTex.Bind(Shader::gMaterialTexUnit);
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);
	
	// clear color buffer only.
	glClearColor(0, 0, 0, 0);
	//glClearDepth(1);
	//glClearStencil(0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	DirectionalLightPass(mainViewpoint);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	PointLightPass(mainViewpoint);	
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

}

void DebugForwardPass()
{
	gDebugForwardState.Apply();

	// draw debug
	gLightDebugShader.Use();

	for (int i = 0; i < gPointLights.size(); ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, gPointLights[i].position);
		modelMat = glm::scale(modelMat, glm::vec3(0.3f, 0.3f, 0.3f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gLightDebugShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gLightDebugShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));
		glUniform3fv(gLightDebugShader.GetUniformLocation("color"), 1, glm::value_ptr(gPointLights[i].GetColorIntensity()));

		gLightDebugMesh.Draw();
	}
}

void PostProcessPass()
{
	gSceneColorTex.Bind(Shader::sceneColorTexUnit);

	// directional light
	gPostProcessFinishState.Apply();

	gToneMapShader.Use();

	// draw quad
	gToneMapMesh.Draw();
}

void render()
{
	const Viewpoint& mainViewpoint = gCamera.ProcessCamera((GLfloat)SCREEN_WIDTH, (GLfloat)SCREEN_HEIGHT, 0.1f, 1000.f);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// update ubo
	gRenderInfo.View = mainViewpoint.viewMat;
	gRenderInfo.Proj = mainViewpoint.projMat;
	gRenderInfo.Resolution.x = mainViewpoint.width;
	gRenderInfo.Resolution.y = mainViewpoint.height;
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	// maybe use glBufferSubData later?
	glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderInfo), &gRenderInfo, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	// bind G-buffer
	glBindFramebuffer(GL_FRAMEBUFFER, gGBuffer);

	GeometryPass(mainViewpoint);

	// bind Scene-buffer
	glBindFramebuffer(GL_FRAMEBUFFER, gSceneBuffer);
	
	LightPass(mainViewpoint);

	DebugForwardPass();

	// unbind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	PostProcessPass();
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
				else if (event.type == SDL_KEYUP)
				{
					if (event.key.keysym.sym == SDLK_r)
						LoadShaders(true);
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
