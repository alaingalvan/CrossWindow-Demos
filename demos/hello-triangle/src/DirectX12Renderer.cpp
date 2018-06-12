#include "Renderer.h"

Renderer::Renderer(xwin::Window& window)
{
  auto swapchain = xwin::createSwapchain(&window);
}