// sdl
#include "SDL_image.h"

// glew
#include "gl/glew.h"

#include <string>

#include "Texture2D.h"

REArray<Texture2D*> Texture2D::gTexture2DContainer;

void Texture2D::Load(const char* name, bool bSRGB, GLint wrapS, GLint wrapT, GLint minFilter, GLint magFilter)
{
	if (textureID == GL_INVALID_VALUE)
		glGenTextures(1, &textureID);

	SDL_Surface* image = IMG_Load(name);
	if (!image)
	{
		printf("Fail to load image %s, error : %s\n", name, IMG_GetError());
		return;
	}


	//printf("format: %s\n", SDL_GetPixelFormatName(image->format->format));

	bool bHasAlpha = SDL_ISPIXELFORMAT_ALPHA(image->format->format);
	//if (SDL_ISPIXELFORMAT_INDEXED(image->format->format))
	if(image->format->format != SDL_PIXELFORMAT_ABGR8888 &&
		image->format->format != SDL_PIXELFORMAT_RGB24)
	{
		SDL_Surface* srcImage = image;
		image = SDL_ConvertSurfaceFormat(srcImage, bHasAlpha ? SDL_PIXELFORMAT_ABGR8888 : SDL_PIXELFORMAT_RGB24, 0);
		SDL_FreeSurface(srcImage);
		if (!image)
		{
			printf("Fail to load image %s, error : %s\n", name, SDL_GetError());
			return;
		}
	}

	// find internal format
	if (bHasAlpha)
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

	this->width = image->w;
	this->height = image->h;
	strcpy_s(path, name);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image->w, image->h, 0, format, type, image->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glBindTexture(GL_TEXTURE_2D, 0);
	SDL_FreeSurface(image);
}

void Texture2D::AllocateForFrameBuffer(int width, int height, GLint internalFormat, GLenum format, GLenum type, bool bLinearFilter)
{
	this->width = width;
	this->height = height;
	this->internalFormat = internalFormat;
	this->format = format;
	this->type = type;

	if (textureID == GL_INVALID_VALUE)
		glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	if(width > 0 && height > 0)
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bLinearFilter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bLinearFilter ? GL_LINEAR : GL_NEAREST);
	//if (bShadowMap)
	//{
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	//}
}

void Texture2D::Reallocate(int width, int height)
{
	if (textureID)
	{
		this->width = width;
		this->height = height;
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
	}
}