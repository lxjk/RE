// sdl
#include "SDL_image.h"

// glew
#include "gl/glew.h"

#include <string>

#include "TextureCube.h"

REArray<TextureCube*> TextureCube::gTextureCubeContainer;

void TextureCube::Load(REArray<const char*> name, bool bSRGB, GLint wrapS, GLint wrapT, GLint wrapR, GLint minFilter, GLint magFilter)
{
	if (name.size() == 0)
		return;

	REArray<SDL_Surface*> images;
	images.reserve(name.size());

	bool bSuccess = true;
	for (int i = 0; i < name.size(); ++i)
	{
		SDL_Surface* image = IMG_Load(name[i]);
		if (!image)
		{
			printf("Fail to load image %s, error : %s\n", name[i], IMG_GetError());
			bSuccess = false;
			break;
		}
		images.push_back(image);
	}

	if (!bSuccess)
	{
		for (int i = 0; i < images.size(); ++i)
		{
			SDL_FreeSurface(images[i]);
		}
		return;
	}
	

	// find internal format
	if (SDL_ISPIXELFORMAT_ALPHA(images[0]->format->format))
	{
		internalFormat = bSRGB ? GL_SRGB_ALPHA : GL_RGBA;
		format = GL_RGBA;
	}
	else
	{
		internalFormat = bSRGB ? GL_SRGB : GL_RGB;
		format = GL_RGB;
	}
	type = GL_UNSIGNED_BYTE;

	this->width = images[0]->w;
	this->height = images[0]->h;
	strcpy_s(path, name[0]);


	if (textureID == GL_INVALID_VALUE)
		glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	for (int i = 0; i < images.size(); ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, images[i]->w, images[i]->h, 0, format, type, images[i]->pixels);
	}
	//glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrapT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrapR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	for (int i = 0; i < images.size(); ++i)
	{
		SDL_FreeSurface(images[i]);
	}
}

void TextureCube::AllocateForFrameBuffer(int width, int height, GLint internalFormat, GLenum format, GLenum type, bool bLinearFilter)
{
	this->width = width;
	this->height = height;
	this->internalFormat = internalFormat;
	this->format = format;
	this->type = type;

	if (textureID == GL_INVALID_VALUE)
		glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	if (width > 0 && height > 0)
	{
		for (int i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, type, NULL);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, bLinearFilter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, bLinearFilter ? GL_LINEAR : GL_NEAREST);	
}

void TextureCube::Reallocate(int width, int height)
{
	if (textureID)
	{
		this->width = width;
		this->height = height;
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
		for (int i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, type, NULL);
	}
}