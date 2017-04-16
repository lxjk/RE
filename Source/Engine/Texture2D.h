#pragma once

// opengl
#include "SDL_opengl.h"

#include <vector>
#include <string>

class Texture2D
{
public:

	static std::vector<Texture2D*> gTexture2DContainer;

	static Texture2D* Create()
	{
		Texture2D* tex = new Texture2D();
		gTexture2DContainer.push_back(tex);
		return tex;
	}

	static Texture2D* FindOrCreate(const char* name, bool bSRGB,
		GLint wrapS = GL_CLAMP_TO_EDGE, GLint wrapT = GL_CLAMP_TO_EDGE,
		GLint minFilter = GL_LINEAR_MIPMAP_LINEAR, GLint magFilter = GL_LINEAR)
	{
		for (int i = 0; i < gTexture2DContainer.size(); ++i)
		{
			if (strcmp(gTexture2DContainer[i]->path, name) == 0)
				return gTexture2DContainer[i];
		}
		Texture2D* tex = Create();
		tex->Load(name, bSRGB, wrapS, wrapT, minFilter, magFilter);
		return tex;
	}

	Texture2D()
	{
		textureID = 0;
		width = 0;
		height = 0;
		internalFormat = 0;
		format = 0;
		type = 0;
		path[0] = 0;
	}

	void Load(const char* name, bool bSRGB,
		GLint wrapS = GL_CLAMP_TO_EDGE, GLint wrapT = GL_CLAMP_TO_EDGE,
		GLint minFilter = GL_LINEAR_MIPMAP_LINEAR, GLint magFilter = GL_LINEAR);

	void AllocateForFrameBuffer(int width, int height, GLint internalFormat, GLenum format, GLenum type, bool bShadowMap = false);

	void Reallocate(int width, int height);

	void AttachToFrameBuffer(GLenum attachment);

	void Bind(GLuint textureUnitOffset);

	GLuint textureID;
	int width;
	int height;
	GLint internalFormat;
	GLenum format;
	GLenum type;
	char path[512];
};
