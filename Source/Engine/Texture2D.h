#pragma once

// sdl
#include "SDL_image.h"

// glew
#include "gl/glew.h"

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
		GLint minFilter = GL_LINEAR_MIPMAP_LINEAR, GLint magFilter = GL_LINEAR)
	{
		if (!textureID)
		{
			glGenTextures(1, &textureID);
		}

		SDL_Surface* image = IMG_Load(name);
		if (!image)
		{
			printf("Fail to load image %s\n", name);
			return;
		}

		// find internal format
		if (SDL_ISPIXELFORMAT_ALPHA(image->format->format))
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

	void AllocateForFrameBuffer(int width, int height, GLint internalFormat, GLenum format, GLenum type)
	{
		this->width = width;
		this->height = height;
		this->internalFormat = internalFormat;
		this->format = format;
		this->type = type;

		if (textureID)
			glDeleteTextures(1, &textureID);
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	}

	void Reallocate(int width, int height)
	{
		if (textureID)
		{
			this->width = width;
			this->height = height;
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);
		}
	}

	void AttachToFrameBuffer(GLenum attachment)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, textureID, 0);
	}

	void Bind(GLuint textureUnitOffset)
	{
		glActiveTexture(GL_TEXTURE0 + textureUnitOffset);
		glBindTexture(GL_TEXTURE_2D, textureID);
	}

protected:
	GLuint textureID;
	int width;
	int height;
	GLint internalFormat;
	GLenum format;
	GLenum type;
	char path[512];
};

std::vector<Texture2D*> Texture2D::gTexture2DContainer;