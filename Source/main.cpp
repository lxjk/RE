
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
#include "Engine/Bounds.h"

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

// settings
RenderSettings gRenderSettings;

// ubo
GLuint gUBO_Matrices = 0;

// frame buffer
GLuint gGBuffer = 0;
Texture2D gNormalTex, gAlbedoTex, gMaterialTex, gDepthStencilTex;

GLuint gSceneBuffer = 0;
Texture2D gSceneColorTex, gSceneDepthStencilTex;
GLuint gSceneDepthStencilRBO;

GLuint gDepthOnlyBuffer = 0;
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

	// setup shadow maps
	for (int i = 0; i < gDirectionalLights.size(); ++i)
	{
		for (int j = 0; j < MAX_CASCADE_COUNT; ++j)
		{
			Texture2D* shadowMap = Texture2D::Create();
			shadowMap->AllocateForFrameBuffer(1024, 1024, GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, true);
			gDirectionalLights[i].shadowData[j].shadowMap = shadowMap;
		}
	}

	assert(gDirectionalLights.size() <= MAX_DIRECTIONAL_LIGHT_COUNT);

	// point lights
	int plIdx = 0;
	//gPointLights.push_back(
	//	Light(Mesh::Create(&gIcosahedronMeshData, Material::Create(&gLightVolumeShader))));
	//plIdx = (int)gPointLights.size() - 1;
	//gPointLights[plIdx].SetPointLight(
	//	/*pos=*/	glm::vec3(0, 10, 20),
	//	/*radius=*/	4.f,
	//	/*color=*/	glm::vec3(1.f, 1.f, 1.f),
	//	/*int=*/	20
	//);

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
		meshComp->AddMesh(boxMesh);
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
		meshComp->AddMesh(sphereMesh);
		meshComp->SetPosition(glm::vec3(-20 + i * 2.5, 0, 7.5));
	}

	for (int i = 0; i < 20; ++i)
	{
		Mesh* sphereMesh = Mesh::Create(&gSphereMeshData, Material::Create(gGBufferMaterial));
		sphereMesh->material->SetParameter("metallic", i * 0.05f);
		sphereMesh->material->SetParameter("roughness", 0.4f);
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->AddMesh(sphereMesh);
		meshComp->SetPosition(glm::vec3(-20 + i * 2.5, 0, 12.5));
	}

	// nanosuit
	{
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->SetMeshList(gNanosuitMeshes);
		meshComp->SetPosition(glm::vec3(5, -1, 5));
		meshComp->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));

		const std::vector<Mesh*>& meshList = meshComp->GetMeshList();
		for (int i = 0; i < meshList.size(); ++i)
		{
			Material* material = meshList[i]->material;
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
		meshComp->AddMesh(floorMesh);
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
	if (!gDepthOnlyBuffer)
	{
		glGenFramebuffers(1, &gDepthOnlyBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gDepthOnlyBuffer);



		glDrawBuffer(GL_NONE);

		//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		//	printf("Error: Shadow buffer not complete!\n");

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
	// settings
	gRenderSettings.bDrawShadow = true;
	gRenderSettings.bDrawBounds = false;
	gRenderSettings.bDrawLightVolume = false;

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
	for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
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
	for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
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
	int shadowIdx = 0;
	Light* lightPtr = gDirectionalLights.data();
	for (int i = 0, ni = (int)gDirectionalLights.size(); i < ni; ++i)
	{
		Light& light = lightPtr[i];
		gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("directionRAB").c_str(), light.GetDirectionVSRAB(renderContext.viewPoint.viewMat));
		gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("color").c_str(), light.colorIntensity);
		gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("attenParams").c_str(), light.attenParams);
		// shadow
		if (gRenderSettings.bDrawShadow)
		{
			int shadowCount = MAX_CASCADE_COUNT;
			gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("shadowDataCount").c_str(), shadowCount);
			for (int j = 0; j < shadowCount; ++j)
			{
				gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("shadowData")[shadowIdx + j]("shadowMat").c_str(), light.shadowData[j].shadowMat);
				gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("shadowData")[shadowIdx + j]("farPlane").c_str(), light.shadowData[j].farPlane);
				gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("shadowMap")[shadowIdx + j].c_str(), light.shadowData[j].shadowMap);
			}
			shadowIdx += shadowCount;
		}
		else
		{
			gDirectionalLightMaterial->SetParameter(ShaderNameBuilder("lights")[i]("shadowDataCount").c_str(), 0);
		}

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

void ShadowPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("shadow");
	CPU_SCOPED_PROFILE("shadow");

	const static RenderState renderState([](RenderState& s) {
		// don't write color
		s.bColorWrite = false;
	});

	renderState.Apply();

	const static glm::mat4 remapMat(
		glm::vec4(0.5f, 0.f, 0.f, 0.0f),
		glm::vec4(0.f, 0.5f, 0.f, 0.0f),
		glm::vec4(0.f, 0.f, 0.5f, 0.0f),
		glm::vec4(0.5f, 0.5f, 0.5f, 1.f)
		);

	const static float cascadeRatio[MAX_CASCADE_COUNT] = {
		0.09f, 0.3f, 1.f
	};

	glm::vec3 clipPoints[4];

	Viewpoint& viewPoint = renderContext.viewPoint;
	
	float aspectRatio = viewPoint.height / viewPoint.width;
	float tanHF = glm::tan(viewPoint.fov * 0.5f) * sqrt(1 + aspectRatio * aspectRatio);
	float tanHF2 = tanHF * tanHF;

	MeshComponent** meshCompListPtr = MeshComponent::gMeshComponentContainer.data();

	Light* lightPtr = gDirectionalLights.data();
	for (int lightIdx = 0, nlightIdx = (int)gDirectionalLights.size(); lightIdx < nlightIdx; ++lightIdx)
	{
		Light& light = lightPtr[lightIdx];

		// calculate light space bounds
		for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
		{
			const glm::mat4& adjustMat = light.lightViewMat * meshCompListPtr[i]->modelMat;
			// tranform bounds into light space
			meshCompListPtr[i]->bounds.TransformBounds(adjustMat, meshCompListPtr[i]->boundsLS);
		}

		glm::mat4 viewToLight = light.lightViewMat * viewPoint.invViewMat;
		
		for (int cascadeIdx = 0; cascadeIdx < MAX_CASCADE_COUNT; ++cascadeIdx)
		{
			// near and far plane for this cascade
			float n = (cascadeIdx == 0) ? viewPoint.nearPlane : viewPoint.farPlane * cascadeRatio[cascadeIdx-1];
			float f = viewPoint.farPlane * cascadeRatio[cascadeIdx];

			// minimal bounding sphere diameter
			float diameter = ((f + n) * tanHF2 >= (f - n)) ?
				2 * f * tanHF :
				sqrt((f - n)*(f - n) + 2 * (f*f + n*n) * tanHF2 + (f + n)*(f + n)*tanHF2*tanHF2);

			float pixelRate = diameter / light.shadowData[cascadeIdx].shadowMap->width;

			// process frustum bounds
			BoxBounds frustumBounds;
			viewPoint.GetClipPoints(-n, clipPoints);
			for (int i = 0; i < 4; ++i)
				frustumBounds += light.lightViewMat * glm::vec4(clipPoints[i], 1);
			viewPoint.GetClipPoints(-f, clipPoints);
			for (int i = 0; i < 4; ++i)
				frustumBounds += light.lightViewMat * glm::vec4(clipPoints[i], 1);

			// extent test bound max(near plane) a little bit to include geometry behind us
			// this value need to be increased if we are missing shadow
			const float stepBack = 30.f;
			frustumBounds.max.z += stepBack;
			
			std::vector<MeshComponent*> involvedMeshComps;
			involvedMeshComps.reserve(MeshComponent::gMeshComponentContainer.size());

			// process scene bounds and do frustum culling
			BoxBounds sceneBounds;
			for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
			{
				// overlap test
				if (frustumBounds.IsOverlap(meshCompListPtr[i]->boundsLS))
				{
					sceneBounds += meshCompListPtr[i]->boundsLS;
					involvedMeshComps.push_back(meshCompListPtr[i]);
				}
			}
			
			// skip if we have no mesh to render
			if (involvedMeshComps.size() == 0)
				continue;

			// only change far plane if scene bounds are closer, don't extend it
			frustumBounds.min.z = glm::max(sceneBounds.min.z, frustumBounds.min.z);
			// for near plane simply use what scene bounds has,
			// since we need to include any mesh behind us (in light space) that will cast shadow
			frustumBounds.max.z = sceneBounds.max.z;

			// adjust based on diameter
			frustumBounds.min.x = floorf(frustumBounds.min.x / pixelRate) * pixelRate;
			frustumBounds.min.y = floorf(frustumBounds.min.y / pixelRate) * pixelRate;
			frustumBounds.max.x = frustumBounds.min.x + diameter;
			frustumBounds.max.y = frustumBounds.min.y + diameter;

			// the bounds are mapped as (+x, -x, +y, -y, +z, -z) -> (r, l, t, b, -n, -f)
			// ref: http://www.songho.ca/opengl/gl_projectionmatrix.html
			glm::mat4 lightProjMat = glm::ortho(
				frustumBounds.min.x, frustumBounds.max.x,
				frustumBounds.min.y, frustumBounds.max.y,
				-frustumBounds.max.z, -frustumBounds.min.z);

			light.shadowData[cascadeIdx].farPlane = f;
			light.shadowData[cascadeIdx].shadowMat = remapMat * lightProjMat * viewToLight;

			// attach to frame buffers
			Texture2D* shadowMap = light.shadowData[cascadeIdx].shadowMap;
			shadowMap->AttachToFrameBuffer(GL_DEPTH_ATTACHMENT);

			// set viewport
			glViewport(0, 0, shadowMap->width, shadowMap->height);

			// update ubo
			RenderInfo shadowRenderInfo;
			shadowRenderInfo.View = light.lightViewMat;
			shadowRenderInfo.Proj = lightProjMat;
			glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderInfo), &shadowRenderInfo, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			// clear depth
			glClearDepth(1);
			glClear(GL_DEPTH_BUFFER_BIT);

			// draw models
			MeshComponent** involvedMeshCompListPtr = involvedMeshComps.data();
			for (int i = 0, ni = (int)involvedMeshComps.size(); i < ni; ++i)
			{
				involvedMeshCompListPtr[i]->Draw(renderContext, gPrepassMaterial);
			}
		}
	}
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

	// wireframes
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);

	if (gRenderSettings.bDrawBounds)
	{
		// bounds
		MeshComponent** meshCompListPtr = MeshComponent::gMeshComponentContainer.data();
		for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
		{
			// draw bounds
			MeshComponent* meshComp = meshCompListPtr[i];
			glm::vec3 center = meshComp->bounds.GetCenter();
			glm::vec3 extent = meshComp->bounds.GetExtent();

			glm::mat4 boundMat(1);
			boundMat = glm::scale(boundMat, extent);
			boundMat[3] = glm::vec4(center, 1.f);

			glm::mat4 modelMat = meshComp->modelMat * boundMat;

			gLightDebugMaterial->SetParameter("modelMat", modelMat);
			gLightDebugMaterial->SetParameter("color", glm::vec3(1, 0, 0));

			gCubeMesh->Draw(renderContext, gLightDebugMaterial);
		}
	}


	if (gRenderSettings.bDrawLightVolume)
	{
		for (int i = 0; i < gPointLights.size(); ++i)
		{
			// model
			glm::mat4 modelMat(1);
			modelMat = glm::translate(modelMat, gPointLights[i].position);
			modelMat = glm::scale(modelMat, glm::vec3(gPointLights[i].radius));
			glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));

			gLightDebugMaterial->SetParameter("modelMat", modelMat);
			gLightDebugMaterial->SetParameter("color", gPointLights[i].colorIntensity);

			gPointLightDebugMesh->Draw(renderContext);

		}

		for (int i = 0; i < gSpotLights.size(); ++i)
		{
			// model
			glm::mat4 modelMat(1);
			modelMat = Math::MakeMatFromForward(gSpotLights[i].direction);
			modelMat[3] = glm::vec4(gSpotLights[i].position, 1);
			modelMat = glm::scale(modelMat, glm::vec3(gSpotLights[i].endRadius, gSpotLights[i].endRadius, gSpotLights[i].radius));
			glm::mat3 normalMat = glm::inverseTranspose(glm::mat3(modelMat));

			gLightDebugMaterial->SetParameter("modelMat", modelMat);
			gLightDebugMaterial->SetParameter("color", gSpotLights[i].colorIntensity);

			gSpotLightDebugMesh->Draw(renderContext);

		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

	{
		static bool open = true;
		ImGui::Begin("settings", &open);

		ImGui::Checkbox("shadow", &gRenderSettings.bDrawShadow);
		ImGui::Checkbox("bounds", &gRenderSettings.bDrawBounds);
		ImGui::Checkbox("light volume", &gRenderSettings.bDrawLightVolume);

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
	renderContext.viewPoint = gCamera.ProcessCamera((GLfloat)gWindowWidth, (GLfloat)gWindowHeight, 0.1f, 100.f);
	
	if (gRenderSettings.bDrawShadow)
	{
		// bind shadow buffer
		glBindFramebuffer(GL_FRAMEBUFFER, gDepthOnlyBuffer);
		ShadowPass(renderContext);
	}

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
