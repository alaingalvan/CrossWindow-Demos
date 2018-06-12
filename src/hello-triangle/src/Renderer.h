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
  // Initialization
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> mSwapchain;
	ComPtr<ID3D12Device> mDevice;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	UINT m_rtvDescriptorSize;

	// Resources
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	// Sync
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;
#elif defined(XGFX_OPENGL)
  //Initialization

  // Resources

#elif defined(XGFX_METAL)

#endif



}