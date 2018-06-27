#include "Renderer.h"

// Helper functions

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

// Renderer

Renderer::Renderer(xwin::Window& window)
{
	initializeAPI(window);
	initializeResources();
	setupCommands();
	tStart = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer()
{
}


void Renderer::initializeAPI(xwin::Window& window)
{
	// The renderer needs the window when resizing the swapchain
	mWindow = &window;
}