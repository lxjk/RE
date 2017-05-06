#pragma once

#include "Texture.h"

#include <vector>

class Texture2D : public Texture
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

	Texture2D() : Texture()
	{
		textureType = GL_TEXTURE_2D;
	}

	void Load(const char* name, bool bSRGB,
		GLint wrapS = GL_CLAMP_TO_EDGE, GLint wrapT = GL_CLAMP_TO_EDGE,
		GLint minFilter = GL_LINEAR_MIPMAP_LINEAR, GLint magFilter = GL_LINEAR);

	void AllocateForFrameBuffer(int width, int height, GLint internalFormat, GLenum format, GLenum type, bool bLinearFilter = false);

	void Reallocate(int width, int height);
};
