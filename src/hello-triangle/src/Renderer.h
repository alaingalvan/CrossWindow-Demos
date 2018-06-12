#pragma once

#include "CrossWindow/CrossWindow.h"
#include "CrossWindow/Graphics/Graphics.h"

#if defined(XGFX_VULKAN)
#include <vulkan/vulkan.hpp>
#elif defined(XGFX_DIRECTX12)

#elif defined(XGFX_OPENGL)

#elif defined(XGFX_METAL)

#endif

class Renderer
{
public:
  Renderer(xwin::Window& window);

  void render();

  void resize(unsigned width, unsigned height);

protected:
#if defined(XGFX_VULKAN)
  // Initialization
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
  
  // Resources
  vk::Buffer mVertexBuffer;

  // Sync
  vk::Semaphore mPresentCompleteSemaphore;
  vk::Semaphore mRenderCompleteSemaphore;
  vk::Fence mWaitFence;


#elif defined(XGFX_DIRECTX12)
  using Microsoft::WRL::ComPtr;

  // Initialization
	ComPtr<IDXGISwapChain3> mSwapchain;
	ComPtr<ID3D12Device> mDevice;

	// Resources
	ComPtr<ID3D12Resource> mVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

	// Sync

#elif defined(XGFX_OPENGL)
  //Initialization

  // Resources
  GLuint mVertexArray;
  GLuint mVertexBuffer;
  GLuint mIndexBuffer;

#elif defined(XGFX_METAL)
  //Initialization

  //Resources

  //Sync

#endif



}