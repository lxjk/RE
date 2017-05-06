#pragma once

// glew
#include "gl/glew.h"

// opengl
#include "SDL_opengl.h"

// glm
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// std
#include <functional>

// engine
#include "Material.h"
#include "Viewpoint.h"

struct RenderContext
{
	Material* currentMaterial = 0;
	GLint currentVAO = -1;
	Viewpoint viewPoint;
};

struct RenderInfo
{
	glm::mat4 View;			// 16 * 4
	glm::mat4 Proj;			// 16 * 4
	glm::mat4 ViewProj;		// 16 * 4
	glm::mat4 InvView;		// 16 * 4
	glm::mat4 PrevView;		// 16 * 4
	glm::mat4 PrevProj;		// 16 * 4
	glm::mat4 PrevViewProj;	// 16 * 4
	glm::vec2 Resolution;	// 8
	float Time;				// 4
	float Exposure;			// 4
};

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
	GLuint stencilWriteMask = 0xFF;

	bool bCullFace = true;
	GLenum cullFaceMode = GL_BACK;

	RenderState() {}

	// constructor with a init function callback
	RenderState(std::function<void(RenderState&)> initFunc) : RenderState()
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
			glStencilMask(stencilWriteMask);
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

struct RenderSettings
{
	bool bDrawShadow			= true;
	bool bDrawShadowCSM			= true;
	bool bDrawShadowSpot		= true;
	bool bDrawShadowPoint		= true;
	bool bDrawBounds			= false;
	bool bDrawLightVolume		= false;
	bool bUseTAA				= true;
	bool bUseJitter				= true;
	bool bSkybox				= true;

	//RenderSettings() {};
};