// glew
#include "gl/glew.h"

#include "Texture.h"

void Texture::AttachToFrameBuffer(GLenum attachment)
{
	glFramebufferTexture(GL_FRAMEBUFFER, attachment, textureID, 0);
}

void Texture::Bind(GLuint textureUnitOffset)
{
	glActiveTexture(GL_TEXTURE0 + textureUnitOffset);
	glBindTexture(textureType, textureID);
}

void Texture::BindImage(GLuint imageUnit)
{
	glBindImageTexture(imageUnit, textureID, 0, GL_FALSE, 0, access, internalFormat);
}

bool Texture::HasAlpha()
{
	return format == GL_RGBA || format == GL_ALPHA;
}