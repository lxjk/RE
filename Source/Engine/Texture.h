#pragma once

// opengl
#include "SDL_opengl.h"

class Texture
{
public:

	Texture()
	{
		textureType = 0;
		textureID = 0;
		width = 0;
		height = 0;
		internalFormat = 0;
		format = 0;
		type = 0;
		path[0] = 0;
	}

	void AttachToFrameBuffer(GLenum attachment);
	void Bind(GLuint textureUnitOffset);

	bool HasAlpha();

	GLenum textureType;
	GLuint textureID;
	int width;
	int height;
	GLint internalFormat;
	GLenum format;
	GLenum type;
	char path[256];
};