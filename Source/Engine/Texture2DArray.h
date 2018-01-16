#pragma once

// opengl
#include "SDL_opengl.h"

#include "Containers/Containers.h"

#include "Texture.h"

class Texture2DArray : public Texture
{
public:

	static REArray<Texture2DArray*> gContainer;

	static Texture2DArray* Create()
	{
		Texture2DArray* tex = new Texture2DArray();
		gContainer.push_back(tex);
		return tex;
	}

	Texture2DArray() : Texture()
	{
		textureType = GL_TEXTURE_2D_ARRAY;
	}

	void AllocateForFrameBuffer(int width, int height, int count, GLint internalFormat, GLenum format, GLenum type, bool bLinearFilter = false);

	void Reallocate(int width, int height, int count);
};
