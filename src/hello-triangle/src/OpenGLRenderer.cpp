#include "Renderer.h"
#include <glad/glad.h>

Renderer::Renderer(xwin::Window& window)
{
  initializeAPI(window);
  initializeResources();
  setupCommands();
  tStart = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer()
{
  xgfx::unsetContext(mState);
  xgfx::destroyContext(mState);
}

void Renderer::initializeAPI(xwin::Window& window)
{
  xgfx::OpenGLDesc ogldesc;
  mOGLstate = xwin::createContext(&window, ogldesc);
  xwin::setContext(oglstate);
  if (!gladLoadGL())
  {
    // Failed
  }
}

void Renderer::initializeResources()
{
	// OpenGL global setup
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
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

	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vsShader, nullptr);
	glCompileShader(vs);
	if (!checkShaderCompilation(vs)) close();

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fsShader, nullptr);
	glCompileShader(fs);
	if (!checkShaderCompilation(fs)) close();

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	GLint result = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (result != GL_TRUE) {
		//qDebug() << "Program not linked.";
	}

	glUseProgram(program);

	vboData = {
		//position (x, y)
		-0.5f, -0.5f,
		-0.5f, 0.5f,
		0.5f, -0.5f,
		0.5f, 0.5f
	};

	iboData = {
		0, 1, 2,
		2, 3, 1
	};

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ibo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vboData.size(), vboData.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * iboData.size(), iboData.data(), GL_STATIC_DRAW);

	positionAttrib = glGetAttribLocation(program, "aPosition");
	glEnableVertexAttribArray(positionAttrib);
	glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

	// Update Uniforms
	GLint uniformLocation = glGetUniformLocation(program, "myUniform");
	glUniform4fv(uniformLocation, 1, color);
}

Renderer::render()
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
		glUniform4fv(uniformLocation, 1, color);

		// Draw
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, iboData.size(), GL_UNSIGNED_SHORT, 0);
}