#pragma once

#include "CrossWindow/CrossWindow.h"
#include "CrossWindow/Graphics/Graphics.h"

#if defined(XGFX_VULKAN)
#include <vulkan/vulkan.hpp>
#endif

class Renderer
{
public:
  Renderer(xwin::Window& window);

  void render();

  void resize(unsigned width, unsigned height);

protected:
#if defined(XGFX_VULKAN)
  vk::Instance mInstance;
  vk::PhysicalDevice mPhysicalDevice;
  vk::Device mDevice;
  vk::SwapchainKHR mSwapchain;
  vk::SurfaceKHR mSurface;
  float mQueuePriority;
  vk::Queue mQueue;
  uint32_t mQueueFamilyIndex;
  vk::CommandPool mCommandPool;
  std::vector<vk::CommandBuffer> mCommandBuffers;
  vk::Semaphore mPresentCompleteSemaphore;
  vk::Semaphore mRenderCompleteSemaphore;
  vk::Fence mWaitFence;

#endif

}