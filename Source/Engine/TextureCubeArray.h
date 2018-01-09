#pragma once

// opengl
#include "SDL_opengl.h"

#include "Containers/Containers.h"

#include "Texture.h"

class TextureCubeArray : public Texture
{
public:

	static REArray<TextureCubeArray*> gTextureCubeContainer;

	static TextureCubeArray* Create()
	{
		TextureCubeArray* tex = new TextureCubeArray();
		gTextureCubeContainer.push_back(tex);
		return tex;
	}

	TextureCubeArray() : Texture()
	{
		textureType = GL_TEXTURE_CUBE_MAP_ARRAY;
	}

	// here count is cube map count, not face count
	void AllocateForFrameBuffer(int width, int height, int count, GLint internalFormat, GLenum format, GLenum type, bool bLinearFilter = false);

	// here count is cube map count, not face count
	void Reallocate(int width, int height, int count);
};
