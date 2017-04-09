
// sdl
//#define SDL_MAIN_HANDLED
#include "sdl.h"
#include "SDL_image.h"

// glew
#include "gl/glew.h"

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
#include "Engine/ShaderLoader.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/MeshComponent.h"
#include "Engine/MeshLoader.h"
#include "Engine/Texture2D.h"
#include "Engine/Light.h"
#include "Engine/Viewpoint.h"
#include "Engine/Camera.h"
#include "Engine/Math.h"
#include "Engine/Profiler.h"
#include "Engine/Render.h"

// imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl.h"

int gWindowWidth = 1280;
int gWindowHeight = 720;

const int MAX_DIRECTIONAL_LIGHT_COUNT = 4;

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

// opengl context
SDL_GLContext gContext;

// shader file cache
std::unordered_map<std::string, ShaderInfo> gShaderFileCache;

// time
double gInvPerformanceFreq;
Uint64 gPerformanceCounter;
float gLastDeltaTime;
float gAverageDeltaTime;
float gDeltaTimeAccum;
float gDeltaTimeBuffer[20];
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

GLuint gShadowBuffer = 0;
Texture2D gShadowTex;

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
Material* gGBufferMaterial;
Material* gGBufferColorMaterial;
Material* gPrepassMaterial;
Material* gDirectionalLightMaterial;
Material* gLightVolumeMaterial;
Material* gLightDebugMaterial;
Material* gFSQuadMaterial;
Material* gToneMapMaterial;

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

Mesh* gDirectionalLightMesh;
Mesh* gPointLightMesh;
Mesh* gSpotLightMesh;

Mesh* gToneMapMesh;

Mesh* gPointLightDebugMesh;
Mesh* gSpotLightDebugMesh;

// texture
Texture2D* gDiffuseMap;
Texture2D* gNormalMap;

// light
std::vector<Light> gDirectionalLights;
std::vector<Light> gPointLights;
std::vector<Light> gSpotLights;

// camera
Camera gCamera;

// render info
RenderInfo gRenderInfo;

// input
bool gMouseCaptured = false;
float gMouseWheel;
const Uint8* gKeyStates;


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

	//Use Vsync (or do not?)
	if (SDL_GL_SetSwapInterval(0) < 0)
	{
		printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	}

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
	int dlIdx = 0;
	gDirectionalLights.push_back(Light(gDirectionalLightMesh));
	dlIdx = (int)gDirectionalLights.size() - 1;
	gDirectionalLights[dlIdx].SetDirectionLight(
		/*dir=*/	glm::vec3(0, -1, -2),
		/*color=*/	glm::vec3(1.f, 1.f, 1.f),
		/*int=*/	1
	);

	assert(gDirectionalLights.size() <= MAX_DIRECTIONAL_LIGHT_COUNT);

	// point lights
	int plIdx = 0;
	//gPointLights.push_back(Light());
	//plIdx = gPointLights.size() - 1;
	//gPointLights[plIdx].position = glm::vec3(0, 10, 10);
	//gPointLights[plIdx].color = glm::vec3(1.f, 1.f, 1.f);
	//gPointLights[plIdx].intensity = 1;
	//gPointLights[plIdx].radius = 20.f;

	gPointLights.push_back(
		Light(Mesh::Create(&gIcosahedronMeshData, Material::Create(&gLightVolumeShader))));
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	glm::vec3(0, 2, 3),
		/*radius=*/	4.f,
		/*color=*/	glm::vec3(1.f, 1.f, 1.f),
		/*int=*/	20
	);

	gPointLights.push_back(
		Light(Mesh::Create(&gIcosahedronMeshData, Material::Create(&gLightVolumeShader))));
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	glm::vec3(10, 2, 0),
		/*radius=*/	10.f,
		/*color=*/	glm::vec3(0.f, 1.f, 0.f),
		/*int=*/	20
	);

	gPointLights.push_back(
		Light(Mesh::Create(&gIcosahedronMeshData, Material::Create(&gLightVolumeShader))));
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	glm::vec3(-10, 3, 3),
		/*radius=*/	10.f,
		/*color=*/	glm::vec3(1.f, 0.f, 0.f),
		/*int=*/	20
	);

	//for (int i = 0; i < 100; ++i)
	//{
	//	gPointLights.push_back(
	//		Light(Mesh::Create(&gIcosahedronMeshData, Material::Create(&gLightVolumeShader))));
	//	plIdx = (int)gPointLights.size() - 1;
	//	gPointLights[plIdx].SetPointLight(
	//		/*pos=*/	glm::vec3(-10, 3, 3),
	//		/*radius=*/	10.f,
	//		/*color=*/	glm::vec3(1.f, 0.f, 0.f),
	//		/*int=*/	20
	//	);

	//}

	// spot lights
	int slIdx = 0;
	gSpotLights.push_back(
		Light(Mesh::Create(&gConeMeshData, Material::Create(&gLightVolumeShader))));
	slIdx = (int)gSpotLights.size() - 1;
	gSpotLights[slIdx].SetSpotLight(
		/*pos=*/	glm::vec3(0, 2, 10),
		/*dir=*/	glm::vec3(-2, -0.5f, -0.2f),
		/*radius=*/	12.f,
		/*hOuter=*/	30.f,
		/*hInner=*/	20.f,
		/*color=*/	glm::vec3(0.f, 0.f, 1.f),
		/*int=*/	100
	);
}

void MakeMeshComponents()
{

	// box
	Mesh* boxMesh = Mesh::Create(&gCubeMeshData, Material::Create(gGBufferMaterial));
	boxMesh->material->SetParameter("metallic", 1.0f);
	boxMesh->material->SetParameter("roughness", 0.3f);
	for (int i = 0; i < 3; ++i)
	{
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->meshList.push_back(boxMesh);
		meshComp->SetPosition(glm::vec3(-10 + i * 10, 0, 0));
		meshComp->SetRotation(glm::vec3(0, 45, 0));
		meshComp->SetScale(glm::vec3(1.5f, 1.f, 1.2f));
	}

	// sphere
	for (int i = 0; i < 20; ++i)
	{
		Mesh* sphereMesh = Mesh::Create(&gSphereMeshData, Material::Create(gGBufferMaterial));
		sphereMesh->material->SetParameter("metallic", 1.f);
		sphereMesh->material->SetParameter("roughness", i * 0.045f + 0.1f);
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->meshList.push_back(sphereMesh);
		meshComp->SetPosition(glm::vec3(-20 + i * 2.5, 0, 7.5));
	}

	for (int i = 0; i < 20; ++i)
	{
		Mesh* sphereMesh = Mesh::Create(&gSphereMeshData, Material::Create(gGBufferMaterial));
		sphereMesh->material->SetParameter("metallic", i * 0.05f);
		sphereMesh->material->SetParameter("roughness", 0.4f);
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->meshList.push_back(sphereMesh);
		meshComp->SetPosition(glm::vec3(-20 + i * 2.5, 0, 12.5));
	}

	// nanosuit
	{
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->meshList = gNanosuitMeshes;
		meshComp->SetPosition(glm::vec3(5, -1, 5));
		meshComp->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));

		for (int i = 0; i < meshComp->meshList.size(); ++i)
		{
			Material* material = meshComp->meshList[i]->material;
			if (!material)
				continue;

			material->SetParameter("metallic", 0.f);
			material->SetParameter("roughness", 0.3f);
		}
	}

	// floor
	{
		Mesh* floorMesh = Mesh::Create(&gCubeMeshData, Material::Create(&gGBufferColorShader));
		floorMesh->material->SetParameter("metallic", 0.f);
		floorMesh->material->SetParameter("roughness", 1.f);
		floorMesh->material->SetParameter("color", glm::vec3(0.2f));
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->meshList.push_back(floorMesh);
		meshComp->SetPosition(glm::vec3(0.f, -1.2f, 0.f));
		meshComp->SetScale(glm::vec3(32.f, 0.2f, 32.f));
	}
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

	// Shadow Buffer
	if (!gShadowBuffer)
	{
		glGenFramebuffers(1, &gShadowBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gShadowBuffer);

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

	// materials
	gGBufferMaterial = Material::Create(&gGBufferShader);
	gGBufferColorMaterial = Material::Create(&gGBufferColorShader);
	gPrepassMaterial = Material::Create(&gPrepassShader);
	gDirectionalLightMaterial = Material::Create(&gDirectionalLightShader);
	gLightVolumeMaterial = Material::Create(&gLightVolumeShader);
	gLightDebugMaterial = Material::Create(&gLightDebugShader);
	gFSQuadMaterial = Material::Create(&gFSQuadShader);
	gToneMapMaterial = Material::Create(&gToneMapShader);

	// mesh data
	MakeCube(gCubeMeshData);
	MakeSphere(gSphereMeshData, 32);
	MakeIcosahedron(gIcosahedronMeshData, 2);
	MakeCone(gConeMeshData, 16, 2);
	MakeQuad(gQuadMeshData);

	// mesh
	gCubeMesh = Mesh::Create(&gCubeMeshData, gGBufferMaterial);
	gSphereMesh = Mesh::Create(&gSphereMeshData, gGBufferMaterial);
	gFSQuadMesh = Mesh::Create(&gQuadMeshData, gFSQuadMaterial);

	LoadMesh(gNanosuitMeshes, "Content/Model/nanosuit/nanosuit.obj", &gGBufferShader);
	//LoadMesh(gNanosuitMeshes, "Content/Model/Lakecity/Lakecity.obj", &gGBufferShader);

	gDirectionalLightMesh = Mesh::Create(&gQuadMeshData, gDirectionalLightMaterial);
	gPointLightMesh = Mesh::Create(&gIcosahedronMeshData, gLightVolumeMaterial);
	gSpotLightMesh = Mesh::Create(&gConeMeshData, gLightVolumeMaterial);

	gToneMapMesh = Mesh::Create(&gQuadMeshData, gToneMapMaterial);

	gPointLightDebugMesh = Mesh::Create(&gIcosahedronMeshData, gLightDebugMaterial);
	gSpotLightDebugMesh = Mesh::Create(&gConeMeshData, gLightDebugMaterial);

	// texture
	gDiffuseMap = Texture2D::FindOrCreate("Content/Texture/154.jpg", true);
	gNormalMap = Texture2D::FindOrCreate("Content/Texture/154_norm.jpg", false);

	// set textures
	gGBufferMaterial->SetParameter("diffuseTex", gDiffuseMap);
	gGBufferMaterial->SetParameter("normalTex", gNormalMap);

	// light
	MakeLights();

	// mesh components
	MakeMeshComponents();
	
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

	float smoothDeltaTime = glm::min(0.03f, deltaTime);

	updateMouseInput(smoothDeltaTime);
	updateKeyboardInput(smoothDeltaTime);

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
		gSpotLights[0].SetDirection(glm::normalize(glm::mix(startDir, endDir, ratio)));
	}

	// end of frame
	MeshComponent** meshCompListPtr = MeshComponent::gMeshComponentContainer.data();
	for (int i = 0, ni = MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
	{
		meshCompListPtr[i]->UpdateEndOfFrame(deltaTime);
	}

	// update imgui
	ImGui_Impl_NewFrame(gWindow);
}

void GeometryPass(RenderContext& renderContext)
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
	MeshComponent** meshCompListPtr = MeshComponent::gMeshComponentContainer.data();
	for (int i = 0, ni = MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
	{
		meshCompListPtr[i]->Draw(renderContext);
	}
}

void DirectionalLightPass(RenderContext& renderContext)
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

	gDirectionalLightMaterial->SetParameter("lightCount", (int)gDirectionalLights.size());

	// set light
	for (int i = 0; i < gDirectionalLights.size(); ++i)
	{
		gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("directionRAB").c_str(), gDirectionalLights[i].GetDirectionVSRAB(renderContext.viewPoint.viewMat));
		gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("color").c_str(), gDirectionalLights[i].colorIntensity);
		gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("attenParams").c_str(), gDirectionalLights[i].attenParams);
		// shadow
		gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("csmCount").c_str(), 0);

	}

	// draw quad
	gDirectionalLightMesh->Draw(renderContext);
}

void LightVolumePass(RenderContext& renderContext, const std::vector<Light>& lights, const Mesh* lightVolumeMesh)
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

	std::vector<int> cameraInsideLight;
	cameraInsideLight.reserve(4);

	const Light* lightsPtr = lights.data();
	for (int i = 0, ni = (int)lights.size(); i < ni; i += 8)
	{
		// clear stencil
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);

		char cameraInsideFlag = 0;
		
		// prepass
		prepassRenderState.Apply();

		for (int j = 0; j < 8 && i + j < ni; ++j)
		{
			int lightIdx = i + j;
			char mask = 1 << j;

			const Light& light = lightsPtr[lightIdx];
			bool bSpot = (light.attenParams.y > 0);

			// camera in light volume?
			glm::vec3 lightToCamera = renderContext.viewPoint.position - light.position;
			bool isInLightVolume = (Math::SizeSquared(lightToCamera) <	light.radius * light.radius * 1.01f * 1.01f);
			if (bSpot)
			{
				isInLightVolume &= (glm::dot(glm::normalize(lightToCamera), light.direction) > light.outerCosHalfAngle - KINDA_SMALL_NUMBER);
			}

			if (isInLightVolume)
			{
				cameraInsideFlag |= mask;
				cameraInsideLight.push_back(lightIdx);
				continue;
			}

			// set mask
			glStencilFunc(prepassRenderState.stencilTestFunc, mask, mask);

			gPrepassMaterial->SetParameter("modelMat", light.modelMat);
			light.LightMesh->Draw(renderContext, gPrepassMaterial);
		}

		// draw light
		lightingRenderState.Apply();

		for (int j = 0; j < 8 && i + j < ni; ++j)
		{
			int lightIdx = i + j;
			char mask = 1 << j;

			const Light& light = lightsPtr[lightIdx];

			if (cameraInsideFlag & mask)
				continue;

			// set mask
			glStencilFunc(prepassRenderState.stencilTestFunc, mask, mask);

			light.LightMesh->material->SetParameter(ShaderNameBuilder("light")("positionInvR").c_str(), light.GetPositionVSInvR(renderContext.viewPoint.viewMat));
			light.LightMesh->material->SetParameter(ShaderNameBuilder("light")("directionRAB").c_str(), light.GetDirectionVSRAB(renderContext.viewPoint.viewMat));
			//light.LightMesh->material->SetParameter("", glm::vec4(0));
			light.LightMesh->material->SetParameter("modelMat", light.modelMat);

			light.LightMesh->Draw(renderContext);
		}
	}

	// draw camera inside lights
	onePassVolumeRenderState.Apply();
	for (int i = 0, ni = (int)cameraInsideLight.size(); i < ni; ++i)
	{
		const Light& light = lightsPtr[cameraInsideLight.data()[i]];

		light.LightMesh->material->SetParameter(ShaderNameBuilder("light")("positionInvR").c_str(), light.GetPositionVSInvR(renderContext.viewPoint.viewMat));
		light.LightMesh->material->SetParameter(ShaderNameBuilder("light")("directionRAB").c_str(), light.GetDirectionVSRAB(renderContext.viewPoint.viewMat));
		//light.LightMesh->material->SetParameter("", glm::vec4(0));
		light.LightMesh->material->SetParameter("modelMat", light.modelMat);

		light.LightMesh->Draw(renderContext);
	}

	glDisable(GL_BLEND);
}

void PointLightPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("point light");
	LightVolumePass(renderContext, gPointLights, gPointLightMesh);
}

void SpotLightPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("spot light");
	LightVolumePass(renderContext, gSpotLights, gSpotLightMesh);
}

void LightPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("light");
	
	// clear color buffer only.
	glClearColor(0, 0, 0, 0);
	//glClearDepth(1);
	//glClearStencil(0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);

	DirectionalLightPass(renderContext);
	PointLightPass(renderContext);
	SpotLightPass(renderContext);

}

void ShaderPass(RenderContext& renderContext)
{

}

void DebugForwardPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("debug foward");

	static const RenderState renderState;

	renderState.Apply();

	// draw debug
	for (int i = 0; i < gPointLights.size(); ++i)
	{
		glm::mat4 modelMat(1);
		modelMat = glm::translate(modelMat, gPointLights[i].position);
		modelMat = glm::scale(modelMat, glm::vec3(0.3f, 0.3f, 0.3f));
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		gLightDebugMaterial->SetParameter("modelMat", modelMat);
		gLightDebugMaterial->SetParameter("color", gPointLights[i].colorIntensity);

		gPointLightDebugMesh->Draw(renderContext);
	}

	for (int i = 0; i < gSpotLights.size(); ++i)
	{
		glm::mat4 modelMat(1);
		//modelMat = glm::translate(modelMat, gSpotLights[i].position);
		modelMat = Math::MakeMatFromForward(gSpotLights[i].direction);
		modelMat[3] = glm::vec4(gSpotLights[i].position, 1);
		modelMat = glm::scale(modelMat, glm::vec3(gSpotLights[i].endRadius, gSpotLights[i].endRadius, gSpotLights[i].radius) / gSpotLights[i].radius * 0.6f);
		glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));
		gLightDebugMaterial->SetParameter("modelMat", modelMat);
		gLightDebugMaterial->SetParameter("color", gSpotLights[i].colorIntensity);

		gSpotLightDebugMesh->Draw(renderContext);
	}

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glDisable(GL_CULL_FACE);

	//for (int i = 0; i < gPointLights.size(); ++i)
	//{
	//	// model
	//	glm::mat4 modelMat(1);
	//	modelMat = glm::translate(modelMat, gPointLights[i].position);
	//	modelMat = glm::scale(modelMat, glm::vec3(gPointLights[i].radius));
	//	glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));

	//	gLightDebugMaterial->SetParameter("modelMat", modelMat);
	//	gLightDebugMaterial->SetParameter("color", gPointLights[i].colorIntensity);

	//	gPointLightDebugMesh->Draw(renderContext);

	//}

	//for (int i = 0; i < gSpotLights.size(); ++i)
	//{
	//	// model
	//	glm::mat4 modelMat(1);
	//	modelMat = Math::MakeMatFromForward(gSpotLights[i].direction);
	//	modelMat[3] = glm::vec4(gSpotLights[i].position, 1);
	//	modelMat = glm::scale(modelMat, glm::vec3(gSpotLights[i].endRadius, gSpotLights[i].endRadius, gSpotLights[i].radius));
	//	glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));

	//	gLightDebugMaterial->SetParameter("modelMat", modelMat);
	//	gLightDebugMaterial->SetParameter("color", gSpotLights[i].colorIntensity);

	//	gSpotLightDebugMesh->Draw(renderContext);

	//}

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void PostProcessPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("post process");

	const static RenderState renderState( [] (RenderState& s) {
		// no depth test
		s.bDepthTest = false;
	});

	renderState.Apply();

	// draw quad
	gToneMapMesh->Draw(renderContext);
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
		ImGui::Text("CPU");
		for (auto it = ScopedProfileTimerCPU::timerMap.begin(); it != ScopedProfileTimerCPU::timerMap.end(); ++it)
		{
			size_t layer = std::count(it->first.begin(), it->first.end(), '/') - 1;
			std::string displayName(layer, '\t');
			displayName.append(it->first.substr(it->first.find_last_of('/') + 1));
			float timeRatio = (float)glm::clamp(it->second / averageFrameTime, 0.0, 1.0);
			ImGui::Text("%s \t %.3f ms %.2f%%", displayName.c_str(), it->second, timeRatio * 100);
			ImGui::ProgressBar(timeRatio, ImVec2(0.f, 5.f));
		}
		// profiling gpu
		ImGui::Text("GPU");
		for (auto &it = ScopedProfileTimerGPU::timerMap.begin(); it != ScopedProfileTimerGPU::timerMap.end(); ++it)
		{
			size_t layer = std::count(it->first.begin(), it->first.end(), '/') - 1;
			std::string displayName(layer, '\t');
			displayName.append(it->first.substr(it->first.find_last_of('/') + 1));
			float timeRatio = (float)glm::clamp(it->second / averageFrameTime, 0.0, 1.0);
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
	CPU_SCOPED_PROFILE("render");

	RenderContext renderContext;
	renderContext.viewPoint = gCamera.ProcessCamera((GLfloat)gWindowWidth, (GLfloat)gWindowHeight, 0.1f, 1000.f);

	glViewport(0, 0, gWindowWidth, gWindowHeight);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// update ubo
	gRenderInfo.View = renderContext.viewPoint.viewMat;
	gRenderInfo.InvView = renderContext.viewPoint.invViewMat;
	gRenderInfo.Proj = renderContext.viewPoint.projMat;
	gRenderInfo.Resolution.x = renderContext.viewPoint.width;
	gRenderInfo.Resolution.y = renderContext.viewPoint.height;
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	// maybe use glBufferSubData later?
	glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderInfo), &gRenderInfo, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	// bind G-buffer
	glBindFramebuffer(GL_FRAMEBUFFER, gGBuffer);

	GeometryPass(renderContext);

	// bind Scene-buffer
	glBindFramebuffer(GL_FRAMEBUFFER, gSceneBuffer);
	
	// bind deferred pass textures
	gNormalTex.Bind(Shader::gNormalTexUnit);
	gAlbedoTex.Bind(Shader::gAlbedoTexUnit);
	gMaterialTex.Bind(Shader::gMaterialTexUnit);
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);
	
	LightPass(renderContext);

	DebugForwardPass(renderContext);

	// unbind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// find post process pass textures
	gSceneColorTex.Bind(Shader::gSceneColorTexUnit);
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);

	PostProcessPass(renderContext);

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
						Material** materialContainerPtr = Material::gMaterialContainer.data();
						for (int i = 0, ni = (int)Material::gMaterialContainer.size(); i < ni; ++i)
							materialContainerPtr[i]->Reload();
						//Mesh** meshlContainerPtr = Mesh::gMeshContainer.data();
						//for (int i = 0, ni = (int)Mesh::gMeshContainer.size(); i < ni; ++i)
						//	meshlContainerPtr[i]->SetAttributes();
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
				ScopedProfileTimerCPU::Swap();
				// profiling gpu
				ScopedProfileTimerGPU::Swap();
			}
#endif
		}
	}

	Close();

	return EXIT_SUCCESS;
}
