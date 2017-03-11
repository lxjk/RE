#pragma once

// sdl
#include "SDL_image.h"

// glew
#include "gl/glew.h"

// opengl
#include "SDL_opengl.h"

#include <string>

class Texture2D
{
public:

	void Load(const char* name, 
		GLint wrapS = GL_CLAMP_TO_EDGE, GLint wrapT = GL_CLAMP_TO_EDGE,
		GLint minFilter = GL_LINEAR_MIPMAP_LINEAR, GLint magFilter = GL_NEAREST_MIPMAP_NEAREST)
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

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->w, image->h, 0, GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glBindTexture(GL_TEXTURE_2D, 0);
		SDL_FreeSurface(image);
	}

	void Bind(GLuint textureUnit)
	{
		glActiveTexture(textureUnit);
		glBindTexture(GL_TEXTURE_2D, textureID);
	}

protected:
	GLuint textureID = 0;
};