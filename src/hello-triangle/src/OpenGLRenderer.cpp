#include <glad/glad.h>
#include "Renderer.h"

std::vector<char> readFile(const std::string& filename) {
	std::string path = filename;
	char pBuf[1024];
#ifdef XWIN_WIN32

	_getcwd(pBuf, 1024);
	path = pBuf;
	path += "\\";
#else
	getcwd(pBuf, 1024);
	path = pBuf;
	path += "/";
#endif
	path += filename;
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	bool exists = (bool)file;

	if (!exists || !file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
};

Renderer::Renderer(xwin::Window& window)
{
	initializeAPI(window);
	initializeResources();
	setupCommands();
	tStart = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer()
{
	xgfx::unsetContext(mOGLState);
	xgfx::destroyContext(mOGLState);
}

void Renderer::initializeAPI(xwin::Window& window)
{
	xgfx::OpenGLDesc ogldesc;
	mOGLState = xgfx::createContext(&window, ogldesc);
	xgfx::setContext(mOGLState);
	if (!gladLoadGL())
	{
		// Failed
		return;
	}

	auto err = glGetError();

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

	glGenVertexArrays(1, &mVertexArray);
	glBindVertexArray(mVertexArray);
	glEnableVertexAttribArray(0);

	auto checkShaderCompilation = [&](GLuint shader)
	{
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
	if (result != GL_TRUE) {
        std::cout << "Program failed to link.";
		return;
	}

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
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
}

void Renderer::resize(unsigned width, unsigned height)
{
	// Update Unforms
	uboVS.projectionMatrix = Matrix4::perspective(45.0f, (float)width / (float)height, 0.01f, 1024.0f);
	GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(p, &uboVS, sizeof(uboVS));
	glUnmapBuffer(GL_UNIFORM_BUFFER);
}

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

void Renderer::initFrameBuffer()
{
	// Driver creates initial framebuffer
}

void Renderer::destroyFrameBuffer()
{
	// Driver creates color + depth stencil frame buffer by default
}

void Renderer::createRenderPass()
{
	// Render passes exist at the driver level
}

void Renderer::createSynchronization()
{
	// Driver handles sync
}
