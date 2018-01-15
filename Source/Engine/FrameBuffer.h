#pragma once

// glew
#include "gl/glew.h"

// opengl
#include "SDL_opengl.h"

#include <stdio.h>

#include "Texture.h"

class FrameBuffer
{
public:
	
	inline void Bind(GLenum target = GL_FRAMEBUFFER)
	{
		glBindFramebuffer(target, frameBufferID);
	}

	void StartSetup()
	{
		colorAttachmentCount = 0;
		bFinishedSetup = false;

		glGenFramebuffers(1, &frameBufferID);
		Bind();
	}

	void SetupColor(Texture* tex)
	{
		if (bFinishedSetup)
		{
			printf("Error: Call SetupColor after FinishSetup.\n");
			return;
		}
		int location = colorAttachmentCount;
		++colorAttachmentCount;
		if(tex)
			AttachColor(tex, location);
	}

	void SetupColors(Texture** tex, int count)
	{
		if (bFinishedSetup)
		{
			printf("Error: Call SetupColors after FinishSetup.\n");
			return;
		}
		for (int i = 0; i < count; ++i)
			SetupColor(tex[i]);
	}

	void SetupDepth(Texture* tex, bool bWithStencil)
	{
		if (bFinishedSetup)
		{
			printf("Error: Call SetupDepth after FinishSetup.\n");
			return;
		}
		AttachDepth(tex, bWithStencil);
	}

	void SetupDepth(GLuint rbo, bool bWithStencil)
	{
		if (bFinishedSetup)
		{
			printf("Error: Call SetupDepth after FinishSetup.\n");
			return;
		}
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, bWithStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
	}

	void FinishSetup()
	{
		if (colorAttachmentCount == 0)
		{
			glDrawBuffer(GL_NONE);
		}
		else if (colorAttachmentCount > 1)
		{
			GLuint* attachments = new GLuint[colorAttachmentCount];
			for (int i = 0; i < colorAttachmentCount; ++i)
				attachments[i] = GL_COLOR_ATTACHMENT0 + i;

			glDrawBuffers(colorAttachmentCount, attachments);

			delete[] attachments;
		}
		glReadBuffer(GL_NONE);
		bFinishedSetup = true;
	}

	void TestComplete()
	{
		Bind();
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			printf("Error: buffer not complete!\n");
	}

	void AttachColor(Texture* tex, int location)
	{
		if (location >= colorAttachmentCount)
		{
			printf("Error: Attach location out of bound %d of %d\n", location, colorAttachmentCount);
			return;
		}
		if (tex)
		{
			GLenum attachment = GL_COLOR_ATTACHMENT0 + location;
			tex->AttachToFrameBuffer(attachment);
		}
	}

	void AttachDepth(Texture* tex, bool bWithStencil)
	{
		if (tex)
			tex->AttachToFrameBuffer(bWithStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT);
	}

	GLuint frameBufferID;

protected:
	int colorAttachmentCount;
	bool bFinishedSetup;
};