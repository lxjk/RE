// sdl
#include "SDL_image.h"

// glew
#include "gl/glew.h"

#include <string>

#include "TextureCubeArray.h"

REArray<TextureCubeArray*> TextureCubeArray::gTextureCubeContainer;

void TextureCubeArray::AllocateForFrameBuffer(int width, int height, int count, GLint internalFormat, GLenum format, GLenum type, bool bLinearFilter)
{
	this->width = width;
	this->height = height;
	this->count = count;
	this->internalFormat = internalFormat;
	this->format = format;
	this->type = type;

	if (textureID)
		glDeleteTextures(1, &textureID);
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textureID);
	if (width > 0 && height > 0 && count > 0)
	{
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, internalFormat, width, height, count * 6, 0, format, type, NULL);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, bLinearFilter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, bLinearFilter ? GL_LINEAR : GL_NEAREST);	
}

void TextureCubeArray::Reallocate(int width, int height, int count)
{
	if (textureID)
	{
		this->width = width;
		this->height = height;
		this->count = count;
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textureID);
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, internalFormat, width, height, count * 6, 0, format, type, NULL);
	}
}