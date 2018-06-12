#include "Renderer.h"

Renderer::Renderer(xwin::Window& window)
{
  auto layer = xwin::createMetalLayer(&window);
}