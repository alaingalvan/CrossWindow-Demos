#include <glad/glad.h>
#include "Renderer.h"

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
  }
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

			//qDebug() << errorLog.data();

			return false;
		}
		return true;
	};

	mVertexShader = glCreateShader(GL_VERTEX_SHADER);
	//glShaderSource(mVertexShader, 1, &mVertexShader, nullptr);
	glCompileShader(mVertexShader);
	if (!checkShaderCompilation(mVertexShader)) return;

	mFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//glShaderSource(mFragmentShader, 1, &mFragmentShader, nullptr);
	glCompileShader(mFragmentShader);
	if (!checkShaderCompilation(mFragmentShader)) return;

	mProgram = glCreateProgram();
	glAttachShader(mProgram, mVertexShader);
	glAttachShader(mProgram, mFragmentShader);
	glLinkProgram(mProgram);

	GLint result = 0;
	glGetProgramiv(mProgram, GL_LINK_STATUS, &result);
	if (result != GL_TRUE) {
		//Program failed to link
		return;
	}

	glUseProgram(mProgram);

	glGenBuffers(1, &mVertexBuffer);
	glGenBuffers(1, &mIndexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 3, mVertexBufferData, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3, mIndexBufferData, GL_STATIC_DRAW);

	mPositionAttrib = glGetAttribLocation(mProgram, "aPosition");
	glEnableVertexAttribArray(mPositionAttrib);
	glVertexAttribPointer(mPositionAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

	// Update Uniforms
	mUniformTime = glGetUniformLocation(mProgram, "myUniform");
	//glUniform4fv(uniformLocation, 1, color);
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
    { return; }
    tStart = std::chrono::high_resolution_clock::now();

    mElapsedTime += 0.001f * time;
    mElapsedTime = fmodf(mElapsedTime, 6.283185307179586f);

	// Update Uniforms
	//glUniform4fv(uniformLocation, 1, color);

	// Draw
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
}

void Renderer::resize(unsigned width, unsigned height)
{
}
