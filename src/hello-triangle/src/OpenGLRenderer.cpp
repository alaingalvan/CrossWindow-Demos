#include <glad/glad.h>
#include "Renderer.h"

Renderer::Renderer(xwin::Window& window)
{
	xwin::WindowDesc desc = window.getDesc();
	mWidth = clamp(desc.width, 1u, 0xffffu);
	mHeight = clamp(desc.height, 1u, 0xffffu);

	initializeAPI(window);
	initializeResources();
	setupCommands();
	tStart = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer()
{
	destroyFrameBuffer();

	destroyResources();

	destroyAPI();
}

void Renderer::initializeAPI(xwin::Window& window)
{
	xgfx::OpenGLDesc ogldesc;
	mOGLState = xgfx::createContext(&window, ogldesc);
	xgfx::setContext(mOGLState);
	if (!gladLoadGL())
	{
		// Failed
		std::cout << "Failed to load OpenGL.";
		return;
	}
#if defined(_DEBUG)
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		std::cout << "Error loading OpenGL.";
	}
#endif
}

void Renderer::destroyAPI()
{
	xgfx::unsetContext(mOGLState);
	xgfx::destroyContext(mOGLState);
}

void Renderer::initializeResources()
{
	// OpenGL global setup
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	glGenVertexArrays(1, &mVertexArray);
	glBindVertexArray(mVertexArray);
	glEnableVertexAttribArray(0);

	auto checkShaderCompilation = [&](GLuint shader)
	{
#if defined(_DEBUG)
		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
			std::vector<char> errorLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
			glDeleteShader(shader);

			std::cout << errorLog.data();

			return false;
		}
#endif
		return true;
	};

	std::vector<char> vertShaderCode = readFile("triangle.vert.glsl");
	GLchar* vertStr = vertShaderCode.data();
	GLint vertLen = vertShaderCode.size();
	std::vector<char> fragShaderCode = readFile("triangle.frag.glsl");
	GLchar* fragStr = fragShaderCode.data();
	GLint fragLen = fragShaderCode.size();

	mVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(mVertexShader, 1, &vertStr, &vertLen);
	glCompileShader(mVertexShader);
	if (!checkShaderCompilation(mVertexShader)) return;

	mFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(mFragmentShader, 1, &fragStr, &fragLen);
	glCompileShader(mFragmentShader);
	if (!checkShaderCompilation(mFragmentShader)) return;

	mProgram = glCreateProgram();
	glAttachShader(mProgram, mVertexShader);
	glAttachShader(mProgram, mFragmentShader);
	glLinkProgram(mProgram);

	GLint result = 0;
	glGetProgramiv(mProgram, GL_LINK_STATUS, &result);
#if defined(_DEBUG)
	if (result != GL_TRUE) {
		std::cout << "Program failed to link.";
		return;
	}
#endif
	glUseProgram(mProgram);

	glGenBuffers(1, &mVertexBuffer);
	glGenBuffers(1, &mIndexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 3, mVertexBufferData, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3, mIndexBufferData, GL_STATIC_DRAW);

	mPositionAttrib = glGetAttribLocation(mProgram, "inPos");
	mColorAttrib = glGetAttribLocation(mProgram, "inColor");
	glEnableVertexAttribArray(mPositionAttrib);
	glEnableVertexAttribArray(mColorAttrib);
	glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glVertexAttribPointer(mColorAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

	GLuint matrixBlockIndex = glGetUniformBlockIndex(mProgram, "UBO");
	glUniformBlockBinding(mProgram, matrixBlockIndex, 0);

	glGenBuffers(1, &mUniformUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUniformUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(uboVS), &uboVS, GL_DYNAMIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, mUniformUBO, 0, sizeof(uboVS));
	// Update Uniforms
	uboVS.projectionMatrix = Matrix4::perspective(45.0f, (float)1280 / (float)720, 0.01f, 1024.0f);
	uboVS.viewMatrix = Matrix4::translation(Vector3(0.0f, 0.0f, -2.5f)) * Matrix4::rotationZ(3.14f);
	uboVS.modelMatrix = Matrix4::identity();

	GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p, &uboVS, sizeof(uboVS));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

}

void Renderer::destroyResources()
{
	glDisableVertexAttribArray(mPositionAttrib);
	glDisableVertexAttribArray(mColorAttrib);
	glDeleteShader(mVertexShader);
	glDeleteShader(mFragmentShader);
	glDeleteProgram(mProgram);
	glDeleteVertexArrays(1, &mVertexArray);
	glDeleteBuffers(1, &mVertexBuffer);
	glDeleteBuffers(1, &mIndexBuffer);
	glDeleteBuffers(1, &mUniformUBO);
}

void Renderer::render()
{
	// Framelimit set to 60 fps
	tEnd = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
	if (time < (1000.0f / 60.0f))
	{
		return;
	}
	tStart = std::chrono::high_resolution_clock::now();

	xgfx::swapBuffers(mOGLState);

	// Update Uniforms
	mElapsedTime += 0.001f * time;
	mElapsedTime = fmodf(mElapsedTime, 6.283185307179586f);

	uboVS.modelMatrix = Matrix4::rotationY(mElapsedTime);
	GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p, &uboVS, sizeof(uboVS));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	// Draw
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glViewport(0, 0, mWidth, mHeight);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

	// Blit framebuffer to window
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mFrameBuffer);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, mWidth, mHeight);
	glBlitFramebuffer(0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Renderer::resize(unsigned width, unsigned height)
{
	mWidth = clamp(width, 1u, 0xffffu);
	mHeight = clamp(height, 1u, 0xffffu);

	// Update Unforms
	uboVS.projectionMatrix = Matrix4::perspective(45.0f, static_cast<float>(mWidth) / static_cast<float>(mHeight), 0.01f, 1024.0f);
	GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p, &uboVS, sizeof(uboVS));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	destroyFrameBuffer();
	initFrameBuffer();
}

void Renderer::initFrameBuffer()
{
	glGenTextures(1, &mFrameBufferTex);
	glBindTexture(GL_TEXTURE_2D, mFrameBufferTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenRenderbuffers(1, &mRenderBufferDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, mRenderBufferDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mWidth, mHeight);

	glGenFramebuffers(1, &mFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFrameBufferTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mRenderBufferDepth);

	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);
#if defined(_DEBUG)
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Frame Buffer Failed to be Created!";
	}
#endif
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::destroyFrameBuffer()
{
	glDeleteTextures(1, &mFrameBufferTex);
	glDeleteRenderbuffers(1, &mRenderBufferDepth);
	glDeleteFramebuffers(1, &mFrameBuffer);
}

/**
 * While most modern graphics APIs have a swapchains, command queue, sync, and render passes, OpenGL does not.
 * OpenGL does have frame buffers, but it creates one by default.
 * So these functions are just stubs:
 */

void Renderer::setupSwapchain(unsigned width, unsigned height)
{
	// Driver sets up swapchain
}

void Renderer::createCommands()
{
	// Driver creates commands in OpenGL, you just set state
}

void Renderer::setupCommands()
{
	// Driver creates commands in OpenGL, you just set state
}

void Renderer::destroyCommands()
{
	// Driver destroys commands
}

void Renderer::createRenderPass()
{
	// Render passes exist at the driver level
}

void Renderer::createSynchronization()
{
	// Driver handles sync
}
