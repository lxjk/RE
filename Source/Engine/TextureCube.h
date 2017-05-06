#pragma once

// opengl
#include "SDL_opengl.h"

#include <vector>

#include "Texture.h"

class TextureCube : public Texture
{
public:

	static std::vector<TextureCube*> gTextureCubeContainer;

	static TextureCube* Create()
	{
		TextureCube* tex = new TextureCube();
		gTextureCubeContainer.push_back(tex);
		return tex;
	}

	TextureCube() : Texture()
	{
		textureType = GL_TEXTURE_CUBE_MAP;
	}

	void Load(std::vector<const char*> name, bool bSRGB,
		GLint wrapS = GL_CLAMP_TO_EDGE, GLint wrapT = GL_CLAMP_TO_EDGE, GLint wrapR = GL_CLAMP_TO_EDGE,
		GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR);

	void AllocateForFrameBuffer(int width, int height, GLint internalFormat, GLenum format, GLenum type, bool bLinearFilter = false);

	void Reallocate(int width, int height);
};
