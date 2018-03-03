
// sdl
//#define SDL_MAIN_HANDLED
#include "sdl.h"
#include "SDL_image.h"

// glew
#include "gl/glew.h"

// opengl
#include "SDL_opengl.h"

// math
#include "Math/REMath.h"

// containers
#include "Containers/Containers.h"

// std
#include <stdlib.h>
#include <stdio.h>
#include <functional>
#include <algorithm>

// local
#include "Engine/Shader.h"
#include "Engine/ShaderLoader.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/MeshComponent.h"
#include "Engine/MeshLoader.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2DArray.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureCubeArray.h"
#include "Engine/Light.h"
#include "Engine/Viewpoint.h"
#include "Engine/Camera.h"
#include "Engine/Profiler.h"
#include "Engine/Render.h"
#include "Engine/Bounds.h"
#include "Engine/FrameBuffer.h"
#include "Engine/FileWatcher.h"

// imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl.h"

// cache sime
#include "CacheSim.h"

#define SHADER_DEBUG_BUFFER 0
#define VISUALIZE_TEXTURE 0
#define LOAD_SCENE_MESH 1

#define DEBUG_SINGLE_LIGHT 0

#define LOAD_CACHE_SIM 0

int gWindowWidth = 1280;
int gWindowHeight = 720;

// the window we'll be rendering to
SDL_Window* gWindow = NULL;

// opengl context
SDL_GLContext gContext;

// shader file cache
std::unordered_map<std::string, ShaderFileInfo> gShaderFileCache;
FileWatcher fileWatcher;

// cache sim
#if LOAD_CACHE_SIM
CacheSim::DynamicLoader cachesim;
bool bCacheSimCaptureFrame = false;
#endif

// time
bool gHasResetFrame;
double gInvPerformanceFreq;
Uint64 gPerformanceCounter;
double gTime;
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
GLuint gUBO_GlobalLights = 0;

// ssbo
GLuint gSSBO_LocalLights = 0;
GLuint gSSBO_LocalLightsBounds = 0;
GLuint gSSBO_LocalLightsShadowMatrices = 0;
GLuint gSSBO_LightTileInfoData = 0;
GLuint gSSBO_LightTileCullingResultInfoData = 0;

int gPrevLocalLightCapacity = 0;
int gPrevLocalLightShadowMatCapacity = 0;
int gCurLocalLightShadowMatCount = 0;

// frame buffer
FrameBuffer gGBuffer;
Texture2D gNormalTex, gAlbedoTex, gMaterialTex, gVelocityTex, gDepthStencilTex;

FrameBuffer gSSAOBuffer;

FrameBuffer gSceneBuffer;
const int gSceneColorTexCount = 3;
const int gSceneDepthTexCount = 2;
Texture2D gSceneColorTex[gSceneColorTexCount], gSceneDepthStencilTex[gSceneDepthTexCount];
//Texture2D gSceneColorCompTex;
GLuint gSceneDepthStencilRBO;
int gSceneColorReadIdx, gSceneColorWriteIdx, gSceneColorHistoryIdx, gSceneColorPrevHistoryIdx;
int gSceneDepthCurrentIdx, gSceneDepthHistoryIdx;

FrameBuffer gDepthCopyBuffer;

FrameBuffer gDepthOnlyBuffer;
Texture2DArray gCSMTexArray;
TextureCubeArray gShadowCubeTexArray;
Texture2D gShadowTiledTex;

#if SHADER_DEBUG_BUFFER
Texture2D gDebugTex;
float* gDebugTexBuffer;
#endif

// render globals
REArray<MeshRenderData, 16> gOpaqueMeshRenderList;
REArray<MeshRenderData, 16> gAlphaBlendMeshRenderList;
REArray<LightRenderData> gVisibleLightList;

int gShadowCubeMapCount;

// light const
Matrix4 gLightTetrahedronViewMat[4];
Matrix4 gLightCubeViewMat[6];

// jitter
#define JITTER_HALTON 1
#if JITTER_HALTON
const static int gJitterCount = 16;
Vector4_2 gJitter[gJitterCount];
#else
const int gJitterCount = 4;
Vector4_2 gJitter[gJitterCount] =
{
	Vector4_2(-2.f / 16.f, -6.f / 16.f),
	Vector4_2(6.f / 16.f, -2.f / 16.f),
	Vector4_2(2.f / 16.f,  6.f / 16.f),
	Vector4_2(-6.f / 16.f,  2.f / 16.f)
};
#endif
int gJitterIdx = 0;

// shader
Shader gGBufferShader;
Shader gAlphaBlendBasicShader;
Shader gPrepassShader;
Shader gPrepassTiledShader;
Shader gPrepassCubeShader;
Shader gPrepassTetrahedronShader;
Shader gTiledDeferredLightShader;
Shader gDirectionalLightShader;
Shader gLightVolumePointCubeShader;
Shader gLightVolumePointTetrahedronShader;
Shader gLightVolumeSpotShader;
Shader gLightVolumeFogShader;
Shader gLightDebugShader;
Shader gSkyboxShader;
Shader gSSAOShader;
Shader gSSRShader;
Shader gSSRStencilShader;
Shader gToneMapShader;
Shader gTAAShader;
Shader gVisualizeTextureShader;

Shader gTileBasedDeferredLightCompShader;
Shader gLightTileReductionCompShader;
Shader gLightTileCullingCompShader;
Shader gLightTileReductionAndCullingCompShader;

// material
Material* gGBufferMaterial;
Material* gAlphaBlendBasicMaterial;
Material* gPrepassMaterial;
Material* gPrepassTiledMaterial;
Material* gPrepassCubeMaterial;
Material* gPrepassTetrahedronMaterial;
Material* gTiledDeferredLightMaterial;
Material* gDirectionalLightMaterial;
Material* gLightVolumePointCubeMaterial;
Material* gLightVolumePointTetrahedronMaterial;
Material* gLightVolumeSpotMaterial;
Material* gLightDebugMaterial;
Material* gSkyboxMaterial;
Material* gSSAOMaterial;
Material* gSSRMaterial;
Material* gSSRStencilMaterial;
Material* gToneMapMaterial;
Material* gTAAMaterial;
Material* gVisualizeTextureMaterial;

Material* gTileBasedDeferredLightCompMaterial;
Material* gLightTileReductionCompMaterial;
Material* gLightTileCullingCompMaterial;
Material* gLightTileReductionAndCullingCompMaterial;

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
Mesh* gIcosahedronMesh;
Mesh* gConeMesh;
REArray<Mesh*> gNanosuitMeshes;
REArray<Mesh*> gSceneMeshes;

// texture
Texture2D* gDiffuseMap;
Texture2D* gNormalMap;

Texture2D* gFloorDiffuseMap;
Texture2D* gFloorNormalMap;

TextureCube* gSkyboxMap;

// light
REArray<Light> gDirectionalLights;
REArray<Light> gPointLights;
REArray<Light> gSpotLights;

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
	if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) == 0)
	{
		printf("SDL_Image could not initialize! SDL_IMAGE_Error: %s\n", IMG_GetError());
		return false;
	}

	// use opengl 4.3 core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	// no need for alpha and depth/stencil
	//SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	//SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// create window with opengl
	gWindow = SDL_CreateWindow("RE",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, gWindowWidth, gWindowHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
	if (gWindow == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

	// create logo
	SDL_Surface* icon = IMG_Load("Content/Logo/reicon_white.png");
	if (icon)
	{
		SDL_SetWindowIcon(gWindow, icon);
		SDL_FreeSurface(icon);
		icon = 0;
	}

	// Get window surface 
	SDL_Surface* windowSurface = SDL_GetWindowSurface(gWindow);
	if (windowSurface == NULL)
		printf("Window surface could not be created! SDL_Error: %s\n", SDL_GetError());

	// Create splash screen surface 
	SDL_Surface* splash = IMG_Load("Content/Logo/relogo_white.png");
	if (splash)
	{
		SDL_Rect rect;
		rect.w = splash->w;
		rect.h = splash->h;
		rect.x = (gWindowWidth - rect.w) / 2;
		rect.y = (gWindowHeight - rect.h) / 2;

		SDL_BlitScaled(splash, NULL, windowSurface, &rect);
		SDL_UpdateWindowSurface(gWindow);

		SDL_FreeSurface(splash);
		splash = 0;
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
	io.Fonts->AddFontFromFileTTF("Content/Fonts/DroidSans.ttf", 20.0f);

	//Use Vsync (or do not?)
	if (SDL_GL_SetSwapInterval(0) < 0)
	{
		printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	}
	
	// init cache sim
#if LOAD_CACHE_SIM
	if (!cachesim.Init())
	{
		printf("Warning: Cache sim failed to init.\n");
	}
	else
	{
		cachesim.SetThreadCoreMapping(cachesim.GetCurrentThreadId(), 0);
	}
#endif

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
	gTime = 0;
	gLastDeltaTime = 0;
	gAverageDeltaTime = 0.03f;
	gDeltaTimeAccum = 0.f;
	gDeltaTimeBufferCount = _countof(gDeltaTimeBuffer);
	memset(gDeltaTimeBuffer, 0, sizeof(float) * gDeltaTimeBufferCount);
	gDeltaTimeBufferIdx = 0;

	if(windowSurface)
	{
		SDL_FreeSurface(windowSurface);
		windowSurface = 0;
	}

	return true;
}

void MakeLights()
{
	// directional lights
	int dlIdx = 0;
	gDirectionalLights.push_back(Light(0));
	dlIdx = (int)gDirectionalLights.size() - 1;
	gDirectionalLights[dlIdx].SetDirectionLight(
#if LOAD_SCENE_MESH
		/*dir=*/	Vector4_3(-0.8f, 2.f, -5.f),
#else
		/*dir=*/	Vector4_3(-0.8f, 2.f, -1.f),
#endif
		/*color=*/	Vector4_3(1.f, 1.f, 1.f),
		/*int=*/	1
	);
	gDirectionalLights[dlIdx].bCastShadow = true;

	assert(gDirectionalLights.size() <= RenderConsts::MAX_DIRECTIONAL_LIGHT_COUNT);

	// point lights
	int plIdx = 0;
	gPointLights.push_back(
		Light(gIcosahedronMesh));
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	Vector4_3(-10, -3, 3),
		/*radius=*/	10.f,
		/*color=*/	Vector4_3(1.f, 0.f, 0.f),
		/*int=*/	200
	);
	gPointLights[plIdx].bCastShadow = true;

#if !DEBUG_SINGLE_LIGHT
	gPointLights.push_back(
		Light(gIcosahedronMesh));
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	Vector4_3(0, -3, 2),
		/*radius=*/	4.f,
		/*color=*/	Vector4_3(1.f, 1.f, 1.f),
		/*int=*/	200
	);
	gPointLights[plIdx].bCastShadow = true;

	gPointLights.push_back(
		Light(gIcosahedronMesh));
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	Vector4_3(10, -2, 2),
		/*radius=*/	10.f,
		/*color=*/	Vector4_3(0.f, 1.f, 0.f),
		/*int=*/	20
	);
	gPointLights[plIdx].bCastShadow = true;
	//gPointLights[plIdx].bVolumetricFog = true;


	gPointLights.push_back(
		Light(gIcosahedronMesh));
	plIdx = (int)gPointLights.size() - 1;
	gPointLights[plIdx].SetPointLight(
		/*pos=*/	Vector4_3(-94, -4, 15),
		/*radius=*/	30.f,
		/*color=*/	Vector4_3(1.f, 0.f, 0.f),
		/*int=*/	80
	);
	gPointLights[plIdx].bCastShadow = true;

	for (int i = 0; i < 10; ++i)
	{
		gPointLights.push_back(
			Light(gIcosahedronMesh));
		plIdx = (int)gPointLights.size() - 1;
		gPointLights[plIdx].SetPointLight(
			/*pos=*/	Vector4_3(RandRange(-10.f, 10.f), RandRange(-15.f, 5.f), 3),
			/*radius=*/	10.f,
			/*color=*/	Vector4_3(1.f, 0.f, 0.f),
			/*int=*/	20
		);
		gPointLights[plIdx].bCastShadow = true;
	}


	for (int i = 0; i < 1000; ++i)
	{
		gPointLights.push_back(
			Light(gIcosahedronMesh));
		plIdx = (int)gPointLights.size() - 1;
		gPointLights[plIdx].SetPointLight(
			/*pos=*/	Vector4_3(RandRange(-50.f, 50.f), RandRange(-35.f, 25.f), RandRange(0.f, 50.f)),
			/*radius=*/	6.f,
			/*color=*/	Vector4_3(RandRange(0.f, 1.f), RandRange(0.f, 1.f), RandRange(0.f, 1.f)),
			/*int=*/	20
		);
		//gPointLights[plIdx].bCastShadow = true;
	}

	// spot lights
	int slIdx = 0;
	gSpotLights.push_back(
		Light(gConeMesh));
	slIdx = (int)gSpotLights.size() - 1;
	gSpotLights[slIdx].SetSpotLight(
		/*pos=*/	Vector4_3(0, -10, 3),
		/*dir=*/	Vector4_3(-2, 0.2f, -0.5f),
		/*radius=*/	20.f,
		/*hOuter=*/	45.f,
		/*hInner=*/	20.f,
		/*color=*/	Vector4_3(0.f, 0.f, 1.f),
		/*int=*/	1000
	);
	gSpotLights[slIdx].bCastShadow = true;
	//gSpotLights[slIdx].bVolumetricFog = true;
#endif
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
		meshComp->SetPosition(Vector4_3(-10.f + i * 10.f, 0.f, 0.f));
		meshComp->SetRotation(Vector4_3(0.f, 0.f, 45.f));
		meshComp->SetScale(Vector4_3(1.5f, 1.2f, 1.f));
	}

	// sphere
	for (int i = 0; i < 20; ++i)
	{
		Mesh* sphereMesh = Mesh::Create(&gSphereMeshData, Material::Create(gGBufferMaterial));
		sphereMesh->material->SetParameter("metallic", 1.f);
		sphereMesh->material->SetParameter("roughness", i * 0.9f / 19.f + 0.1f);
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->AddMesh(sphereMesh);
		meshComp->SetPosition(Vector4_3(-20.f + i * 2.5f, -7.5f, 0.f));
	}

	for (int i = 0; i < 20; ++i)
	{
		Mesh* sphereMesh = Mesh::Create(&gSphereMeshData, Material::Create(gGBufferMaterial));
		sphereMesh->material->SetParameter("metallic", i * 0.05f);
		sphereMesh->material->SetParameter("roughness", 0.4f);
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->AddMesh(sphereMesh);
		meshComp->SetPosition(Vector4_3(-20.f + i * 2.5f, -12.5f, 0.f));
	}

	// nanosuit
	{
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->SetMeshList(gNanosuitMeshes);
		meshComp->SetPosition(Vector4_3(5, -5, -1));
		meshComp->SetScale(Vector4_3(0.3f, 0.3f, 0.3f));

		const REArray<Mesh*>& meshList = meshComp->GetMeshList();
		for (int i = 0; i < meshList.size(); ++i)
		{
			Material* material = meshList[i]->material;
			if (!material)
				continue;

			material->SetParameter("metallic", 0.5f);
			material->SetParameter("roughness", 0.2f);
		}

		//for (int i = 0; i < gNanosuitMeshes.size(); ++i)
		//{
		//	MeshComponent* meshComp = MeshComponent::Create();
		//	meshComp->AddMesh(gNanosuitMeshes[i]);
		//	meshComp->SetPosition(Vector4_3(5, -5, -1));
		//	meshComp->SetScale(Vector4_3(0.3f, 0.3f, 0.3f));

		//	Material* material = gNanosuitMeshes[i]->material;
		//	if (!material)
		//		continue;

		//	material->SetParameter("metallic", 0.f);
		//	material->SetParameter("roughness", 0.3f);
		//}
	}

	// scene
#if LOAD_SCENE_MESH
	{
		//MeshComponent* meshComp = MeshComponent::Create();
		//meshComp->SetMeshList(gSceneMeshes);
		//meshComp->SetPosition(Vector4_3(0, -6, -1));
		//meshComp->SetScale(Vector4_3(1.f, 1.f, 1.f) * 0.07f);

		//const REArray<Mesh*>& meshList = meshComp->GetMeshList();
		//for (int i = 0; i < meshList.size(); ++i)
		//{
		//	Material* material = meshList[i]->material;
		//	if (!material)
		//		continue;

		//	material->SetParameter("metallic", 0.f);
		//	material->SetParameter("roughness", 1.f);
		//}

		for (int i = 0; i < gSceneMeshes.size(); ++i)
		{
			MeshComponent* meshComp = MeshComponent::Create();
			meshComp->AddMesh(gSceneMeshes[i]);
			meshComp->SetPosition(Vector4_3(0, -6, -1));
			meshComp->SetScale(Vector4_3(1.f, 1.f, 1.f) * 0.07f);

			Material* material = gSceneMeshes[i]->material;
			if (!material)
				continue;

			material->SetParameter("metallic", 0.f);
			material->SetParameter("roughness", 1.f);
		}
	}
#endif

	// floor
	{
		Mesh* floorMesh = Mesh::Create(&gCubeMeshData, Material::Create(gGBufferMaterial));
		floorMesh->material->SetParameter("diffuseTex", gFloorDiffuseMap);
		floorMesh->material->SetParameter("normalTex", gFloorNormalMap);
		floorMesh->material->SetParameter("tile", Vector4(32, 32, 0, 0), 4);
		floorMesh->material->SetParameter("metallic", 0.f);
		floorMesh->material->SetParameter("roughness", 0.3f);
		//floorMesh->material->SetParameter("tintColor", Vector4(0.2f, 0.2f, 0.2f, 1.f), 4);
		MeshComponent* meshComp = MeshComponent::Create();
		meshComp->AddMesh(floorMesh);
		meshComp->SetPosition(Vector4_3(0.f, 0.f, -1.2f));
		meshComp->SetScale(Vector4_3(32.f, 32.f, 0.2f));
	}

	//{
	//	Mesh* floorMesh = Mesh::Create(&gCubeMeshData, Material::Create(gGBufferMaterial));
	//	floorMesh->material->SetParameter("hasDiffuseTex", 0);
	//	floorMesh->material->SetParameter("hasNormalTex", 0);
	//	floorMesh->material->SetParameter("metallic", 0.f);
	//	floorMesh->material->SetParameter("roughness", 0.f);
	//	floorMesh->material->SetParameter("color", Vector4_3(0.2f));
	//	MeshComponent* meshComp = MeshComponent::Create();
	//	meshComp->AddMesh(floorMesh);
	//	meshComp->SetPosition(Vector4_3(-21.2f, 0.f, 0.f));
	//	meshComp->SetScale(Vector4_3(0.2f, 32.f, 5.f));
	//}
}

void AllocateRenderTarget(bool bCreate)
{
	if (bCreate)
	{

#if SHADER_DEBUG_BUFFER
		gDebugTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, true);
		gDebugTexBuffer = new float[gWindowWidth * gWindowHeight * 4];
#endif

		// == GBuffer ==
		// normal
		//gNormalTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGB16F, GL_RGB, GL_FLOAT);
		gNormalTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_BYTE);
		// albedo
		//gAlbedoTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
		gAlbedoTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
		// material: metallic + roughness + ? + ao
		gMaterialTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
		// velocity
		gVelocityTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RG16, GL_RG, GL_UNSIGNED_BYTE);
		// depth stencil
		gDepthStencilTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		
		// == Scene Buffer ==
		// HDR scene color
		for (int i = 0; i < gSceneColorTexCount; ++i)
		{
			gSceneColorTex[i].AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGBA16F, GL_RGBA, GL_FLOAT, true);
		}
		//gSceneColorCompTex.AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_RGBA16F, GL_RGBA, GL_FLOAT, true);
		// depth stencil
		//for (int i = 0; i < gSceneDepthTexCount; ++i)
		//{
		//	gSceneDepthStencilTex[i].AllocateForFrameBuffer(gWindowWidth, gWindowHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		//}
		// depth stencil rbo
		glGenRenderbuffers(1, &gSceneDepthStencilRBO);
		glBindRenderbuffer(GL_RENDERBUFFER, gSceneDepthStencilRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gWindowWidth, gWindowHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	else
	{

#if SHADER_DEBUG_BUFFER
		gDebugTex.Reallocate(gWindowWidth, gWindowHeight);
		delete[] gDebugTexBuffer;
		gDebugTexBuffer = new float[gWindowWidth * gWindowHeight * 4];
#endif

		// normal
		gNormalTex.Reallocate(gWindowWidth, gWindowHeight);
		// albedo
		gAlbedoTex.Reallocate(gWindowWidth, gWindowHeight);
		// material: metallic + roughness + ? + ao
		gMaterialTex.Reallocate(gWindowWidth, gWindowHeight);
		// velocity
		gVelocityTex.Reallocate(gWindowWidth, gWindowHeight);
		// depth stencil
		gDepthStencilTex.Reallocate(gWindowWidth, gWindowHeight);

		// HDR scene color
		for (int i = 0; i < gSceneColorTexCount; ++i)
		{
			gSceneColorTex[i].Reallocate(gWindowWidth, gWindowHeight);
		}
		//gSceneColorCompTex.Reallocate(gWindowWidth, gWindowHeight);
		// depth stencil
		//for (int i = 0; i < gSceneDepthTexCount; ++i)
		//{
		//	gSceneDepthStencilTex[i].Reallocate(gWindowWidth, gWindowHeight);
		//}
		// depth stencil rbo
		glBindRenderbuffer(GL_RENDERBUFFER, gSceneDepthStencilRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gWindowWidth, gWindowHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
}

void SetupFrameBuffers()
{
	// G-Buffer
	{
		gGBuffer.StartSetup();

		Texture* colors[] =
		{
			&gNormalTex,		// normal(RGB)
			&gAlbedoTex,		// color(RGB)
			&gMaterialTex,		// matellic(R) + roughness(B) + AO(A)
			&gVelocityTex,
		};
		gGBuffer.SetupColors(colors, _countof(colors));
		gGBuffer.SetupDepth(&gDepthStencilTex, true);
		gGBuffer.FinishSetup();

		gGBuffer.TestComplete();
	}

	// SSAO Buffer
	{
		gSSAOBuffer.StartSetup();
		gSSAOBuffer.SetupColor(&gMaterialTex);
		gSSAOBuffer.FinishSetup();
	}

	// Scene Buffer
	{
		gSceneBuffer.StartSetup();
		gSceneBuffer.SetupColor(0); // reserve a scene color tex
		//gSceneBuffer.SetupColor(&gDebugTex); // debug
		gSceneBuffer.SetupDepth(gSceneDepthStencilRBO, true);
		gSceneBuffer.FinishSetup();
	}

	// depth copy buffer
	{
		gDepthCopyBuffer.StartSetup();
		gDepthCopyBuffer.SetupDepth(gSceneDepthStencilRBO, true);
		gDepthCopyBuffer.FinishSetup();
	}

	// Shadow Buffer
	{
		gDepthOnlyBuffer.StartSetup();
		gDepthOnlyBuffer.FinishSetup();
		gShadowTiledTex.AllocateForFrameBuffer(4096, 4096, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, true);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LoadShaders(bool bReload)
{
	// clear cache
	gShaderFileCache.clear();

	gGBufferShader.Load("Shader/gbuffer.vert", "Shader/gbuffer.frag", !bReload);
	gAlphaBlendBasicShader.Load("Shader/alphaBlendBasic.vert", "Shader/alphaBlendBasic.frag", !bReload);
	gPrepassShader.Load("Shader/prepass.vert", "Shader/prepass.frag", !bReload);
	gPrepassTiledShader.Load("Shader/prepassTiled.vert", "Shader/prepass.frag", !bReload);
	gPrepassCubeShader.Load("Shader/prepass.vert", "Shader/prepassCube.geom", "Shader/prepass.frag", !bReload);
	gPrepassTetrahedronShader.Load("Shader/prepass.vert", "Shader/prepassTetrahedron.geom", "Shader/prepass.frag", !bReload);
	gTiledDeferredLightShader.Load("Shader/fsQuad.vert", "Shader/fsQuadTiledLight.frag", !bReload);
	gDirectionalLightShader.Load("Shader/fsQuad.vert", "Shader/fsQuadLight.frag", !bReload);
	gLightVolumePointCubeShader.Load("Shader/lightVolume.vert", "Shader/lightVolumePointCube.frag", !bReload);
	gLightVolumePointTetrahedronShader.Load("Shader/lightVolume.vert", "Shader/lightVolumePointTetrahedron.frag", !bReload);
	gLightVolumeSpotShader.Load("Shader/lightVolume.vert", "Shader/lightVolumeSpot.frag", !bReload);
	gLightVolumeFogShader.Load("Shader/lightVolume.vert", "Shader/lightVolumeFog.frag", !bReload);
	gLightDebugShader.Load("Shader/test.vert", "Shader/lightDebug.frag", !bReload);
	gSkyboxShader.Load("Shader/skybox.vert", "Shader/skybox.frag", !bReload);
	gSSAOShader.Load("Shader/fsQuad.vert", "Shader/fsQuadSSAO.frag", !bReload);
	gSSRShader.Load("Shader/fsQuad.vert", "Shader/fsQuadSSR.frag", !bReload);
	gSSRStencilShader.Load("Shader/fsQuad.vert", "Shader/fsQuadSSRStencil.frag", !bReload);
	gToneMapShader.Load("Shader/fsQuad.vert", "Shader/fsQuadToneMap.frag", !bReload);
	gTAAShader.Load("Shader/fsQuad.vert", "Shader/fsQuadTAA.frag", !bReload);
	gVisualizeTextureShader.Load("Shader/fsQuad.vert", "Shader/fsQuadVisualizeTexture.frag", !bReload);

	gTileBasedDeferredLightCompShader.Load("Shader/tileBasedDeferredLight.comp", !bReload);
	gLightTileReductionCompShader.Load("Shader/lightTileReduction.comp", !bReload);
	gLightTileCullingCompShader.Load("Shader/lightTileCulling.comp", !bReload);
	gLightTileReductionAndCullingCompShader.Load("Shader/lightTileReductionAndCulling.comp", !bReload);
}

float HaltonSeq(int prime, int index = 1)
{
	float r = 0.f;
	float f = 1.f;
	int i = index;
	while (i > 0)
	{
		f /= prime;
		r += f * (i % prime);
		i = (int)floor(i / (float)prime);
	}
	return r;
}

void InitializeHalton_2_3()
{
	for (int i = 0; i < gJitterCount; ++i)
	{
		gJitter[i].x = HaltonSeq(2, i + 1) - 0.5f;
		gJitter[i].y = HaltonSeq(3, i + 1) - 0.5f;
	}
}

void InitializeLightOmniViewMat()
{
	// tetrahedron
	{
		const float a = 0.457745343f;//0.57735026f;
		const float b = 0.889066577f;//0.81649661f;
		// in view space
		const Vector4_3 omniDirs[4] =
		{
			Vector4_3(0.f, -a, -b),	// forward
			Vector4_3(0.f, -a, b),	// backward
			Vector4_3(b, a, 0.f),	// right
			Vector4_3(-b, a, 0.f)	// left
		};

		for (int i = 0; i < 4; ++i)
		{
			gLightTetrahedronViewMat[i] = MakeMatrixFromViewForward(omniDirs[i]).GetTransposed3();
		}
	}

	// cube
	{
		// in view space
		const Vector4_3 omniDirs[6][2] =
		{
			{ Vector4_3(1, 0, 0),	Vector4_3(0, -1, 0) },	// right (+x)
			{ Vector4_3(-1, 0, 0),	Vector4_3(0, -1, 0) },	// left (-x)
			{ Vector4_3(0, 1, 0),	Vector4_3(0, 0, 1) },	// top (+y)
			{ Vector4_3(0, -1, 0),	Vector4_3(0, 0, -1) },	// bottom (-y)
			{ Vector4_3(0, 0, 1),	Vector4_3(0, -1, 0) },	// backward (+z)
			{ Vector4_3(0, 0, -1),	Vector4_3(0, -1, 0) }	// forward (-z)
		};
		for (int i = 0; i < 6; ++i)
		{
			gLightCubeViewMat[i] = MakeMatrixFromViewForward(omniDirs[i][0], omniDirs[i][1]).GetTransposed3();
		}
	}
}

bool InitEngine()
{
	gHasResetFrame = true;

#if JITTER_HALTON
	InitializeHalton_2_3();
#endif
	InitializeLightOmniViewMat();

	// frame buffers
	AllocateRenderTarget(true);
	SetupFrameBuffers();
	// scene buffer idx
	gSceneColorWriteIdx = 0;
	gSceneColorReadIdx = 1;
	gSceneColorHistoryIdx = 2;
	gSceneColorPrevHistoryIdx = -1;
	gSceneDepthCurrentIdx = 0;
	gSceneDepthHistoryIdx = 1;

	// ubo
	// matrices
	glGenBuffers(1, &gUBO_Matrices);
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderInfo), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, Shader::RenderInfoBP, gUBO_Matrices);
	// global lights
	glGenBuffers(1, &gUBO_GlobalLights);
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_GlobalLights);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(GlobalLightsRenderInfo), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, Shader::GlobalLightsRenderInfoBP, gUBO_GlobalLights);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// ssbo
	glGenBuffers(1, &gSSBO_LocalLights);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LocalLights);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, Shader::LocalLightsRenderInfoBP, gSSBO_LocalLights);

	glGenBuffers(1, &gSSBO_LocalLightsBounds);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LocalLightsBounds);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, Shader::LocalLightsCullingInfoBP, gSSBO_LocalLightsBounds);

	glGenBuffers(1, &gSSBO_LocalLightsShadowMatrices);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LocalLightsShadowMatrices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, Shader::LocalLightsShadowMatrixInfoBP, gSSBO_LocalLightsShadowMatrices);

	glGenBuffers(1, &gSSBO_LightTileInfoData);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LightTileInfoData);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, Shader::LightTileInfoBP, gSSBO_LightTileInfoData);

	glGenBuffers(1, &gSSBO_LightTileCullingResultInfoData);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LightTileCullingResultInfoData);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, Shader::LightTileCullingResultInfoBP, gSSBO_LightTileCullingResultInfoData);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	
	// shader
	LoadShaders(false);
	fileWatcher.Start("Shader");
	fileWatcher.Start("Shader/Include");

	// materials
	gGBufferMaterial = Material::Create(&gGBufferShader);
	gAlphaBlendBasicMaterial = Material::Create(&gAlphaBlendBasicShader);
	gPrepassMaterial = Material::Create(&gPrepassShader);
	gPrepassTiledMaterial = Material::Create(&gPrepassTiledShader);
	gPrepassCubeMaterial = Material::Create(&gPrepassCubeShader);
	gPrepassTetrahedronMaterial = Material::Create(&gPrepassTetrahedronShader);
	gTiledDeferredLightMaterial = Material::Create(&gTiledDeferredLightShader);
	gDirectionalLightMaterial = Material::Create(&gDirectionalLightShader);
	gLightVolumePointCubeMaterial = Material::Create(&gLightVolumePointCubeShader);
	gLightVolumePointTetrahedronMaterial = Material::Create(&gLightVolumePointTetrahedronShader);
	gLightVolumeSpotMaterial = Material::Create(&gLightVolumeSpotShader);
	gLightDebugMaterial = Material::Create(&gLightDebugShader);
	gSkyboxMaterial = Material::Create(&gSkyboxShader);
	gSSAOMaterial = Material::Create(&gSSAOShader);
	gSSRMaterial = Material::Create(&gSSRShader);
	gSSRStencilMaterial = Material::Create(&gSSRStencilShader);
	gToneMapMaterial = Material::Create(&gToneMapShader);
	gTAAMaterial = Material::Create(&gTAAShader);
	gVisualizeTextureMaterial = Material::Create(&gVisualizeTextureShader);

	gTileBasedDeferredLightCompMaterial = Material::Create(&gTileBasedDeferredLightCompShader);
	gLightTileReductionCompMaterial = Material::Create(&gLightTileReductionCompShader);
	gLightTileCullingCompMaterial = Material::Create(&gLightTileCullingCompShader);
	gLightTileReductionAndCullingCompMaterial = Material::Create(&gLightTileReductionAndCullingCompShader);

	// mesh data
	MakeCube(gCubeMeshData);
	MakeSphere(gSphereMeshData, 32);
	MakeIcosahedron(gIcosahedronMeshData, 2);
	MakeCone(gConeMeshData, 16, 2);
	MakeQuadVS(gQuadMeshData);

	// mesh
	gCubeMesh = Mesh::Create(&gCubeMeshData, gGBufferMaterial);
	gSphereMesh = Mesh::Create(&gSphereMeshData, gGBufferMaterial);

	gFSQuadMesh = Mesh::Create(&gQuadMeshData);
	gIcosahedronMesh = Mesh::Create(&gIcosahedronMeshData);
	gConeMesh = Mesh::Create(&gConeMeshData);

	LoadMesh(gNanosuitMeshes, "Content/Model/nanosuit/nanosuit.obj", &gGBufferShader, EMeshConversion::YUpToZUP);
	//LoadMesh(gNanosuitMeshes, "Content/Model/Lakecity/Lakecity.obj", &gGBufferShader, EMeshConversion::YUpToZUP);
#if LOAD_SCENE_MESH
	LoadMesh(gSceneMeshes, "Content/Model/sponza/sponza.obj", &gGBufferShader, EMeshConversion::YUpToZUP);
#endif

	// texture
	gDiffuseMap = Texture2D::FindOrCreate("Content/Texture/154.jpg", true, GL_REPEAT, GL_REPEAT);
	gNormalMap = Texture2D::FindOrCreate("Content/Texture/154_norm.jpg", false, GL_REPEAT, GL_REPEAT);
	gFloorDiffuseMap = Texture2D::FindOrCreate("Content/Texture/178.jpg", true, GL_REPEAT, GL_REPEAT);
	gFloorNormalMap = Texture2D::FindOrCreate("Content/Texture/178_norm.jpg", false, GL_REPEAT, GL_REPEAT);

	gSkyboxMap = TextureCube::Create();
	REArray<const char*> skyboxMapNames;
	skyboxMapNames.push_back("Content/Texture/Skybox/miramar_rt.tga");
	skyboxMapNames.push_back("Content/Texture/Skybox/miramar_lf.tga");
	skyboxMapNames.push_back("Content/Texture/Skybox/miramar_up.tga");
	skyboxMapNames.push_back("Content/Texture/Skybox/miramar_dn.tga");
	skyboxMapNames.push_back("Content/Texture/Skybox/miramar_bk.tga");
	skyboxMapNames.push_back("Content/Texture/Skybox/miramar_ft.tga");
	gSkyboxMap->Load(skyboxMapNames, true);

	// set default materials
	gGBufferMaterial->SetParameter("tintColor", Vector4(1.f), 4);
	gGBufferMaterial->SetParameter("hasDiffuseTex", 1);
	gGBufferMaterial->SetParameter("diffuseTex", gDiffuseMap);
	gGBufferMaterial->SetParameter("hasNormalTex", 1);
	gGBufferMaterial->SetParameter("normalTex", gNormalMap);
	gGBufferMaterial->SetParameter("hasRoughnessTex", 0);
	gGBufferMaterial->SetParameter("hasMaskTex", 0);
	gGBufferMaterial->SetParameter("tile", Vector4(1, 1, 0, 0), 4);
	
	gAlphaBlendBasicMaterial->SetParameter("tintColor", Vector4(1.f), 4);
	gAlphaBlendBasicMaterial->SetParameter("hasDiffuseTex", 1);
	gAlphaBlendBasicMaterial->SetParameter("diffuseTex", gDiffuseMap);
	gAlphaBlendBasicMaterial->SetParameter("hasNormalTex", 1);
	gAlphaBlendBasicMaterial->SetParameter("normalTex", gNormalMap);
	gAlphaBlendBasicMaterial->SetParameter("hasRoughnessTex", 0);
	gAlphaBlendBasicMaterial->SetParameter("hasMaskTex", 0);
	gAlphaBlendBasicMaterial->SetParameter("tile", Vector4(1, 1, 0, 0), 4);
	gAlphaBlendBasicMaterial->SetParameter("skyTex", gSkyboxMap);
	gAlphaBlendBasicMaterial->bAlphaBlend = true;

	// light
	MakeLights();

	// mesh components
	MakeMeshComponents();
	
	// camera
	gCamera.fov = 90.f;
	gCamera.position = Vector4_3(0.f, -20.f, 5.f);
	gCamera.euler = Vector4_3(-10.f, 0.f, 0.f);

	return true;
}

void OnWindowResize(int newWidth, int newHeight)
{
	gWindowWidth = newWidth;
	gWindowHeight = newHeight;

	AllocateRenderTarget(false);
	gHasResetFrame = true;
}

void Close()
{
	fileWatcher.Stop();

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
		gCamera.euler.z -= rotSpeed * deltaTime * deltaX;
		gCamera.euler.x -= rotSpeed * deltaTime * deltaY;
		gCamera.euler.x = Clamp(gCamera.euler.x, -89.f, 89.f);
		bShouldCaptureMouse = true;
	}

	Quat cameraRot = EulerToQuat(gCamera.euler);
	Vector4_3 cameraForward = GetForwardVector(cameraRot);
	Vector4_3 cameraUp = GetUpVector(cameraRot);
	Vector4_3 cameraRight = GetRightVector(cameraRot);

	if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE))
	{
		gCamera.position -= cameraRight * deltaX * moveSpeed * deltaTime;
		gCamera.position += cameraUp * deltaY * moveSpeed * deltaTime;
		bShouldCaptureMouse = true;
	}
	// mouse wheel?
	if (gMouseWheel != 0 && !(mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)))
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

	Quat cameraRot = EulerToQuat(gCamera.euler);
	Vector4_3 cameraForward = GetForwardVector(cameraRot);
	Vector4_3 cameraRight = GetRightVector(cameraRot);

	Vector4_3 up = Vector4_3(0, 0, 1);

	const static float maxMoveSpeed = 30.f;
	const static float minMoveSpeed = 0.1f;
	static float moveSpeed = 12.f;

	float moveSpeedFactor = 1.f;
	if (gKeyStates[SDL_SCANCODE_LSHIFT])
		moveSpeedFactor = 0.01f;

	if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
	{
		if (gMouseWheel != 0)
		{
			moveSpeed *= (gMouseWheel > 0) ? 1.1f : 0.9f;
			moveSpeed = Clamp(moveSpeed, minMoveSpeed, maxMoveSpeed);
			gMouseWheel = 0;
		}
		if (gKeyStates[SDL_SCANCODE_W])
		{
			gCamera.position += cameraForward * moveSpeed * moveSpeedFactor * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_S])
		{
			gCamera.position -= cameraForward * moveSpeed * moveSpeedFactor * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_D])
		{
			gCamera.position += cameraRight * moveSpeed * moveSpeedFactor * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_A])
		{
			gCamera.position -= cameraRight * moveSpeed * moveSpeedFactor * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_E])
		{
			gCamera.position += up * moveSpeed * moveSpeedFactor * deltaTime;
		}
		if (gKeyStates[SDL_SCANCODE_Q])
		{
			gCamera.position -= up * moveSpeed * moveSpeedFactor * deltaTime;
		}
	}

}

void Update(float deltaTime)
{
	CPU_SCOPED_PROFILE("update");

	float smoothDeltaTime = Min(0.03f, deltaTime);

	updateMouseInput(smoothDeltaTime);
	updateKeyboardInput(smoothDeltaTime);

	// update spot light
	if (gSpotLights.size() > 0)
	//if(false)
	{
		static const float totalTime = 4.f;
		static float spotLightLocalTime = 0.f;
		spotLightLocalTime += deltaTime;
		if (spotLightLocalTime > totalTime)
		{
			spotLightLocalTime -= totalTime;
		}

		float ratio = spotLightLocalTime / totalTime;
		ratio = Abs(ratio * 2 - 1); // [0 - 1] -> [1 - 0 - 1]

		const Vector4_3 startPos(-3, -10, 3);
		const Vector4_3 endPos(3, -10, 3);
		const Vector4_3 startDir(-2, 0.2f, -0.5f);
		const Vector4_3 endDir(2, 0.2f, -0.5f);
		
		gSpotLights[0].SetPosition(Lerp(startPos, endPos, ratio));
		gSpotLights[0].SetDirection(Lerp(startDir, endDir, ratio).GetNormalized3());
	}

	// end of frame
	for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
	{
		MeshComponent::gMeshComponentContainer[i]->UpdateEndOfFrame(deltaTime);
	}

	// update imgui
	ImGui_Impl_NewFrame(gWindow);
}

void GetTiledShadowMapValue(int index, int totalSize, int& outOffsetX, int& outOffsetY, int& outSize)
{
	outOffsetX = 0;
	outOffsetY = 0;
	outSize = totalSize;
	int offsetSize = 1;
	while (index > 0)
	{
		outSize = outSize >> 1;
		--index;
		if (index & 0x1) outOffsetX += offsetSize;
		if (index & 0x2) outOffsetY += offsetSize;
		index = index >> 2;
		offsetSize = offsetSize << 1;
	}
	outOffsetX *= outSize;
	outOffsetY *= outSize;
}

void GetNormalizedTiledShadowMapValue(int index, int totalSize, float desiredTileSize, 
	float& outNormOffsetX, float& outNormOffsetY, float& outNormSize)
{
	int offsetX = 0;
	int offsetY = 0;
	int tileSize = totalSize;
	int offsetSize = 1;
	while (index > 0)
	{
		tileSize = tileSize >> 1;
		--index;
		if (index & 0x1) offsetX += offsetSize;
		if (index & 0x2) offsetY += offsetSize;
		index = index >> 2;
		offsetSize = offsetSize << 1;
	}
	outNormOffsetX = (float)(tileSize * offsetX) / totalSize;
	outNormOffsetY = (float)(tileSize * offsetY) / totalSize;
	outNormSize = (float)tileSize / totalSize;
}

void CullLights(RenderContext& renderContext)
{
	CPU_SCOPED_PROFILE("cull lights");

	Viewpoint& viewPoint = renderContext.viewPoint;
	
	// local lights
	{
		gVisibleLightList.clear();

		LightRenderData lightDataTmpl;

		// point lights
		lightDataTmpl.bSpot = false;
		lightDataTmpl.bUseTetrahedronShadowMap = true;
		for (int lightIdx = 0, nlightIdx = (int)gPointLights.size(); lightIdx < nlightIdx; ++lightIdx)
		{
			Light& light = gPointLights[lightIdx];
			if (IsSphereIntersectFrustum(light.position, light.radius, viewPoint.frustumPlanes, 6))
			{
				lightDataTmpl.light = &light;
				lightDataTmpl.bActualCastShadow = light.bCastShadow && gRenderSettings.bDrawShadow && gRenderSettings.bDrawShadowPoint;
				float lightDist = (light.position - viewPoint.position).Size3();
				lightDataTmpl.shadowMapSize = light.sphereBounds.centerRadius.w * viewPoint.screenScale / Max(lightDist, 1.f) * 1.25f;
				gVisibleLightList.push_back(lightDataTmpl);
			}
		}

		// spot lights
		lightDataTmpl.bSpot = true;
		lightDataTmpl.bUseTetrahedronShadowMap = false;
		Vector4 packedFrustumVerts[6];
		for (int lightIdx = 0, nlightIdx = (int)gSpotLights.size(); lightIdx < nlightIdx; ++lightIdx)
		{
			Light& light = gSpotLights[lightIdx];
			MakeFrustumPackedVerts(light.lightInvViewMat, 0, light.radius, light.outerTanHalfAngle, 1.f, packedFrustumVerts);
			if (IsFrustumIntersectFrustum(packedFrustumVerts, viewPoint.frustumPlanes, 6))
			{
				lightDataTmpl.light = &light;
				lightDataTmpl.bActualCastShadow = light.bCastShadow && gRenderSettings.bDrawShadow && gRenderSettings.bDrawShadowSpot;
				float lightDist = (light.position - viewPoint.position).Size3();
				lightDataTmpl.shadowMapSize = light.sphereBounds.centerRadius.w * viewPoint.screenScale / Max(lightDist, 1.f) * 0.5f;
				gVisibleLightList.push_back(lightDataTmpl);
			}
		}

		// sort
		std::sort(gVisibleLightList.begin(), gVisibleLightList.end(), LightRenderData::CompareShadowIndex);

		gCurLocalLightShadowMatCount = 0;
		const float minCubeMapScreenSize = 400.f;
		const int maxCubeMapCount = 4;
		const float minTileSize = 128;
		const float maxTileSize = 1024;
		int cubeMapIdx = 0;
		int currentTileIndex = 0;
		int currentTileMaxIndex = 0;
		int totalSize = gShadowTiledTex.width;
		int nextTileSize = totalSize >> 1;
		// assign shadow index
		for (int lightIdx = 0, nlightIdx = (int)gVisibleLightList.size(); lightIdx < nlightIdx; ++lightIdx)
		{
			LightRenderData& lightData = gVisibleLightList[lightIdx];

			// we reached non shadow caster, break
			if (!lightData.bActualCastShadow)
				break;

			// tmp assign shadow index
			if (!lightData.bSpot && lightData.shadowMapSize > minCubeMapScreenSize && cubeMapIdx < maxCubeMapCount)
			{
				lightData.bUseTetrahedronShadowMap = false;
				lightData.shadowMapIndex = cubeMapIdx;
				++cubeMapIdx;
				gCurLocalLightShadowMatCount += 2; // 2 shadow mat for cube map
				continue;
			}

			// if we used up all tiles, no more shadow
			if (currentTileIndex > currentTileMaxIndex)
			{
				lightData.bActualCastShadow = false;
				continue;
			}

			gCurLocalLightShadowMatCount += (lightData.bSpot ? 1 : 5); // 1 shadow mat for spot light, 5 for tetrahedron
			lightData.shadowMapSize = Clamp(lightData.shadowMapSize, minTileSize, maxTileSize);
			// find the right level
			while (lightData.shadowMapSize <= (float)nextTileSize)
			{
				// to next level
				nextTileSize = nextTileSize >> 1;
				currentTileIndex = (currentTileIndex << 2) + 1;
				currentTileMaxIndex = (currentTileMaxIndex + 1) << 2;
			}
			lightData.shadowMapIndex = currentTileIndex;
			//GetTiledShadowMapValue(lightData.shadowMapIndex, totalSize,
			//	lightData.offsetX, lightData.offsetY, lightData.size);

			++currentTileIndex;
		}
		gShadowCubeMapCount = cubeMapIdx;

		// sort
		std::sort(gVisibleLightList.begin(), gVisibleLightList.end(), LightRenderData::CompareRender);
	}
}

void SetupLightTileBuffer(int tileCount)
{	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LightTileInfoData);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * tileCount * RenderConsts::lightTileInfoStride, 0, GL_DYNAMIC_DRAW);

	int tileCullingResultStride = (RenderConsts::maxLightPerSlicedTile <= 16 ? 1 : 2);
	int tileCullingResultCount = tileCount * RenderConsts::lightSlicesPerTile * RenderConsts::maxLightPerSlicedTile;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LightTileCullingResultInfoData);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * tileCullingResultCount * tileCullingResultStride + RenderConsts::lightTileCullingResultOffset, 0, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void DrawMesh(RenderContext& renderContext, MeshRenderData& meshRenderData)
{
	if (!meshRenderData.material || !meshRenderData.material->shader)
		return;

	meshRenderData.material->SetParameter("prevModelMat", meshRenderData.prevModelMat);
	meshRenderData.material->SetParameter("modelMat", meshRenderData.modelMat);
	
	//if(renderContext.currentMaterial != material)
	meshRenderData.material->Use(renderContext);

	if (meshRenderData.VAO != renderContext.currentVAO)
	{
		renderContext.currentVAO = meshRenderData.VAO;
		glBindVertexArray(meshRenderData.VAO);
	}
	if (meshRenderData.material->bBothSide && renderContext.currentRenderState->bCullFace)
		glDisable(GL_CULL_FACE);
	//glDrawElements(GL_TRIANGLES, (GLsizei)meshData->indices.size(), GL_UNSIGNED_INT, 0);
	glDrawElements(GL_TRIANGLES, meshRenderData.idxCount, GL_UNSIGNED_INT, 0);
	//glBindVertexArray(0);

	if (meshRenderData.material->bBothSide && renderContext.currentRenderState->bCullFace)
		glEnable(GL_CULL_FACE);
}

void GeometryPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("geometry");

	static const RenderState renderState;

	renderState.Apply(renderContext);

	// clear frame buffer
	glClearColor(0, 0, 0, 0);
	glClearDepth(1);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	// visibility
	// clear list
	gOpaqueMeshRenderList.clear();
	gAlphaBlendMeshRenderList.clear();
	for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
	{
		MeshComponent* meshComp = MeshComponent::gMeshComponentContainer[i];
		if(IsOBBIntersectFrustum(meshComp->OBB.permutedAxisCenter, meshComp->OBB.extent, renderContext.viewPoint.frustumPlanes, 6))
		{
			//meshComp->Draw(renderContext);
			// add mesh to render list
			MeshRenderData renderDataTmpl;
			renderDataTmpl.prevModelMat = meshComp->prevModelMat;
			renderDataTmpl.modelMat = meshComp->modelMat;
			const REArray<Mesh*>& meshList = meshComp->GetMeshList();
			for (int mi = 0, nmi = (int)meshList.size(); mi < nmi; ++mi)
			{
				Mesh* mesh = meshList[mi];

				REArray<MeshRenderData, 16>* listPtr = 0;
				if (!mesh->material->bAlphaBlend)
					listPtr = &gOpaqueMeshRenderList;
				else
					listPtr = &gAlphaBlendMeshRenderList;

				renderDataTmpl.material = mesh->material;
				renderDataTmpl.VAO = mesh->meshData->VAO;
				renderDataTmpl.idxCount = mesh->meshData->idxCount;

				renderDataTmpl.distToCamera =
					(renderDataTmpl.modelMat.TransformPoint(mesh->meshData->bounds.GetCenter()) - renderContext.viewPoint.position).Size3();

				listPtr->push_back(renderDataTmpl);
			}
		}
	}

	// draw mesh
	for (int i = 0, ni = (int)gOpaqueMeshRenderList.size(); i < ni; ++i)
	{
		DrawMesh(renderContext, gOpaqueMeshRenderList[i]);
	}
}

void SSAOPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("SSAO");

	const static RenderState renderState([](RenderState& s) {
		// no depth test, no depth write
		s.bDepthTest = false;
		s.bDepthWrite = false;
		// only write alpha
		s.bColorWrite = true;
		s.bColorWriteR = false;
		s.bColorWriteG = false;
		s.bColorWriteB = false;
		s.bColorWriteA = true;
	});

	renderState.Apply(renderContext);

	// no need to clear
	
	gFSQuadMesh->Draw(renderContext, gSSAOMaterial);
}

void DirectionalLightPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("directional light");

	const static RenderState renderState([](RenderState& s) {
		// depth test to skip sky box (depth == 1)
		s.bDepthTest = true;
		s.depthTestFunc = GL_GREATER;
		s.bDepthWrite = false;
	});

	// directional light
	renderState.Apply(renderContext);

	if (!gRenderSettings.bSkybox)
	{
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// draw quad
	gFSQuadMesh->Draw(renderContext, gDirectionalLightMaterial);
}

Material* SetupLightVolumeMaterial(RenderContext& renderContext, const LightRenderData& lightData, int lightIndex)
{
	Light& light = *lightData.light;

	Material* lightVolumeMaterial = lightData.bSpot ? gLightVolumeSpotMaterial :
		(lightData.bUseTetrahedronShadowMap ? gLightVolumePointTetrahedronMaterial : gLightVolumePointCubeMaterial);

	lightVolumeMaterial->SetParameter(ShaderNameBuilder("light")("color").c_str(), light.colorIntensity, 4);
	lightVolumeMaterial->SetParameter(ShaderNameBuilder("light")("attenParams").c_str(), light.attenParams, 4);
	lightVolumeMaterial->SetParameter(ShaderNameBuilder("light")("positionInvR").c_str(), light.GetPositionVSInvR(renderContext.viewPoint.viewMat), 4);
	lightVolumeMaterial->SetParameter(ShaderNameBuilder("light")("directionRAB").c_str(), light.GetDirectionVSRAB(renderContext.viewPoint.viewMat), 4);
	lightVolumeMaterial->SetParameter("modelMat", light.modelMat);
	//lightVolumeMaterial->SetParameter("lightIndex", lightIndex);

	bool bDrawShadow = gRenderSettings.bDrawShadow && lightData.bActualCastShadow;
	if (lightData.bSpot)
		bDrawShadow &= gRenderSettings.bDrawShadowSpot;
	else
		bDrawShadow &= gRenderSettings.bDrawShadowPoint;
	// shadow
	if (bDrawShadow)
	{
		lightVolumeMaterial->SetParameter(ShaderNameBuilder("light")("shadowParamA").c_str(), 1);
		lightVolumeMaterial->SetParameter("shadowMat", light.shadowMat[0]);
		if (lightData.bSpot)
		{
		}
		else
		{
			if (lightData.bUseTetrahedronShadowMap)
			{
				for (int i = 0; i < 4; ++i)
					lightVolumeMaterial->SetParameter(ShaderNameBuilder("lightProjRemapMat")[i].c_str(), light.shadowMat[i+1]);
			}
			else
			{
				lightVolumeMaterial->SetParameter("lightProjRemapMat", light.shadowMat[1]);
				lightVolumeMaterial->SetParameter(ShaderNameBuilder("light")("shadowParamB").c_str(), lightData.shadowMapIndex);
			}
		}
	}
	else
	{
		lightVolumeMaterial->SetParameter(ShaderNameBuilder("light")("shadowParamA").c_str(), -1);
	}

	return lightVolumeMaterial;
}

void LightVolumePass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("light volume");

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

	// one pass render state, when camera inside volume
	const static RenderState frontFogVolumeRenderState([](RenderState& s) {
		// inverse depth test only, don't write depth
		s.bDepthTest = false;
		//s.depthTestFunc = GL_LESS;
		//s.bDepthWrite = false;
		// draw backface
		s.bCullFace = true;
		s.cullFaceMode = GL_FRONT;
	});

	REArray<int> cameraInsideLight;
	cameraInsideLight.reserve(4);
	REArray<int> volumetricLight;
	volumetricLight.reserve(4);

	int stencilBits = 8;

	for (int i = 0, ni = (int)gVisibleLightList.size(); i < ni; i += stencilBits)
	{
		// prepass
		prepassRenderState.Apply(renderContext);

		// clear stencil
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);

		char skipFlag = 0;

		for (int j = 0; j < stencilBits && i + j < ni; ++j)
		{
			int lightIdx = i + j;
			char mask = 1 << j;

			const Light& light = *gVisibleLightList[lightIdx].light;
			bool bSpot = (light.attenParams.y > 0);

			// volumetric?
			if (light.bVolumetricFog)
			{
				skipFlag |= mask;
				volumetricLight.push_back(lightIdx);
				continue;
			}

			// camera in light volume?
			Vector4_3 lightToCamera = renderContext.viewPoint.position - light.position;
			float radiusFactor = 1.01f;
			if (bSpot)
				radiusFactor /= light.outerCosHalfAngle;
			float adjustedRadius = light.radius * radiusFactor + renderContext.viewPoint.nearRadius;
			bool isInLightVolume = (lightToCamera.SizeSqr3() < adjustedRadius * adjustedRadius);
			if (bSpot && isInLightVolume)
			{
				// cone check
				isInLightVolume = (lightToCamera.GetNormalized3().Dot3(light.direction) > light.outerCosHalfAngle - KINDA_SMALL_NUMBER);
				if (!isInLightVolume)
				{
					// test near plane clip
					float dp = lightToCamera.Dot3(light.direction);
					isInLightVolume |= (Sqrt(lightToCamera.SizeSqr3() - dp * dp) - dp * light.outerTanHalfAngle) * light.outerCosHalfAngle <= renderContext.viewPoint.nearRadius;
				}
			}

			if (isInLightVolume)
			{
				skipFlag |= mask;
				cameraInsideLight.push_back(lightIdx);
				continue;
			}

			// set mask
			glStencilFunc(prepassRenderState.stencilTestFunc, mask, mask);
			glStencilMask(mask);

			gPrepassMaterial->SetParameter("modelMat", light.modelMat);
			light.LightMesh->Draw(renderContext, gPrepassMaterial);
		}

		// draw light
		lightingRenderState.Apply(renderContext);

		for (int j = 0; j < stencilBits && i + j < ni; ++j)
		{
			int lightIdx = i + j;
			char mask = 1 << j;

			const LightRenderData& lightData = gVisibleLightList[lightIdx];

			if (skipFlag & mask)
				continue;

			// set mask
			glStencilFunc(lightingRenderState.stencilTestFunc, mask, mask);

			Material* lightVolumeMaterial = SetupLightVolumeMaterial(renderContext, lightData, lightIdx);
			lightData.light->LightMesh->Draw(renderContext, lightVolumeMaterial);
		}
	}

	// draw camera inside lights
	onePassVolumeRenderState.Apply(renderContext);
	for (int i = 0, ni = (int)cameraInsideLight.size(); i < ni; ++i)
	{
		int lightIndex = cameraInsideLight[i];
		const LightRenderData& lightData = gVisibleLightList[lightIndex];
		Material* lightVolumeMaterial = SetupLightVolumeMaterial(renderContext, lightData, lightIndex);
		lightData.light->LightMesh->Draw(renderContext, lightVolumeMaterial);
	}

	frontFogVolumeRenderState.Apply(renderContext);
	for (int i = 0, ni = (int)volumetricLight.size(); i < ni; ++i)
	{
		int lightIndex = volumetricLight[i];
		const LightRenderData& lightData = gVisibleLightList[lightIndex];
		Material* lightVolumeMaterial = SetupLightVolumeMaterial(renderContext, lightData, lightIndex);
		lightData.light->LightMesh->Draw(renderContext, lightVolumeMaterial);
	}
}

void LightPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("light");

	// must have directional light pass, it put down base color (also clears color if no skybox)
	DirectionalLightPass(renderContext);

	// enable blend for light additive
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	LightVolumePass(renderContext);

	// disable blend
	glDisable(GL_BLEND);
}

void TileBasedDeferredLightPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("tile based light");


	if (gRenderSettings.bTileInfo)
	{
		if (!gRenderSettings.bTileOnePass)
		{
			GPU_SCOPED_PROFILE("tile based culling");

			gLightTileReductionCompMaterial->DispatchCompute(renderContext, gRenderInfo.TileCountX, gRenderInfo.TileCountY);

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			// 32 for NVidia, 64 for AMD
			const unsigned int threadPerGroup = 128;
			unsigned int cullingGroupCount = (gRenderInfo.TileCountX * gRenderInfo.TileCountY + threadPerGroup - 1) / threadPerGroup;
			// for culling, 1 thread per tile, use max thread per warp/wavefront in a group
			gLightTileCullingCompMaterial->DispatchCompute(renderContext, cullingGroupCount);

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}
		else
		{
			GPU_SCOPED_PROFILE("tile based culling");

			gLightTileReductionAndCullingCompMaterial->DispatchCompute(renderContext, gRenderInfo.TileCountX, gRenderInfo.TileCountY);

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		// now render light
		const static RenderState renderState([](RenderState& s) {
			// depth test to skip sky box (depth == 1)
			s.bDepthTest = true;
			s.depthTestFunc = GL_GREATER;
			s.bDepthWrite = false;
		});

		// directional light
		renderState.Apply(renderContext);

		if (!gRenderSettings.bSkybox)
		{
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		// draw quad
		gFSQuadMesh->Draw(renderContext, gTiledDeferredLightMaterial);
	}
	else
	{
		Texture2D& curSceneColorTex = gSceneColorTex[gSceneColorWriteIdx];
		curSceneColorTex.access = GL_WRITE_ONLY;
		gTileBasedDeferredLightCompMaterial->SetParameter("outColor", &curSceneColorTex, true);

		gTileBasedDeferredLightCompMaterial->DispatchCompute(renderContext, gRenderInfo.TileCountX, gRenderInfo.TileCountY);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
}

void DrawShadowScene(RenderContext& renderContext, Texture* shadowMap, const RenderInfo& renderInfo, Material* material,
	REArray<MeshComponent*>& involvedMeshComps)
{
	if (shadowMap)
	{
		// attach to frame buffers
		gDepthOnlyBuffer.AttachDepth(shadowMap, false);

		// set viewport
		glViewport(0, 0, shadowMap->width, shadowMap->height);

		// clear depth
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	// update ubo
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RenderInfo), &renderInfo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// draw models
	for (int i = 0, ni = (int)involvedMeshComps.size(); i < ni; ++i)
	{
		MeshComponent*& meshComp = involvedMeshComps[i];
		if(meshComp->bRenderVisibile)
			meshComp->Draw(renderContext, material);
	}
}

void ShadowPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("shadow");
	CPU_SCOPED_PROFILE("shadow");

	const static RenderState renderState([](RenderState& s) {
		// don't write color
		s.bColorWrite = false;
	});

	renderState.Apply(renderContext);

	const static Matrix4 remapMat(
		Vector4(0.5f, 0.f, 0.f, 0.0f),
		Vector4(0.f, 0.5f, 0.f, 0.0f),
		Vector4(0.f, 0.f, 0.5f, 0.0f),
		Vector4(0.5f, 0.5f, 0.5f, 1.f)
		);

	Viewpoint& viewPoint = renderContext.viewPoint;

	RenderInfo shadowRenderInfo = gRenderInfo;

	GlobalLightsRenderInfo globalLightsRenderInfo;
	globalLightsRenderInfo.localLightCount = (int)gVisibleLightList.size();
	globalLightsRenderInfo.globalLightCount = Min((int)gDirectionalLights.size(), RenderConsts::MAX_DIRECTIONAL_LIGHT_COUNT);

	// directional lights
	if(gRenderSettings.bDrawShadow && gRenderSettings.bDrawShadowCSM)
	{
		// get shadow map count
		int csmCount = 0;
		for (int lightIdx = 0, nlightIdx = Min((int)gDirectionalLights.size(), RenderConsts::MAX_DIRECTIONAL_LIGHT_COUNT); lightIdx < nlightIdx; ++lightIdx)
		{
			Light& light = gDirectionalLights[lightIdx];
			if (light.bCastShadow)
				csmCount += RenderConsts::MAX_CSM_COUNT;
		}

		const int csmMapSize = 1024;
		// TODO: deallocate when count drop to 0 ?
		if (gCSMTexArray.count == 0)
		{
			gCSMTexArray.AllocateForFrameBuffer(csmMapSize, csmMapSize, csmCount, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, true);
		}
		else if (gCSMTexArray.count != csmCount)
		{
			gCSMTexArray.Reallocate(csmMapSize, csmMapSize, csmCount);
		}

		// attach to frame buffers
		gDepthOnlyBuffer.AttachDepth(&gCSMTexArray, false);
		// set viewport
		glViewport(0, 0, gCSMTexArray.width, gCSMTexArray.height);
		// clear depth
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);

		const static float cascadeRatio[RenderConsts::MAX_CSM_COUNT] = {
			0.12f, 0.36f, 1.f
		};

		Vector4_3 clipPoints[4];

		float aspectRatio = viewPoint.height / viewPoint.width;
		float tanHF = Tan(viewPoint.fov * 0.5f) * Sqrt(1 + aspectRatio * aspectRatio);
		float tanHF2 = tanHF * tanHF;

		bool bFixedSize = false;
		int csmIndex = 0;

		REArray<BoxBounds, 16> lightSpaceBounds;
		lightSpaceBounds.resize(MeshComponent::gMeshComponentContainer.size());
		
		int shadowIdx = 0;
		for (int lightIdx = 0, nlightIdx = (int)gDirectionalLights.size(); lightIdx < nlightIdx; ++lightIdx)
		{
			Light& light = gDirectionalLights[lightIdx];

			LightRenderInfo& lightRenderInfo = globalLightsRenderInfo.globalLights[lightIdx];
			lightRenderInfo.directionRAB = light.GetDirectionVSRAB(viewPoint.viewMat);
			lightRenderInfo.color = light.colorIntensity;
			lightRenderInfo.attenParams = light.attenParams;
			lightRenderInfo.shadowParamA = 0;

			if (!light.bCastShadow)
				continue;

			// calculate light space bounds
			for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
			{
				MeshComponent*& meshComp = MeshComponent::gMeshComponentContainer[i];
				const Matrix4& adjustMat = light.lightViewMat * meshComp->modelMat;
				// tranform bounds into light space
				lightSpaceBounds[i] = meshComp->bounds.GetTransformedBounds(adjustMat);
			}

			Matrix4 viewToLight = light.lightViewMat * viewPoint.invViewMat;

			const int cascadeCount = RenderConsts::MAX_CSM_COUNT;

			int cascadeIdx = 0;
			for (; cascadeIdx < cascadeCount; ++cascadeIdx)
			{
				// near and far plane for this cascade
				float n = (cascadeIdx == 0) ? viewPoint.nearPlane : viewPoint.farPlane * cascadeRatio[cascadeIdx - 1];
				float f = viewPoint.farPlane * cascadeRatio[cascadeIdx];

				// minimal bounding sphere diameter
				float diameter = ((f + n) * tanHF2 >= (f - n)) ?
					2 * f * tanHF :
					Sqrt((f - n)*(f - n) + 2 * (f*f + n*n) * tanHF2 + (f + n)*(f + n)*tanHF2*tanHF2);

				float pixelRate = diameter / gCSMTexArray.width;

				// process frustum bounds
				BoxBounds frustumBounds;
				viewPoint.GetClipPoints(-n, clipPoints);
				for (int i = 0; i < 4; ++i)
					frustumBounds += light.lightViewMat.TransformPoint(clipPoints[i]);
				viewPoint.GetClipPoints(-f, clipPoints);
				for (int i = 0; i < 4; ++i)
					frustumBounds += light.lightViewMat.TransformPoint(clipPoints[i]);

				// extent test bound max(near plane) a little bit to include geometry behind us
				// this value need to be increased if we are missing shadow
				const float stepBack = 50.f;
				frustumBounds.max.z += stepBack;

				// process scene bounds and do frustum culling
				bool bHasMeshToRender = false;
				BoxBounds sceneBounds;
				for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
				{
					MeshComponent*& meshComp = MeshComponent::gMeshComponentContainer[i];
					// overlap test
					if (IsAABBIntersectAABB(frustumBounds.min, frustumBounds.max, 
						lightSpaceBounds[i].min, lightSpaceBounds[i].max))
					{
						sceneBounds += lightSpaceBounds[i];
						meshComp->bRenderVisibile = true;
						bHasMeshToRender = true;
					}
					else
					{
						meshComp->bRenderVisibile = false;
					}
				}

				// skip if we have no mesh to render
				if (!bHasMeshToRender)
					continue;

				// only change far plane if scene bounds are closer, don't extend it
				frustumBounds.min.z = Max(sceneBounds.min.z, frustumBounds.min.z);
				// for near plane simply use what scene bounds has,
				// since we need to include any mesh behind us (in light space) that will cast shadow
				frustumBounds.max.z = sceneBounds.max.z;

				if (bFixedSize || cascadeIdx == 0)
				{
					// adjust based on diameter
					frustumBounds.min.x = floorf(frustumBounds.min.x / pixelRate) * pixelRate;
					frustumBounds.min.y = floorf(frustumBounds.min.y / pixelRate) * pixelRate;
					frustumBounds.max.x = frustumBounds.min.x + diameter;
					frustumBounds.max.y = frustumBounds.min.y + diameter;
				}
				else
				{
					frustumBounds.min.x = Max(frustumBounds.min.x, sceneBounds.min.x);
					frustumBounds.min.y = Max(frustumBounds.min.y, sceneBounds.min.y);
					frustumBounds.max.x = Min(frustumBounds.max.x, sceneBounds.max.x);
					frustumBounds.max.y = Min(frustumBounds.max.y, sceneBounds.max.y);
				}

				// the bounds are mapped as (+x, -x, +y, -y, +z, -z) -> (r, l, t, b, -n, -f)
				// ref: http://www.songho.ca/opengl/gl_projectionmatrix.html
				Matrix4 lightProjMat = MakeMatrixOrthoProj(
					frustumBounds.min.x, frustumBounds.max.x,
					frustumBounds.min.y, frustumBounds.max.y,
					-frustumBounds.max.z, -frustumBounds.min.z,
					(float)gCSMTexArray.width, (float)gCSMTexArray.height,
					viewPoint.jitterX, viewPoint.jitterY);

				// update shadow info
				globalLightsRenderInfo.globalShadowData[shadowIdx].shadowMat = remapMat * lightProjMat * viewToLight;
				globalLightsRenderInfo.globalShadowData[shadowIdx].bounds = Vector4_3(
					frustumBounds.max.x - frustumBounds.min.x,
					frustumBounds.max.y - frustumBounds.min.y,
					f);
				++shadowIdx;

				// update render info
				shadowRenderInfo.View = light.lightViewMat;
				shadowRenderInfo.Proj = lightProjMat;
				shadowRenderInfo.ViewProj = lightProjMat * light.lightViewMat;

				// attach to right layer
				gDepthOnlyBuffer.AttachDepth(&gCSMTexArray, false, csmIndex);
				DrawShadowScene(renderContext, 0, shadowRenderInfo, gPrepassMaterial, MeshComponent::gMeshComponentContainer);
				++csmIndex;
			}

			// set shadow count
			lightRenderInfo.shadowParamA = cascadeIdx;
		}
		assert(shadowIdx <= csmCount);
	}
	else
	{
		// set light
		int shadowIdx = 0;
		for (int i = 0; i < globalLightsRenderInfo.globalLightCount; ++i)
		{
			Light& light = gDirectionalLights[i];
			LightRenderInfo& lightRenderInfo = globalLightsRenderInfo.globalLights[i];
			lightRenderInfo.directionRAB = light.GetDirectionVSRAB(viewPoint.viewMat);
			lightRenderInfo.color = light.colorIntensity;
			lightRenderInfo.attenParams = light.attenParams;
			lightRenderInfo.shadowParamA = 0;
		}
	}
	


	// enable custom clipping planes, clip distance is sent in vertex shader or geometry shader via gl_ClipDistance
	glEnable(GL_CLIP_DISTANCE0);
	glEnable(GL_CLIP_DISTANCE1);
	glEnable(GL_CLIP_DISTANCE2);
	glEnable(GL_CLIP_DISTANCE3);


	// local light data
	int visibleLightCount = (int)gVisibleLightList.size();
	REArray<LightRenderInfo> localLights;
	REArray<Vector4, 16> localLightsBounds;
	REArray<Matrix4, 16> shadowMatrices;
	localLights.reserve(visibleLightCount);
	localLightsBounds.reserve(visibleLightCount);
	shadowMatrices.reserve(gCurLocalLightShadowMatCount);

	LightRenderInfo lightTmpl;

	const float lightNearPlane = 0.01f;

	float maxLocalLightDist = 0.f;

	bool bDrawShadowPoint = gRenderSettings.bDrawShadow && gRenderSettings.bDrawShadowPoint;
	bool bDrawShadowSpot = gRenderSettings.bDrawShadow && gRenderSettings.bDrawShadowSpot;
	bool bSpotLightPass = false;
	bool bTetrahedronMapPass = false;
	bool bCubeMapPass = false;
	int shadowMatIdx = 0;
	for (int lightIdx = 0, nlightIdx = (int)gVisibleLightList.size(); lightIdx < nlightIdx; ++lightIdx)
	{
		LightRenderData& lightData = gVisibleLightList[lightIdx];
		Light& light = *lightData.light;

		lightTmpl.positionInvR = light.GetPositionVSInvR(viewPoint.viewMat);
		lightTmpl.directionRAB = light.GetDirectionVSRAB(viewPoint.viewMat);
		lightTmpl.color = light.colorIntensity;
		lightTmpl.attenParams = light.attenParams;
		lightTmpl.shadowParamA = lightData.bActualCastShadow ? shadowMatIdx : -1;
		lightTmpl.shadowParamB = (!lightData.bSpot && !lightData.bUseTetrahedronShadowMap) ? lightData.shadowMapIndex : -1;

		localLights.push_back(lightTmpl);
		Vector4 sphereBounds = light.GetSphereBoundVS(viewPoint.viewMat);
		localLightsBounds.push_back(sphereBounds);

		maxLocalLightDist = Max(maxLocalLightDist, sphereBounds.Size3() + sphereBounds.w);

		if (!lightData.bActualCastShadow)
			continue;

		shadowMatIdx += (lightData.bSpot ? 1 : (lightData.bUseTetrahedronShadowMap ? 5 : 2));

		bool bShouldInitTiledShadowMap = false;
		bool bShouldInitCubeMapArray = false;
		if (!bSpotLightPass && lightData.bSpot && bDrawShadowSpot)
		{
			// init spot light pass
			bSpotLightPass = true;
			bShouldInitTiledShadowMap = true;
		}
		if (!bTetrahedronMapPass && !lightData.bSpot && lightData.bUseTetrahedronShadowMap && bDrawShadowPoint)
		{
			// init tetrahedron map pass
			bTetrahedronMapPass = true;
			if (!bSpotLightPass)
				bShouldInitTiledShadowMap = true;
			
			// only need 3 clipping planes
			glDisable(GL_CLIP_DISTANCE3);
		}
		if (!bCubeMapPass && !lightData.bSpot && !lightData.bUseTetrahedronShadowMap && bDrawShadowPoint)
		{
			// init cube map pass
			bCubeMapPass = true;
			bShouldInitCubeMapArray = true;

			// disable custom clipping planes
			glDisable(GL_CLIP_DISTANCE0);
			glDisable(GL_CLIP_DISTANCE1);
			glDisable(GL_CLIP_DISTANCE2);
			glDisable(GL_CLIP_DISTANCE3);

		}

		if (bShouldInitTiledShadowMap)
		{
			// attach to frame buffers
			gDepthOnlyBuffer.AttachDepth(&gShadowTiledTex, false);
			// set viewport
			glViewport(0, 0, gShadowTiledTex.width, gShadowTiledTex.height);
			// clear depth
			glClearDepth(1);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		if (bShouldInitCubeMapArray)
		{
			const int cubeMapSize = 512;
			// TODO: deallocate when count drop to 0 ?
			if (gShadowCubeTexArray.count == 0)
			{
				gShadowCubeTexArray.AllocateForFrameBuffer(cubeMapSize, cubeMapSize, gShadowCubeMapCount, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, true);
			}
			else if (gShadowCubeTexArray.count != gShadowCubeMapCount)
			{
				gShadowCubeTexArray.Reallocate(cubeMapSize, cubeMapSize, gShadowCubeMapCount);
			}

			// attach to frame buffers
			gDepthOnlyBuffer.AttachDepth(&gShadowCubeTexArray, false);
			// set viewport
			glViewport(0, 0, gShadowCubeTexArray.width, gShadowCubeTexArray.height);
			// clear depth
			glClearDepth(1);
			glClear(GL_DEPTH_BUFFER_BIT);
		}


		// spot lights
		if (lightData.bSpot && bDrawShadowSpot)
		{
			Plane frustumPlanes[6];

			int totalSize = gShadowTiledTex.width;
			float offsetX = 0.f, offsetY = 0.f, tileSize = 0.f;
			GetNormalizedTiledShadowMapValue(lightData.shadowMapIndex, totalSize, lightData.shadowMapSize, 
				offsetX, offsetY, tileSize);

			Matrix4 tileMat = Matrix4(
				Vector4(tileSize, 0.f, 0.f, 0.f),
				Vector4(0.f, tileSize, 0.f, 0.f),
				Vector4(0.f, 0.f, 1.f, 0.f),
				Vector4(tileSize + 2.f * offsetX - 1.f, tileSize + 2.f * offsetY - 1.f, 0.f, 1.f)
			);

			gPrepassTiledMaterial->SetParameter("clippingValue", 
				Vector4(offsetX, offsetX + tileSize, offsetY, offsetY + tileSize) * 2.f - 1.f,
				4);

			Matrix4 lightProjMat = MakeMatrixPerspectiveProj(
				DegToRad(light.outerHalfAngle) * 2,
				lightData.shadowMapSize, lightData.shadowMapSize,
				lightNearPlane, light.radius,
				viewPoint.jitterX, viewPoint.jitterY);

			light.shadowMat[0] = remapMat * tileMat * lightProjMat * light.lightViewMat * viewPoint.invViewMat;
			shadowMatrices.push_back(light.shadowMat[0]);

			// update render info
			shadowRenderInfo.View = light.lightViewMat;
			shadowRenderInfo.Proj = tileMat * lightProjMat;
			shadowRenderInfo.ViewProj = tileMat * lightProjMat * light.lightViewMat;

			// culling
			GetFrustumPlanes(light.lightViewMat, lightProjMat.m[0][0], lightProjMat.m[1][1],
				lightNearPlane, light.radius, frustumPlanes);

			bool bHasMeshToRender = false;
			for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
			{
				MeshComponent*& meshComp = MeshComponent::gMeshComponentContainer[i];
				if (IsOBBIntersectFrustum(meshComp->OBB.permutedAxisCenter, meshComp->OBB.extent, frustumPlanes, 6))
				{
					meshComp->bRenderVisibile = true;
					bHasMeshToRender = true;
				}
				else
				{
					meshComp->bRenderVisibile = false;
				}
			}

			if (bHasMeshToRender)
				DrawShadowScene(renderContext, 0, shadowRenderInfo, gPrepassTiledMaterial, MeshComponent::gMeshComponentContainer);
		}
		// point lights
		else if (!lightData.bSpot && bDrawShadowPoint)
		{
			// update render info, only do view, since we proj in geometry shader
			shadowRenderInfo.View = light.lightViewMat;
			shadowRenderInfo.Proj = Matrix4::Identity();
			shadowRenderInfo.ViewProj = light.lightViewMat;

			if(lightData.bUseTetrahedronShadowMap)
			{
				int totalSize = gShadowTiledTex.width;
				float offsetX = 0.f, offsetY = 0.f, tileSize = 0.f;
				GetNormalizedTiledShadowMapValue(lightData.shadowMapIndex, totalSize, lightData.shadowMapSize,
					offsetX, offsetY, tileSize);

				Matrix4 tileMat = Matrix4(
					Vector4(tileSize, 0.f, 0.f, 0.f),
					Vector4(0.f, tileSize, 0.f, 0.f),
					Vector4(0.f, 0.f, 1.f, 0.f),
					Vector4(tileSize + 2.f * offsetX - 1.f, tileSize + 2.f * offsetY - 1.f, 0.f, 1.f)
				);

				const float tetrahedronFov = DegToRad(143.98570868f);
				const float tetrahedronWidth = 1.59245026f; //tan(DegToRad(143.98570868 * 0.5)) / tan(DegToRad(125.26438968 * 0.5));
				const float jitterScale = 2.f;

				Matrix4 lightProjMat = MakeMatrixPerspectiveProj(
					tetrahedronFov,
					tetrahedronWidth * lightData.shadowMapSize, lightData.shadowMapSize,
					lightNearPlane, light.radius,
					viewPoint.jitterX * jitterScale, viewPoint.jitterY * jitterScale);

				const Matrix4 gLightOmniTextureProjMat[4] =
				{
					// forwad to texture bottom
					Matrix4(Vector4(1.f,0.f,0.f,0.f), Vector4(0.f,0.5f,0.f,0.f), Vector4(0.f,0.f,1.f,0.f), Vector4(0.f,-0.5f,0.f,1.f)),
					// backward to texture right
					Matrix4(Vector4(0.f,1.f,0.f,0.f), Vector4(-0.5f,0.f,0.f,0.f), Vector4(0.f,0.f,1.f,0.f), Vector4(0.5f,0.f,0.f,1.f)),
					// right to texture top
					Matrix4(Vector4(1.f,0.f,0.f,0.f), Vector4(0.f,0.5f,0.f,0.f), Vector4(0.f,0.f,1.f,0.f), Vector4(0.f,0.5f,0.f,1.f)),
					// left to texture left
					Matrix4(Vector4(0.f,1.f,0.f,0.f), Vector4(-0.5f,0.f,0.f,0.f), Vector4(0.f,0.f,1.f,0.f), Vector4(-0.5f,0.f,0.f,1.f)),
				};


				light.shadowMat[0] = light.lightViewMat * viewPoint.invViewMat;
				shadowMatrices.push_back(light.shadowMat[0]);

				for (int i = 0; i < 4; ++i)
				{
					Matrix4 lightViewProjMat = tileMat * gLightOmniTextureProjMat[i] * lightProjMat * gLightTetrahedronViewMat[i];
					light.shadowMat[i+1] = remapMat * lightViewProjMat;
					shadowMatrices.push_back(light.shadowMat[i+1]);
					gPrepassTetrahedronMaterial->SetParameter(ShaderNameBuilder("lightViewProjMat")[i].c_str(), lightViewProjMat);
				}
			}
			else
			{
				Matrix4 lightProjMat = MakeMatrixPerspectiveProj(
					DegToRad(90.f),
					(float)gShadowCubeTexArray.width, (float)gShadowCubeTexArray.height,
					lightNearPlane, light.radius,
					viewPoint.jitterX, viewPoint.jitterY);

				light.shadowMat[0] = light.lightViewMat * viewPoint.invViewMat;
				light.shadowMat[1] = remapMat * lightProjMat;
				shadowMatrices.push_back(light.shadowMat[0]);
				shadowMatrices.push_back(light.shadowMat[1]);

				for (int i = 0; i < 6; ++i)
				{
					gPrepassCubeMaterial->SetParameter(ShaderNameBuilder("lightViewProjMat")[i].c_str(), lightProjMat * gLightCubeViewMat[i]);
				}
				gPrepassCubeMaterial->SetParameter("cubeMapArrayIndex", lightData.shadowMapIndex);
			}

			bool bHasMeshToRender = false;
			// process scene bounds and do frustum culling
			for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
			{
				MeshComponent*& meshComp = MeshComponent::gMeshComponentContainer[i];
				if (IsOBBIntersectSphere(
					meshComp->OBB.permutedAxisCenter, meshComp->OBB.center, meshComp->OBB.extent,
					light.position, light.radius))
				{
					bHasMeshToRender = true;
					meshComp->bRenderVisibile = true;
				}
				else
				{
					meshComp->bRenderVisibile = false;
				}
			}

			if (bHasMeshToRender)
			{
				DrawShadowScene(renderContext, 0, shadowRenderInfo, 
					lightData.bUseTetrahedronShadowMap ? gPrepassTetrahedronMaterial : gPrepassCubeMaterial,
					MeshComponent::gMeshComponentContainer);
			}
		}
	}

	if (!bCubeMapPass)
	{
		// disable custom clipping planes
		glDisable(GL_CLIP_DISTANCE0);
		glDisable(GL_CLIP_DISTANCE1);
		glDisable(GL_CLIP_DISTANCE2);
		glDisable(GL_CLIP_DISTANCE3);
	}

	assert(shadowMatrices.size() == gCurLocalLightShadowMatCount);

	// set max local light dist
	globalLightsRenderInfo.maxLocalLightDist = maxLocalLightDist;

	// global light ubo
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_GlobalLights);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GlobalLightsRenderInfo), &globalLightsRenderInfo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	// send ssbo
	bool bNeedReallocate = false;
	if (visibleLightCount > gPrevLocalLightCapacity)
	{
		gPrevLocalLightCapacity = visibleLightCount;
		bNeedReallocate = true;
	}
	// notice the offset of light array is sizeof(int) * 4 because of data layout
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LocalLights);
	//if (bNeedReallocate)
	//	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LightRenderInfo) * visibleLightCount + sizeof(int) * 4, NULL, GL_DYNAMIC_DRAW);
	//glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &visibleLightCount);
	//glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 4, sizeof(LightRenderInfo) * visibleLightCount, localLights.data());
	if (bNeedReallocate)
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(LightRenderInfo) * visibleLightCount, NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(LightRenderInfo) * visibleLightCount, localLights.data());

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LocalLightsBounds);
	if (bNeedReallocate)
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Vector4) * visibleLightCount, NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Vector4) * visibleLightCount, localLightsBounds.data());

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gSSBO_LocalLightsShadowMatrices);
	if (gCurLocalLightShadowMatCount <= gPrevLocalLightShadowMatCapacity)
	{
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Matrix4) * gCurLocalLightShadowMatCount, shadowMatrices.data());
	}
	else
	{
		gPrevLocalLightShadowMatCapacity = gCurLocalLightShadowMatCount;
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Matrix4) * gCurLocalLightShadowMatCount, shadowMatrices.data(), GL_DYNAMIC_DRAW);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


void SkyboxPass(RenderContext& renderContext)
{
	const static RenderState renderState([](RenderState& s) {
		// depth
		s.bDepthTest = true;
		s.depthTestFunc = GL_LEQUAL;
		s.bDepthWrite = false;
		// draw back face
		s.bCullFace = true;
		s.cullFaceMode = GL_FRONT;
	});

	renderState.Apply(renderContext);

	//gSkyboxMaterial->SetParameter("cubeMap", gPointLights[1].shadowData[0].shadowMap);
	gSkyboxMaterial->SetParameter("cubeMap", gSkyboxMap);

	gCubeMesh->Draw(renderContext, gSkyboxMaterial);
}

void AlphaBlendPass(RenderContext& renderContext)
{
	//CPU_SCOPED_PROFILE("alpha blend");
	GPU_SCOPED_PROFILE("alpha blend");

	const static RenderState renderState([](RenderState& s) {
		// don't write depth
		s.bDepthWrite = false;
	});

	renderState.Apply(renderContext);

	// enable blend for skybox additive
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	// sort
	std::sort(gAlphaBlendMeshRenderList.begin(), gAlphaBlendMeshRenderList.end(), MeshRenderData::CompareAlphaBlend);

	// draw mesh
	for (int mi = 0, nmi = (int)gAlphaBlendMeshRenderList.size(); mi < nmi; ++mi)
	{
		Material* material = gAlphaBlendMeshRenderList[mi].material;

		DrawMesh(renderContext, gAlphaBlendMeshRenderList[mi]);
	}

	glDisable(GL_BLEND);
}

void DebugForwardPass(RenderContext& renderContext)
{
	//GPU_SCOPED_PROFILE("debug foward");

	static const RenderState renderState;

	renderState.Apply(renderContext);

	// draw debug
	for (int i = 0; i < gPointLights.size(); ++i)
	{
		Matrix4 modelMat = Matrix4::Identity();
		modelMat.SetTranslation(gPointLights[i].position);
		modelMat.ApplyScale(0.3f);
		gLightDebugMaterial->SetParameter("modelMat", modelMat);
		gLightDebugMaterial->SetParameter("color", gPointLights[i].colorIntensity, 3);

		gIcosahedronMesh->Draw(renderContext, gLightDebugMaterial);
	}

	for (int i = 0; i < gSpotLights.size(); ++i)
	{
		Matrix4 modelMat = MakeMatrixFromForward(gSpotLights[i].direction);
		modelMat.SetTranslation(gSpotLights[i].position);
		modelMat.ApplyScale(Vector4_3(gSpotLights[i].endRadius, gSpotLights[i].radius, gSpotLights[i].endRadius) * (0.6f / gSpotLights[i].radius));
		gLightDebugMaterial->SetParameter("modelMat", modelMat);
		gLightDebugMaterial->SetParameter("color", gSpotLights[i].colorIntensity, 3);

		gConeMesh->Draw(renderContext, gLightDebugMaterial);
	}

	// wireframes
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);

	if (gRenderSettings.bDrawBounds)
	{
		// bounds
		for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
		{
			// draw bounds
			MeshComponent*& meshComp = MeshComponent::gMeshComponentContainer[i];
			Vector4_3 center = meshComp->bounds.GetCenter();
			Vector4_3 extent = meshComp->bounds.GetExtent();

			Matrix4 boundMat = Matrix4::Identity();
			boundMat.SetTranslation(center);
			boundMat.ApplyScale(extent);

			Matrix4 modelMat = meshComp->modelMat * boundMat;

			gLightDebugMaterial->SetParameter("modelMat", modelMat);
			gLightDebugMaterial->SetParameter("color", Vector4_3(1, 0, 0), 3);

			gCubeMesh->Draw(renderContext, gLightDebugMaterial);
		}
	}

#if 0
	Light& light = gSpotLights[0];
	Plane frustumPlanes[6];

	const float lightNearPlane = 0.01f;
	Matrix4 lightProjMat = MakeMatrixPerspectiveProj(
		DegToRad(light.outerHalfAngle) * 2,
		512, 512,
		lightNearPlane, light.radius,
		0, 0);

	GetFrustumPlanes(light.lightViewMat, lightProjMat.m[0][0], lightProjMat.m[1][1],
		lightNearPlane, light.radius, frustumPlanes);

	for (int i = 0, ni = (int)MeshComponent::gMeshComponentContainer.size(); i < ni; ++i)
	{
		MeshComponent*& meshComp = MeshComponent::gMeshComponentContainer[i];
		if (IsOBBIntersectFrustum(meshComp->OBB.permutedAxisCenter, meshComp->OBB.extent, frustumPlanes, 6))
		//if (IsOBBIntersectSphere(
		//	meshComp->OBB.permutedAxisCenter, meshComp->OBB.center, meshComp->OBB.extent,
		//	light.position, light.radius))
		{
			Vector4_3 center = meshComp->bounds.GetCenter();
			Vector4_3 extent = meshComp->bounds.GetExtent();

			Matrix4 boundMat = Matrix4::Identity();
			boundMat.SetTranslation(center);
			boundMat.ApplyScale(extent);

			Matrix4 modelMat = meshComp->modelMat * boundMat;

			gLightDebugMaterial->SetParameter("modelMat", modelMat);
			gLightDebugMaterial->SetParameter("color", Vector4_3(1, 0, 0), 3);

			gCubeMesh->Draw(renderContext, gLightDebugMaterial);
		}
	}
#endif

	if (gRenderSettings.bDrawLightVolume)
	{
		for (int i = 0; i < gPointLights.size(); ++i)
		{
			// model
			Matrix4 modelMat = Matrix4::Identity();
			modelMat.SetTranslation(gPointLights[i].position);
			modelMat.ApplyScale(gPointLights[i].radius);

			gLightDebugMaterial->SetParameter("modelMat", modelMat);
			gLightDebugMaterial->SetParameter("color", gPointLights[i].colorIntensity, 3);

			gIcosahedronMesh->Draw(renderContext, gLightDebugMaterial);
		}

		for (int i = 0; i < gSpotLights.size(); ++i)
		{
			// model
			Matrix4 modelMat = MakeMatrixFromForward(gSpotLights[i].direction);
			modelMat.SetTranslation(gSpotLights[i].position);
			modelMat.ApplyScale(gSpotLights[i].endRadius, gSpotLights[i].radius, gSpotLights[i].endRadius);

			gLightDebugMaterial->SetParameter("modelMat", modelMat);
			gLightDebugMaterial->SetParameter("color", gSpotLights[i].colorIntensity, 3);

			gConeMesh->Draw(renderContext, gLightDebugMaterial);
		}

		//for (int i = 0; i < gPointLights.size(); ++i)
		//{
		//	// model
		//	Matrix4 modelMat = Matrix4::Identity();
		//	modelMat.SetTranslation(gSpotLights[i].sphereBounds.centerRadius);
		//	modelMat.ApplyScale(gSpotLights[i].sphereBounds.centerRadius.w);

		//	gLightDebugMaterial->SetParameter("modelMat", modelMat);
		//	gLightDebugMaterial->SetParameter("color", gPointLights[i].colorIntensity, 3);

		//	gIcosahedronMesh->Draw(renderContext, gLightDebugMaterial);
		//}

		//for (int i = 0; i < gSpotLights.size(); ++i)
		//{
		//	// model
		//	Matrix4 modelMat = MakeMatrixFromForward(gSpotLights[i].direction);
		//	modelMat.SetTranslation(gSpotLights[i].sphereBounds.centerRadius);
		//	modelMat.ApplyScale(gSpotLights[i].sphereBounds.centerRadius.w);

		//	gLightDebugMaterial->SetParameter("modelMat", modelMat);
		//	gLightDebugMaterial->SetParameter("color", gSpotLights[i].colorIntensity, 3);

		//	gIcosahedronMesh->Draw(renderContext, gLightDebugMaterial);
		//}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SwapSceneDepthHistory()
{
	int tmp = gSceneDepthCurrentIdx;
	gSceneDepthCurrentIdx = gSceneDepthHistoryIdx;
	gSceneDepthHistoryIdx = tmp;
}

void SwapSceneColorReadWrite()
{
	int tmp = gSceneColorReadIdx;
	gSceneColorReadIdx = gSceneColorWriteIdx;
	gSceneColorWriteIdx = tmp;
	if (gSceneColorWriteIdx == gSceneColorHistoryIdx)
		gSceneColorWriteIdx = gSceneColorPrevHistoryIdx;
}

void PreparePostProcessPass(bool bClear = false)
{
	// swap
	SwapSceneColorReadWrite();
	// bind read texture
	gSceneColorTex[gSceneColorReadIdx].Bind(Shader::gSceneColorTexUnit);
	// attach write texture
	gSceneBuffer.AttachColor(&gSceneColorTex[gSceneColorWriteIdx], 0);
	if (bClear)
	{
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}
void PostProcessPassBeforeAlpa(RenderContext& renderContext)
{
	//GPU_SCOPED_PROFILE("post process before alpha");

	const static RenderState renderState([](RenderState& s) {
		// no depth test
		s.bDepthTest = false;
	});

	renderState.Apply(renderContext);

	// bind depth stencil tex
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);

	// SSR
	if (gRenderSettings.bSSR)
	{
		GPU_SCOPED_PROFILE("SSR");

		PreparePostProcessPass();

#if 1
		gSSRMaterial->SetParameter("albedoTex", &gAlbedoTex);
		gSSRMaterial->SetParameter("normalTex", &gNormalTex);
		gSSRMaterial->SetParameter("materialTex", &gMaterialTex);
		gSSRMaterial->SetParameter("skyTex", gSkyboxMap);

		// draw quad
		gFSQuadMesh->Draw(renderContext, gSSRMaterial);
#else
		const static RenderState renderStateSSRStencil([](RenderState& s) {
			// don't test depth
			s.bDepthTest = false;
			// write stencil
			s.bStencilTest = true;
			s.stencilTestFunc = GL_ALWAYS;
			s.stencilTestRef = 1;
			s.stencilWriteDPass = GL_REPLACE;
		});

		const static RenderState renderStateSSR([](RenderState& s) {
			// depth test less
			s.bDepthTest = true;
			s.depthTestFunc = GL_GREATER;
			s.bDepthWrite = false;
			// test stencil
			s.bStencilTest = true;
			s.stencilTestFunc = GL_NOTEQUAL;
			s.stencilTestRef = 1;
		});

		renderStateSSRStencil.Apply(renderContext);

		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);

		gSSRStencilMaterial->SetParameter("materialTex", &gMaterialTex);

		// draw quad
		gFSQuadMesh->Draw(renderContext, gSSRStencilMaterial);

		renderStateSSR.Apply(renderContext);

		gSSRMaterial->SetParameter("albedoTex", &gAlbedoTex);
		gSSRMaterial->SetParameter("normalTex", &gNormalTex);
		gSSRMaterial->SetParameter("materialTex", &gMaterialTex);
		gSSRMaterial->SetParameter("skyTex", gSkyboxMap);

		// draw quad
		gFSQuadMesh->Draw(renderContext, gSSRMaterial);

		renderState.Apply(renderContext);
#endif
	}

}

void PostProcessPass(RenderContext& renderContext)
{
	GPU_SCOPED_PROFILE("post process");

	const static RenderState renderState( [] (RenderState& s) {
		// no depth test
		s.bDepthTest = false;
	});

	renderState.Apply(renderContext);

	// bind depth stencil tex
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);

	// bind post process pass textures
	//gSceneDepthStencilTex[gSceneDepthCurrentIdx].Bind(Shader::gDepthStencilTexUnit);

	// TAA
	if(gRenderSettings.bUseTAA)
	{
		GPU_SCOPED_PROFILE("TAA");

		PreparePostProcessPass();
		// for the first frame, just use the same frame for history
		if (gHasResetFrame)
		{
			gTAAMaterial->SetParameter("historyColorTex", &gSceneColorTex[gSceneColorReadIdx]);
			//gTAAMaterial->SetParameter("historyDepthStencilTex", &gSceneDepthStencilTex[gSceneDepthCurrentIdx]);
		}
		else
		{
			gTAAMaterial->SetParameter("historyColorTex", &gSceneColorTex[gSceneColorHistoryIdx]);
			//gTAAMaterial->SetParameter("historyDepthStencilTex", &gSceneDepthStencilTex[gSceneDepthHistoryIdx]);
		}
		gTAAMaterial->SetParameter("velocityTex", &gVelocityTex);
#if 0
		// calculate weights
		{
			static const float SampleOffsets[5][2] =
			{
				{ 0.0f,  0.0f },
				{ -1.0f,  0.0f },
				{ 1.0f,  0.0f },
				{ 0.0f, -1.0f },
				{ 0.0f,  1.0f },
			};
			float plusWeights[5];

			float totalWeight  = 0;
			for (int i = 0; i < 5; ++i)
			{
				float PixelOffsetX = SampleOffsets[i][0] - renderContext.viewPoint.jitterX;
				float PixelOffsetY = SampleOffsets[i][1] - renderContext.viewPoint.jitterY;
				// Normal distribution, Sigma = 0.47
				plusWeights[i] = exp(-2.29f * (PixelOffsetX * PixelOffsetX + PixelOffsetY * PixelOffsetY));
				totalWeight += plusWeights[i];
			}

			for (int i = 0; i < 5; ++i)
			{
				gTAAMaterial->SetParameter(ShaderNameBuilder("plusWeights")[i].c_str(), plusWeights[i] / totalWeight);
			}
		}
#endif

		// draw quad
		gFSQuadMesh->Draw(renderContext, gTAAMaterial);

		gSceneColorPrevHistoryIdx = gSceneColorHistoryIdx;
		gSceneColorHistoryIdx = gSceneColorWriteIdx;

		SwapSceneDepthHistory();
	}
	
}

void ToneMapPass(RenderContext& renderContext)
{
	const static RenderState renderState([](RenderState& s) {
		// no depth test
		s.bDepthTest = false;
	});

	renderState.Apply(renderContext);

	// tone mapping
	{
		// draw quad
		gFSQuadMesh->Draw(renderContext, gToneMapMaterial);
	}
}

void VisualizeTexturePass(RenderContext& renderContext)
{
	const static RenderState renderState([](RenderState& s) {
		// no depth test
		s.bDepthTest = false;
	});

	renderState.Apply(renderContext);
	
	// unbind frame buffer, draw to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Texture2D* vTex = &gShadowTiledTex;//(Texture2D*)(gPointLights[2].shadowData[0].shadowMap);
	//assert(gPointLights[2].shadowData[0].shadowMap->textureType == GL_TEXTURE_2D);
	//assert(vTex->textureType == GL_TEXTURE_2D);
	gVisualizeTextureMaterial->SetParameter("depthParams", 
		//Vector4_3(renderContext.viewPoint.nearPlane, renderContext.viewPoint.farPlane, 1.f), 3);
		Vector4_3(0.01f, 40.f, 1.f), 3);
		//Vector4_3(0.f, 0.f, 0.f), 3);
	gVisualizeTextureMaterial->SetParameter("texSize", Vector4_2((float)vTex->width, (float)vTex->height), 2);
	gVisualizeTextureMaterial->SetParameter("tex", vTex);

	// draw quad
	gFSQuadMesh->Draw(renderContext, gVisualizeTextureMaterial);
}

void UIPass()
{
	//GPU_SCOPED_PROFILE("UI");

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
			float timeRatio = Clamp((float)(it->second / averageFrameTime), 0.0f, 1.0f);
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
			float timeRatio = Clamp((float)(it->second / averageFrameTime), 0.0f, 1.0f);
			ImGui::Text("%s \t %.3f ms %.2f%%", displayName.c_str(), it->second, timeRatio * 100);
			ImGui::ProgressBar(timeRatio, ImVec2(0.f, 5.f));
		}

		ImGui::End();
	}

	{
		static bool open = true;
		ImGui::Begin("settings", &open);

		ImGui::Checkbox("Shadow", &gRenderSettings.bDrawShadow);
		ImGui::Checkbox("- CSM", &gRenderSettings.bDrawShadowCSM);
		ImGui::Checkbox("- Spot", &gRenderSettings.bDrawShadowSpot);
		ImGui::Checkbox("- Point", &gRenderSettings.bDrawShadowPoint);
		ImGui::Checkbox("Bounds", &gRenderSettings.bDrawBounds);
		ImGui::Checkbox("Light Volume", &gRenderSettings.bDrawLightVolume);
		ImGui::Checkbox("TAA", &gRenderSettings.bUseTAA);
		ImGui::Checkbox("Jitter", &gRenderSettings.bUseJitter);
		ImGui::Checkbox("SSAO", &gRenderSettings.bSSAO);
		ImGui::Checkbox("SSR", &gRenderSettings.bSSR);
		ImGui::Checkbox("Skybox", &gRenderSettings.bSkybox);
		ImGui::Checkbox("Tile Based", &gRenderSettings.bTileBasedDeferred);
		ImGui::Checkbox("Tile Info", &gRenderSettings.bTileInfo);
		ImGui::Checkbox("Tile One Pass", &gRenderSettings.bTileOnePass);

		ImGui::End();
	}

	//{
	//	static bool open = true;
	//	ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
	//	ImGui::ShowTestWindow(&open);
	//}

#if SHADER_DEBUG_BUFFER
	glBindTexture(GL_TEXTURE_2D, gDebugTex.textureID);
	glGetTexImage(GL_TEXTURE_2D, 0, gDebugTex.format, gDebugTex.type, gDebugTexBuffer);

	int x, y;
	SDL_GetMouseState(&x, &y);

	float* buffer = gDebugTexBuffer + ((gWindowHeight - 1 - y) * gWindowWidth + x) * 4;

	{
		static bool open = true;
		ImGui::Begin("shader debug", &open);

		ImGui::Text("pos : (%d, %d) (%.2f, %.2f)", x, y, (float)x / gWindowWidth, (float)(gWindowHeight - 1 - y) / gWindowHeight);
		ImGui::Text("R: %f", buffer[0]);
		ImGui::Text("G: %f", buffer[1]);
		ImGui::Text("B: %f", buffer[2]);
		ImGui::Text("A: %f", buffer[3]);

		ImGui::End();
	}
#endif

	ImGui::Render();
}

void Render()
{
	GPU_SCOPED_PROFILE("render");
	CPU_SCOPED_PROFILE("render");

	RenderContext renderContext;
	float jitterX = 0, jitterY = 0;
	if (gRenderSettings.bUseJitter)
	{
		jitterX = gJitter[gJitterIdx].x;
		jitterY = -gJitter[gJitterIdx].y;

		++gJitterIdx;
		if (gJitterIdx >= gJitterCount)
			gJitterIdx -= gJitterCount;
	}
	renderContext.viewPoint = gCamera.ProcessCamera((GLfloat)gWindowWidth, (GLfloat)gWindowHeight, 0.1f, 200.f, jitterX, jitterY);
	
	// cull lights
	CullLights(renderContext);
	
	// bind shadow buffer
	gDepthOnlyBuffer.Bind();
	ShadowPass(renderContext);

	glViewport(0, 0, gWindowWidth, gWindowHeight);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// update ubo
	gRenderInfo.PrevView = gRenderInfo.View;
	gRenderInfo.PrevProj = gRenderInfo.Proj;
	gRenderInfo.PrevViewProj = gRenderInfo.ViewProj;
	gRenderInfo.View = renderContext.viewPoint.viewMat;
	gRenderInfo.InvView = renderContext.viewPoint.invViewMat;
	gRenderInfo.Proj = renderContext.viewPoint.projMat;
	gRenderInfo.ViewProj = renderContext.viewPoint.viewProjMat;
	if (gHasResetFrame)
	{
		gRenderInfo.PrevView = gRenderInfo.View;
		gRenderInfo.PrevProj = gRenderInfo.Proj;
		gRenderInfo.PrevViewProj = gRenderInfo.ViewProj;

		gRenderInfo.TileCountX = (gWindowWidth + RenderConsts::lightTileSize - 1) / RenderConsts::lightTileSize;
		gRenderInfo.TileCountY = (gWindowHeight + RenderConsts::lightTileSize - 1) / RenderConsts::lightTileSize;
	}
	gRenderInfo.Resolution.x = renderContext.viewPoint.width;
	gRenderInfo.Resolution.y = renderContext.viewPoint.height;
	gRenderInfo.Resolution.z = renderContext.viewPoint.nearPlane;
	gRenderInfo.Resolution.w = renderContext.viewPoint.farPlane;
	gRenderInfo.Time = (float)gTime;
	gRenderInfo.Exposure = 1.0f;
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RenderInfo), &gRenderInfo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	if (gHasResetFrame)
		SetupLightTileBuffer(gRenderInfo.TileCountX * gRenderInfo.TileCountY);
	
	// bind G-buffer
	gGBuffer.Bind();

	GeometryPass(renderContext);

	// copy depth to rbo
	// gGBuffer is still binding to read buffer
	gDepthCopyBuffer.Bind(GL_DRAW_FRAMEBUFFER);
	glBlitFramebuffer(0, 0, gWindowWidth, gWindowHeight, 0, 0, gWindowWidth, gWindowHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	
	// bind deferred pass textures
	gNormalTex.Bind(Shader::gNormalTexUnit);
	gMaterialTex.Bind(Shader::gMaterialTexUnit);
	gDepthStencilTex.Bind(Shader::gDepthStencilTexUnit);

	// bind SSAO-buffer
	if (gRenderSettings.bSSAO)
	{
		gSSAOBuffer.Bind();
		SSAOPass(renderContext);
	}

	gAlbedoTex.Bind(Shader::gAlbedoTexUnit);

	gCSMTexArray.Bind(Shader::gCSMTexArrayUnit);
	gShadowTiledTex.Bind(Shader::gShadowTiledTexUnit);
	gShadowCubeTexArray.Bind(Shader::gShadowCubeTexArrayUnit);

	if (gRenderSettings.bTileBasedDeferred && !gRenderSettings.bTileInfo)
		TileBasedDeferredLightPass(renderContext);

	// bind Scene-buffer
	gSceneBuffer.Bind();
	// attach write texture
	gSceneBuffer.AttachColor(&gSceneColorTex[gSceneColorWriteIdx], 0);
	//gSceneBuffer.AttachDepth(&gSceneDepthStencilTex[gSceneDepthCurrentIdx], true);

	if (gRenderSettings.bTileBasedDeferred && gRenderSettings.bTileInfo)
		TileBasedDeferredLightPass(renderContext);

	if (!gRenderSettings.bTileBasedDeferred)
		LightPass(renderContext);

	if(gRenderSettings.bSkybox)
		SkyboxPass(renderContext);

	// post process before alpha
	PostProcessPassBeforeAlpa(renderContext);

	// alpha blend
	AlphaBlendPass(renderContext);

	// detach depth texture
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	
	PostProcessPass(renderContext);

	// unjitter
	RenderInfo debugRenderInfo = gRenderInfo;
	debugRenderInfo.Proj.m[2][0] = 0;
	debugRenderInfo.Proj.m[2][1] = 0;
	debugRenderInfo.ViewProj = debugRenderInfo.Proj * debugRenderInfo.View;
	glBindBuffer(GL_UNIFORM_BUFFER, gUBO_Matrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RenderInfo), &debugRenderInfo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	DebugForwardPass(renderContext);
	
	// swap read/write, get index ready for next frame
	SwapSceneColorReadWrite();
	// bind read texture
	gSceneColorTex[gSceneColorReadIdx].Bind(Shader::gSceneColorTexUnit);
	// unbind frame buffer, draw to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ToneMapPass(renderContext);

#if VISUALIZE_TEXTURE
	VisualizeTexturePass(renderContext);
#endif

	UIPass();
}

void ProcessShaderReload()
{
	static REArray<FileChangeResult> results;
	static RESet<std::string> changedFiles;
	static RESet<Shader*> changedShaders;
	results.clear();
	changedFiles.clear();
	changedShaders.clear();
	fileWatcher.Update(results);
	for (int i = 0; i < results.size(); ++i)
	{
		FileChangeResult& result = results[i];
		if (result.type == EFileChangeType::FileUpdated)
		{
			printf("update %d: %s\n", (int)results[i].type, results[i].fileName.c_str());
			auto it = gShaderFileCache.find(result.fileName);
			if (it != gShaderFileCache.end())
			{
				auto pair = changedFiles.insert(result.fileName);
				if (pair.second)
				{
					ShaderFileInfo& shaderFileInfo = gShaderFileCache[result.fileName];
					for (const std::string& childFile : shaderFileInfo.childFiles)
					{
						changedFiles.insert(childFile);
					}
				}
			}
			else
			{
				printf("file not found in shader cache %d: %s\n", (int)results[i].type, results[i].fileName.c_str());
			}
		}
	}
	for (auto& changedFile : changedFiles)
	{
		ShaderFileInfo& shaderFileInfo = gShaderFileCache[changedFile];
		shaderFileInfo.shaderInfo.bNeedReload = true;
		for (Shader* shader : shaderFileInfo.shaders)
		{
			changedShaders.insert(shader);
		}
	}
	for (Shader* shader : changedShaders)
	{
		shader->Reload();
		for (Material* material : shader->referenceMaterials)
		{
			material->Reload();
		}
	}
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
						for (int i = 0, ni = (int)Material::gMaterialContainer.size(); i < ni; ++i)
							Material::gMaterialContainer[i]->Reload();
						//Mesh** meshlContainerPtr = Mesh::gMeshContainer.data();
						//for (int i = 0, ni = (int)Mesh::gMeshContainer.size(); i < ni; ++i)
						//	meshlContainerPtr[i]->SetAttributes();
					}
#if LOAD_CACHE_SIM
					else if (event.key.keysym.sym == SDLK_c)
					{
						bCacheSimCaptureFrame = true;
					}
#endif
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

#if LOAD_CACHE_SIM
			if(bCacheSimCaptureFrame)
				cachesim.Start();
#endif

			ProcessShaderReload();

			Update(gLastDeltaTime);

			Render();

			SDL_GL_SwapWindow(gWindow);
			
			// time
			Uint64 newPerformanceCounter = SDL_GetPerformanceCounter();
			Uint64 diff = (newPerformanceCounter - gPerformanceCounter);
			if (newPerformanceCounter < gPerformanceCounter)
			{
				static const Uint64 max = Uint64(-1);
				diff = newPerformanceCounter + (max - gPerformanceCounter);
			}
			double deltaTime = (double)(newPerformanceCounter - gPerformanceCounter) * gInvPerformanceFreq;
			gTime += deltaTime;
			gLastDeltaTime = (float)deltaTime;
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

			gHasResetFrame = false;

#if LOAD_CACHE_SIM
			if (bCacheSimCaptureFrame)
			{
				bCacheSimCaptureFrame = false;
				cachesim.End();
			}
#endif
		}
	}

	Close();

	return EXIT_SUCCESS;
}
