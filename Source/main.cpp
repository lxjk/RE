
// sdl
//#define SDL_MAIN_HANDLED
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
#include <functional>

// local
#include "Engine/Shader.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/MeshLoader.h"
#include "Engine/Texture2D.h"
#include "Engine/Light.h"
#include "Engine/Viewpoint.h"
#include "Engine/Camera.h"
#include "Engine/Math.h"

// imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl.h"

#define PROFILE 1
#if PROFILE
#define CPU_SCOPED_PROFILE(name) ScopedProfileTimer _cpu_macro_profiler(name, false)
#define GPU_SCOPED_PROFILE(name) ScopedProfileTimer _gpu_macro_profiler(name, true)
#else
#define CPU_SCOPED_PROFILE(name) 
#define GPU_SCOPED_PROFILE(name) 
#endif

int gWindowWidth = 1280;
int gWindowHeight = 720;

const int DIRECTIONAL_LIGHT_COUNT = 4;

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

// opengl context
SDL_GLContext gContext;

// time
double gInvPerformanceFreq;
Uint64 gPerformanceCounter;
float gLastDeltaTime;
float gAverageDeltaTime;
float gDeltaTimeAccum;
float gDeltaTimeBuffer[60];
int gDeltaTimeBufferCount;
int gDeltaTimeBufferIdx;

// ubo
GLuint gUBO_Matrices = 0;

// frame buffer
GLuint gGBuffer = 0;
Texture2D gNormalTex, gAlbedoTex, gMaterialTex, gDepthStencilTex;

GLuint gSceneBuffer = 0;
Texture2D gSceneColorTex, gSceneDepthStencilTex;
GLuint gSceneDepthStencilRBO;

// shader
Shader gGBufferShader;
Shader gGBufferColorShader;
Shader gPrepassShader;
Shader gDirectionalLightShader;
Shader gLightVolumeShader;
Shader gLightDebugShader;
Shader gFSQuadShader;
Shader gToneMapShader;

// material
Material gGBufferMaterial(&gGBufferShader);
Material gGBufferColorMaterial(&gGBufferColorShader);
Material gPrepassMaterial(&gPrepassShader);
Material gDirectionalLightMaterial(&gDirectionalLightShader);
Material gLightVolumeMaterial(&gLightVolumeShader);
Material gLightDebugMaterial(&gLightDebugShader);
Material gFSQuadMaterial(&gFSQuadShader);
Material gToneMapMaterial(&gToneMapShader);

// mesh data
MeshData gCubeMeshData;
MeshData gSphereMeshData;
MeshData gConeMeshData;
MeshData gIcosahedronMeshData;
MeshData gQuadMeshData;

// mesh
Mesh* gCubeMesh;
Mesh* gSphereMesh;
Mesh* gFSQuadMesh;
std::vector<Mesh*> gNanosuitMeshes;

Mesh* gCubeColorMesh;

Mesh* gDirectionalLightMesh;
Mesh* gPointLightMesh;
Mesh* gPointLightPrepassMesh;
Mesh* gSpotLightMesh;
Mesh* gSpotLightPrepassMesh;

Mesh* gToneMapMesh;

Mesh* gPointLightDebugMesh;
Mesh* gSpotLightDebugMesh;

// texture
Texture2D* gDiffuseMap;
Texture2D* gNormalMap;

// light
Light gDirectionalLights[DIRECTIONAL_LIGHT_COUNT];
std::vector<Light> gPointLights;
std::vector<Light> gSpotLights;

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
	bool bColorWrite = true;

	bool bDepthTest = true;
	GLenum depthTestFunc = GL_LESS;
	bool bDepthWrite = true;

	bool bStencilTest = false;
	GLenum stencilTestFunc = GL_ALWAYS;
	GLint stencilTestRef = 1;
	GLuint stencilTestMask = 0xFF;
	GLenum stencilWriteSFail = GL_KEEP;
	GLenum stencilWriteDFail = GL_KEEP;
	GLenum stencilWriteDPass = GL_KEEP;
	
	bool bCullFace = true;
	GLenum cullFaceMode = GL_BACK;

	RenderState() {}
	
	// constructor with a init function callback
	RenderState(std::function<void (RenderState&)> initFunc) : RenderState()
	{
		if (initFunc)
			initFunc(*this);
	}

	void Apply() const
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

// input
bool gMouseCaptured = false;
float gMouseWheel;
const Uint8* gKeyStates;

class ScopedProfileTimer
{
	Uint64 start;
	glm::uvec2 query;
	std::string name;
	bool bGPUTimer;

public:
	static std::map<std::string, double> timerMapCPU;
	static std::map<std::string, double> timerMapGPU;
	static std::map<std::string, std::vector<glm::uvec2>> timeStampPairMapGPU[2];
	static int GPUMapWriteIdx;
	static std::string fullName;

	ScopedProfileTimer(std::string inName, bool inGPUTimer)
	{
		name = inName;
		bGPUTimer = inGPUTimer;
		// add to prefix
		fullName.append("/");
		fullName.append(name);
		if (bGPUTimer)
		{
			glGenQueries(2, glm::value_ptr(query));
			glQueryCounter(query[0], GL_TIMESTAMP);
		}
		else
			start = SDL_GetPerformanceCounter();
	}

	~ScopedProfileTimer()
	{
		if (bGPUTimer)
		{
			glQueryCounter(query[1], GL_TIMESTAMP);
			timeStampPairMapGPU[GPUMapWriteIdx][fullName].push_back(query);
			auto it = timerMapGPU.find(fullName);
			if (it == timerMapGPU.end())
			{
				timerMapGPU[fullName] = 0;
			}
		}
		else
		{
			Uint64 end = SDL_GetPerformanceCounter();
			double deltaTime = (double)((end - start) * 1000) * gInvPerformanceFreq;
			auto it = timerMapCPU.find(fullName);
			if (it != timerMapCPU.end())
			{
				// find it
				it->second += deltaTime;
			}
			else
			{
				timerMapCPU[fullName] = deltaTime;
			}
		}
		// remove from full name
		fullName = fullName.substr(0, fullName.size() - name.size() - 1);
	}
};
std::map<std::string, double> ScopedProfileTimer::timerMapCPU;
std::map<std::string, double> ScopedProfileTimer::timerMapGPU;
std::map<std::string, std::vector<glm::uvec2>> ScopedProfileTimer::timeStampPairMapGPU[2];
int ScopedProfileTimer::GPUMapWriteIdx = 0;
std::string ScopedProfileTimer::fullName;


bool Init();
bool InitEngine();

bool Init()
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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// create window with opengl
	gWindow = SDL_CreateWindow("RE",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, gWindowWidth, gWindowHeight,
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

	// init imgui
	ImGui_Impl_Init(gWindow);
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("Content/Fonts/DroidSans.ttf", 22.0f);

	//Use Vsync
	//if (SDL_GL_SetSwapInterval(1) < 0)
	//{
	//	printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	//}

	//Initialize Engine
	if (!InitEngine())
	{
		printf("Unable to initialize Engine!\n");
		return false;
	}

	// set key states
	gKeyStates = SDL_GetKeyboardState(NULL);

	// time
	gInvPerformanceFreq = (double)1 / (double)SDL_GetPerformanceFrequency();
	gLastDeltaTime = 0;
	gAverageDeltaTime = 0.03f;
	gDeltaTimeAccum = 0.f;
	gDeltaTimeBufferCount = _countof(gDeltaTimeBuffer);
	memset(gDeltaTimeBuffer, 0, sizeof(float) * gDeltaTimeBufferCount);
	gDeltaTimeBufferIdx = 0;

	return true;
}

void MakeLights()
{
	// directional lights
	memset(gDirectionalLights, 0, sizeof(Light) * DIRECTIONAL_LIGHT_COUNT);

	gDirectionalLights[0].SetDirectionLight(
		/*dir=*/	glm::vec3(0, -1, -2),
		/*color=*/	glm::vec3(1.f, 1.f, 1.f),
		/*int=*/	1
	);

	// point lights
	int plIdx = 0;
	//gPointLights.push_back(Light());
	//plIdx = gPointLights.size() - 1;
	//gPointLights[plIdx].position = glm::vec3(0, 10, 10);
	//gPointLights[plIdx].color = glm::vec3(1.f, 1.f, 1.f);
	//gPointLights[plIdx].intensity = 1;
	//gPointLights[plIdx].radius = 20.f;

	gPointLights.push_back(Light());
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	glm::vec3(0, 3, 3),
		/*radius=*/	12.f,
		/*color=*/	glm::vec3(1.f, 1.f, 1.f),
		/*int=*/	20
	);

	gPointLights.push_back(Light());
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	glm::vec3(10, 2, 0),
		/*radius=*/	20.f,
		/*color=*/	glm::vec3(0.f, 1.f, 0.f),
		/*int=*/	20
	);

	gPointLights.push_back(Light());
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	glm::vec3(-10, 3, 3),
		/*radius=*/	12.f,
		/*color=*/	glm::vec3(1.f, 0.f, 0.f),
		/*int=*/	20
	);

	// spot lights
	int slIdx = 0;
	gSpotLights.push_back(Light());
	slIdx = (int)gSpotLights.size() - 1;
	gSpotLights[slIdx].SetSpotLight(
		/*pos=*/	glm::vec3(0, 2, 6),
		/*dir=*/	glm::vec3(-2, -0.5f, -0.2f),
		/*radius=*/	20.f,
		/*hOuter=*/	30.f,
		/*hInner=*/	20.f,
		/*color=*/	glm::vec3(0.f, 0.f, 1.f),
		/*int=*/	100
	);
}

void SetupFrameBuffers()
{
	// G-Buffer
	if(!gGBuffer)
	{
		// normal(RGB)
		// color(RGB)
		// matellic(R) + roughness(B)
		glGenFramebuffers(1, &gGBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gGBuffer);
		// normal
		//gNormalTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGB16F, GL_RGB, GL_FLOAT);
		gNormalTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_BYTE);
		gNormalTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT0);
		// albedo
		gAlbedoTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
		gAlbedoTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT1);
		// material: metallic + roughness
		gMaterialTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
		gMaterialTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT2);
		// depth stencil
		gDepthStencilTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		gDepthStencilTex.AttachToFrameBuffer(GL_DEPTH_STENCIL_ATTACHMENT);

		GLuint attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(_countof(attachments), attachments);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			printf("Error: G Buffer not complete!\n");
	}
	else
	{
		// normal
		gNormalTex.Reallocate(gWindowWidth, gWindowHeight);
		// albedo
		gAlbedoTex.Reallocate(gWindowWidth, gWindowHeight);
		// material: metallic + roughness
		gMaterialTex.Reallocate(gWindowWidth, gWindowHeight);
		// depth stencil
		gDepthStencilTex.Reallocate(gWindowWidth, gWindowHeight);
	}


	// Scene Buffer
	if(!gSceneBuffer)
	{
		glGenFramebuffers(1, &gSceneBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gSceneBuffer);
		// HDR scene color
		gSceneColorTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGBA16F, GL_RGBA, GL_FLOAT);
		gSceneColorTex.AttachToFrameBuffer(GL_COLOR_ATTACHMENT0);
		// depth stencil
		//gSceneDepthStencilTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		//gSceneDepthStencilTex.AttachToFrameBuffer(GL_DEPTH_STENCIL_ATTACHMENT);
		// depth stencil rbo
		glGenRenderbuffers(1, &gSceneDepthStencilRBO);
		glBindRenderbuffer(GL_RENDERBUFFER, gSceneDepthStencilRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gWindowWidth, gWindowHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gSceneDepthStencilRBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			printf("Error: Scene Buffer not complete!\n");
	}
	else
	{
		// HDR scene color
		gSceneColorTex.Reallocate(gWindowWidth, gWindowHeight);
		// depth stencil
		//gSceneDepthStencilTex.Reallocate(gWindowWidth, gWindowHeight);
		// depth stencil rbo
		glBindRenderbuffer(GL_RENDERBUFFER, gSceneDepthStencilRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gWindowWidth, gWindowHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LoadShaders(bool bReload)
{
	// clear cache
	gShaderFileCache.clear();

	gGBufferShader.Load("Shader/gbuffer.vert", "Shader/gbuffer.frag", !bReload);
	gGBufferColorShader.Load("Shader/gbuffer.vert", "Shader/gbufferColor.frag", !bReload);
	gPrepassShader.Load("Shader/prepass.vert", "Shader/prepass.frag", !bReload);
	gDirectionalLightShader.Load("Shader/fsQuad.vert", "Shader/fsQuadLight.frag", !bReload);
	gLightVolumeShader.Load("Shader/lightVolume.vert", "Shader/lightVolume.frag", !bReload);
	gLightDebugShader.Load("Shader/test.vert", "Shader/lightDebug.frag", !bReload);
	gFSQuadShader.Load("Shader/fsQuad.vert", "Shader/fsQuadLight.frag", !bReload);
	gToneMapShader.Load("Shader/fsQuad.vert", "Shader/fsQuadToneMap.frag", !bReload);
}

bool InitEngine()
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
	
	// shader
	LoadShaders(false);

	// mesh data
	MakeCube(gCubeMeshData);
	MakeSphere(gSphereMeshData, 32);
	MakeIcosahedron(gIcosahedronMeshData, 2);
	MakeCone(gConeMeshData, 16, 2);
	MakeQuad(gQuadMeshData);

	// mesh
	gCubeMesh = Mesh::Create(&gCubeMeshData, &gGBufferMaterial);
	gSphereMesh = Mesh::Create(&gSphereMeshData, &gGBufferMaterial);
	gFSQuadMesh = Mesh::Create(&gQuadMeshData, &gFSQuadMaterial);

	LoadMesh(gNanosuitMeshes, "Content/Model/nanosuit/nanosuit.obj", &gGBufferShader);
	//LoadMesh(gNanosuitMeshes, "Content/Model/Lakecity/Lakecity.obj", &gGBufferShader);

	gCubeColorMesh = Mesh::Create(&gCubeMeshData, &gGBufferColorMaterial);

	gDirectionalLightMesh = Mesh::Create(&gQuadMeshData, &gDirectionalLightMaterial);
	gPointLightMesh = Mesh::Create(&gIcosahedronMeshData, &gLightVolumeMaterial);
	gPointLightPrepassMesh = Mesh::Create(&gIcosahedronMeshData, &gPrepassMaterial);
	gSpotLightMesh = Mesh::Create(&gConeMeshData, &gLightVolumeMaterial);
	gSpotLightPrepassMesh = Mesh::Create(&gConeMeshData, &gPrepassMaterial);

	gToneMapMesh = Mesh::Create(&gQuadMeshData, &gToneMapMaterial);

	gPointLightDebugMesh = Mesh::Create(&gIcosahedronMeshData, &gLightDebugMaterial);
	gSpotLightDebugMesh = Mesh::Create(&gConeMeshData, &gLightDebugMaterial);

	// texture
	gDiffuseMap = Texture2D::FindOrCreate("Content/Texture/154.jpg", true);
	gNormalMap = Texture2D::FindOrCreate("Content/Texture/154_norm.jpg", false);

	// set textures
	gGBufferMaterial.SetTexture("diffuseTex", gDiffuseMap);
	gGBufferMaterial.SetTexture("normalTex", gNormalMap);

	// light
	MakeLights();
	
	// camera
	gCamera.fov = glm::radians(90.f);
	gCamera.position = glm::vec3(0.f, 5.f, 20.f);
	gCamera.euler = glm::vec3(-10.f, 0.f, 0.f);

	return true;
}

void OnWindowResize(int newWidth, int newHeight)
{
	gWindowWidth = newWidth;
	gWindowHeight = newHeight;

	SetupFrameBuffers();
}

void Close()
{
	// shut down imgui
	ImGui_Impl_Shutdown();

	SDL_GL_DeleteContext(gContext);

	//Destroy window
	SDL_DestroyWindow(gWindow);

	//Quit SDL_Image
	IMG_Quit();

	//Quit SDL subsystems
	SDL_Quit();
}

void updateMouseInput(float deltaTime)
{
	bool bShouldCaptureMouse = false;

	int x = 0, y = 0;
	Uint32 mouseState = SDL_GetRelativeMouseState(&x, &y);

	float deltaX = (float)x;
	float deltaY = (float)y;

	const static float rotSpeed = 30.f;
	const static float moveSpeed = 12.f;
	const static float scrollSpeed = 120.f;
	
	if (mouseState & (/*SDL_BUTTON(SDL_BUTTON_LEFT) | */SDL_BUTTON(SDL_BUTTON_RIGHT)))
	{
		gCamera.euler.y -= rotSpeed * deltaTime * deltaX;
		gCamera.euler.x -= rotSpeed * deltaTime * deltaY;
		gCamera.euler.x = glm::clamp(gCamera.euler.x, -89.f, 89.f);
		bShouldCaptureMouse = true;
	}

	glm::quat cameraRot(glm::radians(gCamera.euler));
	glm::vec3 cameraForward = cameraRot * glm::vec3(0, 0, -1);
	glm::vec3 cameraUp = cameraRot * glm::vec3(0, 1, 0);
	glm::vec3 cameraRight = cameraRot * glm::vec3(1, 0, 0);

	if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
	{
		gCamera.position -= cameraRight * deltaX * moveSpeed * deltaTime;
		gCamera.position += cameraUp * deltaY * moveSpeed * deltaTime;
		bShouldCaptureMouse = true;
	}
	// mouse wheel?
	if (gMouseWheel != 0)
	{
		gCamera.position += cameraForward * gMouseWheel * scrollSpeed * deltaTime;
		gMouseWheel = 0;
	}

	if (bShouldCaptureMouse && !gMouseCaptured)
	{
		SDL_CaptureMouse(SDL_TRUE);
		gMouseCaptured = true;
	}
	else if (!bShouldCaptureMouse && gMouseCaptured)
	{
		SDL_CaptureMouse(SDL_FALSE);
		gMouseCaptured = false;
	}
}

void updateKeyboardInput(float deltaTime)
{
	int x = 0, y = 0;
	Uint32 mouseState = SDL_GetRelativeMouseState(&x, &y);

	glm::quat cameraRot(glm::radians(gCamera.euler));
	glm::vec3 cameraForward = cameraRot * glm::vec3(0, 0, -1);
	glm::vec3 cameraRight = cameraRot * glm::vec3(1, 0, 0);

	glm::vec3 up = glm::vec3(0, 1, 0);

	const static float moveSpeed = 12.f;

	if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
	{
		if (gKeyStates[SDL_SCANCODE_W])
		{
			gCamera.position += cameraForward * moveSpeed * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_S])
		{
			gCamera.position -= cameraForward * moveSpeed * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_D])
		{
			gCamera.position += cameraRight * moveSpeed * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_A])
		{
			gCamera.position -= cameraRight * moveSpeed * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_E])
		{
			gCamera.position += up * moveSpeed * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_Q])
		{
			gCamera.position -= up * moveSpeed * deltaTime;
		}
	}

}

void Update(float deltaTime)
{
	CPU_SCOPED_PROFILE("update");

	updateMouseInput(deltaTime);
	updateKeyboardInput(deltaTime);

	// update spot light
	if (gSpotLights.size() > 0)
	{
		static const float totalTime = 4.f;
		static float spotLightLocalTime = 0.f;
		spotLightLocalTime += deltaTime;
		if (spotLightLocalTime > totalTime)
		{
			spotLightLocalTime -= totalTime;
		}

		float ratio = spotLightLocalTime / totalTime;
		ratio = glm::abs(ratio * 2 - 1); // [0 - 1] -> [1 - 0 - 1]

		const glm::vec3 startPos(12.5f, 1, 6);
		const glm::vec3 endPos(-12.5f, 1, 6);
		const glm::vec3 startDir(-2, -0.5f, -0.2f);
		const glm::vec3 endDir(2, -0.5f, -0.2f);
		
		//gSpotLights[0].position = glm::mix(startPos, endPos, ratio);
		gSpotLights[0].direction = glm::normalize(glm::mix(startDir, endDir, ratio));
	}

	// update imgui
	ImGui_Impl_NewFrame(gWindow);
}

void GeometryPass(const Viewpoint& mainViewpoint)
{
	GPU_SCOPED_PROFILE("geometry");

	static const RenderState renderState;

	renderState.Apply();

	// clear frame buffer
	glClearColor(0, 0, 0, 0);
	glClearDepth(1);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	// draw models
	gGBufferMaterial.Use();

	glUniform1f(gGBufferMaterial.shader->GetUniformLocation("metallic"), 1.0f);
	glUniform1f(gGBufferMaterial.shader->GetUniformLocation("roughness"), 0.3f);
	
	for (int i = 0; i < 3; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(-10 + i * 10, 0, 0));
		modelMat = glm::rotate(modelMat, 45.f, glm::vec3(0, 1, 0));
		modelMat = glm::scale(modelMat, glm::vec3(1.5f, 1.f, 1.2f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gGBufferMaterial.shader->GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gGBufferMaterial.shader->GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		gCubeMesh->Draw();
	}

	for (int i = 0; i < 3; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(-10 + i * 10, 0, 5));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gGBufferMaterial.shader->GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gGBufferMaterial.shader->GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		glUniform1f(gGBufferMaterial.shader->GetUniformLocation("metallic"), 1.f);
		glUniform1f(gGBufferMaterial.shader->GetUniformLocation("roughness"), i * 0.45f + 0.1f);

		gSphereMesh->Draw();
	}

	for (int i = 0; i < 3; ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(-10 + i * 10, 0, 7.5));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gGBufferMaterial.shader->GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gGBufferMaterial.shader->GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		glUniform1f(gGBufferMaterial.shader->GetUniformLocation("metallic"), i * 0.5f);
		glUniform1f(gGBufferMaterial.shader->GetUniformLocation("roughness"), 0.4f);

		gSphereMesh->Draw();
	}
	
	// nanosuit
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(5, -1, 5));
		modelMat = glm::scale(modelMat, glm::vec3(0.3f, 0.3f, 0.3f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));

		for (int i = 0; i < gNanosuitMeshes.size(); ++i)
		{
			Material* material = gNanosuitMeshes[i]->material;
			if (!material)
				continue;

			material->Use();

			glUniformMatrix4fv(material->shader->GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
			glUniformMatrix3fv(material->shader->GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

			glUniform1f(material->shader->GetUniformLocation("metallic"), 0.f);
			glUniform1f(material->shader->GetUniformLocation("roughness"), 0.3f);

			gNanosuitMeshes[i]->Draw();
		}
	}

	gGBufferColorMaterial.Use();

	// floor
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, glm::vec3(0.f, -1.2f, 0.f));
		modelMat = glm::scale(modelMat, glm::vec3(16.f, 0.2f, 12.f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gGBufferColorMaterial.shader->GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix3fv(gGBufferColorMaterial.shader->GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));

		glUniform1f(gGBufferColorMaterial.shader->GetUniformLocation("metallic"), 0.f);
		glUniform1f(gGBufferColorMaterial.shader->GetUniformLocation("roughness"), 1.f);
		glUniform3fv(gGBufferColorMaterial.shader->GetUniformLocation("color"), 1, glm::value_ptr(glm::vec3(0.2f)));

		gCubeColorMesh->Draw();
	}
}

void DirectionalLightPass(const Viewpoint& mainViewpoint)
{
	GPU_SCOPED_PROFILE("directional light");

	const static RenderState renderState([](RenderState& s) {
		//// no depth test
		//s.bDepthTest = false;
		// we need to write depth to new depth buffer
		s.bDepthTest = true;
		s.depthTestFunc = GL_ALWAYS;
		s.bDepthWrite = true;
	});

	glClearDepth(1);
	glClear(GL_DEPTH_BUFFER_BIT);

	// directional light
	renderState.Apply();

	gDirectionalLightMaterial.Use();

	//glUniform3fv(gDirectionalLightShader.GetUniformLocation("viewPos"), 1, glm::value_ptr(mainViewpoint.position));

	// set light
	for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i)
	{
		//glUniform3fv(gDirectionalLightShader.GetUniformLocation("lights", i, "position"), 1, glm::value_ptr(gDirectionalLights[i].GetPositionViewSpace(mainViewpoint.viewMat)));
		glUniform3fv(gDirectionalLightMaterial.shader->GetUniformLocation("lights", i, "direction"), 1, glm::value_ptr(gDirectionalLights[i].GetDirectionVS(mainViewpoint.viewMat)));
		glUniform3fv(gDirectionalLightMaterial.shader->GetUniformLocation("lights", i, "color"), 1, glm::value_ptr(gDirectionalLights[i].colorIntensity));
		glUniform4fv(gDirectionalLightMaterial.shader->GetUniformLocation("lights", i, "attenParams"), 1, glm::value_ptr(gDirectionalLights[i].attenParams));
	}

	// draw quad
	gDirectionalLightMesh->Draw();
}

void LightVolumePass(const Viewpoint& mainViewpoint, const std::vector<Light>& lights, const Mesh* lightVolumePrepassMesh, const Mesh* lightVolumeMesh)
{
	//GPU_SCOPED_PROFILE("light volume");

	// prepass render state
	const static RenderState prepassRenderState( [] (RenderState& s) {
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
	});

	// lighting render state
	const static RenderState lightingRenderState( [] (RenderState& s) {
		// depth test only, don't write depth
		s.bDepthTest = true;
		s.depthTestFunc = GL_LEQUAL;
		s.bDepthWrite = false;
		// stencil test equal only
		s.bStencilTest = true;
		s.stencilTestFunc = GL_EQUAL;
		s.stencilTestRef = 1;
	});

	// one pass render state, when camera inside volume
	const static RenderState onePassVolumeRenderState([](RenderState& s) {
		// inverse depth test only, don't write depth
		s.bDepthTest = true;
		s.depthTestFunc = GL_GEQUAL;
		s.bDepthWrite = false;
		// draw backface
		s.bCullFace = true;
		s.cullFaceMode = GL_FRONT;
	});

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	for (int i = 0; i < lights.size(); ++i)
	{
		bool bSpot = (lights[i].attenParams.y > 0);
		// model
		glm::mat4 modelMat(1);
		if (bSpot)
		{
			modelMat = Math::MakeMatFromForward(lights[i].direction);
			modelMat[3] = glm::vec4(lights[i].position, 1);
			modelMat = glm::scale(modelMat, glm::vec3(lights[i].endRadius, lights[i].endRadius, lights[i].radius));
		}
		else
		{
			modelMat = glm::translate(modelMat, gPointLights[i].position);
			modelMat = glm::scale(modelMat, glm::vec3(gPointLights[i].radius));
		}

		// camera in light volume?
		glm::vec3 lightToCamera = mainViewpoint.position - lights[i].position;
		bool isInLightVolume = (Math::SizeSquared(lightToCamera) <	lights[i].radius * lights[i].radius * 1.01f * 1.01f);
		if (bSpot)
		{
			isInLightVolume &= (glm::dot(glm::normalize(lightToCamera), lights[i].direction) > lights[i].outerCosHalfAngle - KINDA_SMALL_NUMBER);
		}
		if (isInLightVolume)
		{
			// do 1-pass
			onePassVolumeRenderState.Apply();
		}
		else
		{
			// do 2-pass
			// clear stencil
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT);

			// prepass
			prepassRenderState.Apply();
			gPrepassMaterial.Use();
			glUniformMatrix4fv(gPrepassMaterial.shader->GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
			lightVolumePrepassMesh->Draw();

			// draw light
			lightingRenderState.Apply();
		}

		gLightVolumeMaterial.Use();

		glUniform4fv(gLightVolumeMaterial.shader->GetUniformLocation("light", "positionInvR"), 1, glm::value_ptr(lights[i].GetPositionVSInvR(mainViewpoint.viewMat)));
		glUniform3fv(gLightVolumeMaterial.shader->GetUniformLocation("light", "direction"), 1, glm::value_ptr(lights[i].GetDirectionVS(mainViewpoint.viewMat)));
		glUniform3fv(gLightVolumeMaterial.shader->GetUniformLocation("light", "color"), 1, glm::value_ptr(lights[i].colorIntensity));
		glUniform4fv(gLightVolumeMaterial.shader->GetUniformLocation("light", "attenParams"), 1, glm::value_ptr(lights[i].attenParams));

		glUniformMatrix4fv(gLightVolumeMaterial.shader->GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));

		lightVolumeMesh->Draw();

	}

	glDisable(GL_BLEND);
}

void PointLightPass(const Viewpoint& mainViewpoint)
{
	GPU_SCOPED_PROFILE("point light");
	LightVolumePass(mainViewpoint, gPointLights, gPointLightPrepassMesh, gPointLightMesh);
}

void SpotLightPass(const Viewpoint& mainViewpoint)
{
	GPU_SCOPED_PROFILE("spot light");
	LightVolumePass(mainViewpoint, gSpotLights, gSpotLightPrepassMesh, gSpotLightMesh);
}

void LightPass(const Viewpoint& mainViewpoint)
{
	GPU_SCOPED_PROFILE("light");
	
	// clear color buffer only.
	glClearColor(0, 0, 0, 0);
	//glClearDepth(1);
	//glClearStencil(0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	DirectionalLightPass(mainViewpoint);
	PointLightPass(mainViewpoint);
	SpotLightPass(mainViewpoint);

}

void DebugForwardPass()
{
	GPU_SCOPED_PROFILE("debug foward");

	static const RenderState renderState;

	renderState.Apply();

	// draw debug
	gLightDebugShader.Use();

	for (int i = 0; i < gPointLights.size(); ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, gPointLights[i].position);
		modelMat = glm::scale(modelMat, glm::vec3(0.3f, 0.3f, 0.3f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gLightDebugShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		//glUniformMatrix3fv(gLightDebugShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));
		glUniform3fv(gLightDebugShader.GetUniformLocation("color"), 1, glm::value_ptr(gPointLights[i].colorIntensity));

		gPointLightDebugMesh->Draw();
	}

	for (int i = 0; i < gSpotLights.size(); ++i)
	{
		glm::mat4 modelMat(1);
		//modelMat = glm::translate(modelMat, gSpotLights[i].position);
		modelMat = Math::MakeMatFromForward(gSpotLights[i].direction);
		modelMat[3] = glm::vec4(gSpotLights[i].position, 1);
		modelMat = glm::scale(modelMat, glm::vec3(gSpotLights[i].endRadius, gSpotLights[i].endRadius, gSpotLights[i].radius) / gSpotLights[i].radius * 0.6f);
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		glUniformMatrix4fv(gLightDebugShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
		//glUniformMatrix3fv(gLightDebugShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));
		glUniform3fv(gLightDebugShader.GetUniformLocation("color"), 1, glm::value_ptr(gSpotLights[i].colorIntensity));

		gSpotLightDebugMesh->Draw();
	}

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glDisable(GL_CULL_FACE);

	//gLightDebugShader.Use();

	//for (int i = 0; i < gPointLights.size(); ++i)
	//{
	//	// model
	//	glm::mat4 modelMat(1);
	//	modelMat = glm::translate(modelMat, gPointLights[i].position);
	//	modelMat = glm::scale(modelMat, glm::vec3(gPointLights[i].radius));
	//	glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
	//			
	//	glUniformMatrix4fv(gLightDebugShader.GetUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
	//	glUniformMatrix3fv(gLightDebugShader.GetUniformLocation("normalMat"), 1, GL_FALSE, glm::value_ptr(normalMat));
	//	glUniform3fv(gLightDebugShader.GetUniformLocation("color"), 1, glm::value_ptr(gPointLights[i].colorIntensity));

	//	gPointLightDebugMesh->Draw();

	//}

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void PostProcessPass()
{
	GPU_SCOPED_PROFILE("post process");

	const static RenderState renderState( [] (RenderState& s) {
		// no depth test
		s.bDepthTest = false;
	});

	renderState.Apply();

	gToneMapShader.Use();

	// draw quad
	gToneMapMesh->Draw();
}

void UIPass()
{
	GPU_SCOPED_PROFILE("UI");

	{
		ImColor red(1.f, 0.f, 0.f);
		ImColor yellow(1.f, 1.f, 0.f);
		ImColor green(0.f, 1.f, 0.f);

		static bool open = true;
		ImGui::Begin("stats", &open);

		float fps = 1.f / gAverageDeltaTime;
		float averageFrameTime = gAverageDeltaTime * 1000.f;
		ImColor fpsColor = fps > 60 ? green : (fps > 30 ? yellow : red);
		ImGui::TextColored(fpsColor, "FPS %.1f \t %.3f ms", fps, averageFrameTime);

		// profiling cpu
		for (auto it = ScopedProfileTimer::timerMapCPU.begin(); it != ScopedProfileTimer::timerMapCPU.end(); ++it)
		{
			size_t layer = std::count(it->first.begin(), it->first.end(), '/') - 1;
			std::string displayName(layer, '\t');
			displayName.append(it->first.substr(it->first.find_last_of('/') + 1));
			float timeRatio = glm::clamp(it->second / averageFrameTime, 0.0, 1.0);
			ImGui::Text("%s \t %.3f ms %.2f%%", displayName.c_str(), it->second, timeRatio * 100);
			ImGui::ProgressBar(timeRatio, ImVec2(0.f, 5.f));
		}
		// profiling gpu
		for (auto &it = ScopedProfileTimer::timerMapGPU.begin(); it != ScopedProfileTimer::timerMapGPU.end(); ++it)
		{
			size_t layer = std::count(it->first.begin(), it->first.end(), '/') - 1;
			std::string displayName(layer, '\t');
			displayName.append(it->first.substr(it->first.find_last_of('/') + 1));
			float timeRatio = glm::clamp(it->second / averageFrameTime, 0.0, 1.0);
			ImGui::Text("%s \t %.3f ms %.2f%%", displayName.c_str(), it->second, timeRatio * 100);
			ImGui::ProgressBar(timeRatio, ImVec2(0.f, 5.f));
		}

		ImGui::End();
	}

	//{
	//	static bool open = true;
	//	ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
	//	ImGui::ShowTestWindow(&open);
	//}

	ImGui::Render();
}

void Render()
{
	GPU_SCOPED_PROFILE("render");

	const Viewpoint& mainViewpoint = gCamera.ProcessCamera((GLfloat)gWindowWidth, (GLfloat)gWindowHeight, 0.1f, 1000.f);

	glViewport(0, 0, gWindowWidth, gWindowHeight);
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
	
	// bind deferred pass textures
	gNormalTex.Bind(Shader::gNormalTexUnit);
	gAlbedoTex.Bind(Shader::gAlbedoTexUnit);
	gMaterialTex.Bind(Shader::gMaterialTexUnit);
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);
	
	LightPass(mainViewpoint);

	DebugForwardPass();

	// unbind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// find post process pass textures
	gSceneColorTex.Bind(Shader::gSceneColorTexUnit);
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);

	PostProcessPass();

	UIPass();
}

int main(int argc, char **argv)
{
	if (!Init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Main loop flag
		bool quit = false;

		gPerformanceCounter = SDL_GetPerformanceCounter();

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
					{
						LoadShaders(true);
						for (int i = 0; i < Mesh::gMeshContainer.size(); ++i)
							Mesh::gMeshContainer[i]->SetAttributes();
					}
				}
				else if (event.type == SDL_WINDOWEVENT)
				{
					switch (event.window.event)
					{
					case SDL_WINDOWEVENT_RESIZED:
						OnWindowResize(event.window.data1, event.window.data2);
						break;

					}
				}
			}

			Update(gLastDeltaTime);

			Render();

			SDL_GL_SwapWindow(gWindow);
			
			// time
			Uint64 newPerformanceCounter = SDL_GetPerformanceCounter();
			gLastDeltaTime = (float)((double)(newPerformanceCounter - gPerformanceCounter) * gInvPerformanceFreq);
			gPerformanceCounter = newPerformanceCounter;
			gDeltaTimeAccum = gDeltaTimeAccum - gDeltaTimeBuffer[gDeltaTimeBufferIdx] + gLastDeltaTime;
			gDeltaTimeBuffer[gDeltaTimeBufferIdx] = gLastDeltaTime;
			++gDeltaTimeBufferIdx;
			if (gDeltaTimeBufferIdx >= gDeltaTimeBufferCount) gDeltaTimeBufferIdx -= gDeltaTimeBufferCount;
			gAverageDeltaTime = gDeltaTimeAccum / gDeltaTimeBufferCount;

#if PROFILE
			{

				// profiling cpu
				for (auto it = ScopedProfileTimer::timerMapCPU.begin(); it != ScopedProfileTimer::timerMapCPU.end(); ++it)
				{
					// clear
					it->second = 0;
				}
				// profiling gpu
				int GPUMapReadIdx = 1 - ScopedProfileTimer::GPUMapWriteIdx;
				ScopedProfileTimer::GPUMapWriteIdx = GPUMapReadIdx;
				for (auto &it = ScopedProfileTimer::timeStampPairMapGPU[GPUMapReadIdx].begin(); it != ScopedProfileTimer::timeStampPairMapGPU[GPUMapReadIdx].end(); ++it)
				{
					double elapsed = 0;
					for (int pairIdx = 0; pairIdx < it->second.size(); ++pairIdx)
					{
						GLuint64 elapsedStart = 0, elapsedEnd = 0;
						glGetQueryObjectui64v(it->second[pairIdx][0], GL_QUERY_RESULT, &elapsedStart);
						glGetQueryObjectui64v(it->second[pairIdx][1], GL_QUERY_RESULT, &elapsedEnd);
						glDeleteQueries(2, glm::value_ptr(it->second[pairIdx]));
						elapsed += (double)(elapsedEnd - elapsedStart) / (double)1000000;
					}
					if (elapsed > 0)
					{
						// update
						if (ScopedProfileTimer::timerMapGPU[it->first] > 0)
							ScopedProfileTimer::timerMapGPU[it->first] = glm::mix(ScopedProfileTimer::timerMapGPU[it->first], elapsed, 0.2);
						else
							ScopedProfileTimer::timerMapGPU[it->first] = elapsed;
					}
					// clear
					it->second.clear();
				}
			}
#endif
		}
	}

	Close();

	return EXIT_SUCCESS;
}
