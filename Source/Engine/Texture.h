#pragma once

// opengl
#include "SDL_opengl.h"

class Texture
{
public:

	Texture()
	{
		textureType = 0;
		textureID = GL_INVALID_VALUE;
		width = 0;
		height = 0;
		internalFormat = 0;
		format = 0;
		type = 0;
		access = GL_READ_ONLY;
		path[0] = 0;
	}

	void AttachToFrameBuffer(GLenum attachment);
	void Bind(GLuint textureUnitOffset);
	void BindImage(GLuint imageUnit);

	bool HasAlpha();

	GLenum textureType;
	GLuint textureID;
	int width;
	int height;
	int count = 0; // used for arrays
	GLint internalFormat;
	GLenum format;
	GLenum type;
	GLenum access; // used for image
	char path[256];
};