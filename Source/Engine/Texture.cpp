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