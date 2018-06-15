#pragma once

#include "CrossWindow/CrossWindow.h"
#include "CrossWindow/Graphics.h"
#include <vector>
#include "vectormath.hpp"

#if defined(XGFX_VULKAN)
#include <vulkan/vulkan.hpp>
#elif defined(XGFX_DIRECTX12)

#elif defined(XGFX_OPENGL)

#elif defined(XGFX_METAL)

#endif

#if defined(XWIN_WIN32)
#include <direct.h>
#endif

class Renderer
{
public:
  Renderer(xwin::Window& window);

  ~Renderer();

  // Initialize your Graphics API
  void initializeAPI(xwin::Window& window);

  // Destroy any Graphics API data structures used in this example
  void destroyAPI();

  // Initialize any resources used in this example
  void initializeResources();

  // Destroy any resources used in this example
  void destroyResources();

  // Create graphics API specific data structures to send commands to the GPU
  void createCommands();

  // Set up commands used when rendering frame by this app
  void setupCommands();

  // Destroy all commands
  void destroyCommands();

  // Render onto the render target
  void render();

  // Resize the window and internal data structures
  void resize(unsigned width, unsigned height);

protected:

    // Set up the FrameBuffer
    void setupFrameBuffer();

    // Set up the RenderPass
    void createRenderPass();

    void createSynchronization();

    // Set up the swapchain
    void setupSwapchain(unsigned width, unsigned height);

  struct Vertex
    {
      float position[3];
      float color[3];
    };
    
  Vertex mVertexBufferData[3] =
  {
    { { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
    { { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
    { { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
  };

  uint32_t mIndexBufferData[3] = { 0, 1, 2 };

  float mElapsedTime = 0.0f;

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
  uint32_t mCurrentBuffer;

  vk::Extent2D mSurfaceSize;
  vk::Rect2D mRenderArea;
  vk::Viewport mViewport;
  
  // Resources
  vk::Format mSurfaceColorFormat;
  vk::ColorSpaceKHR mSurfaceColorSpace;
  vk::Format mSurfaceDepthFormat;
  vk::Image mDepthImage;
  vk::DeviceMemory mDepthImageMemory;

  vk::DescriptorPool mDescriptorPool;
  std::vector<vk::DescriptorSetLayout> mDescriptorSetLayouts;
  std::vector<vk::DescriptorSet> mDescriptorSets;

  vk::ShaderModule mVertModule;
  vk::ShaderModule mFragModule;

  vk::RenderPass mRenderPass;

  vk::Buffer mVertexBuffer;
  vk::Buffer mIndexBuffer;

  vk::PipelineCache mPipelineCache;
  vk::Pipeline mPipeline;
  vk::PipelineLayout mPipelineLayout;

  // Sync
  vk::Semaphore mPresentCompleteSemaphore;
  vk::Semaphore mRenderCompleteSemaphore;
  std::vector<vk::Fence> mWaitFences;

  struct SwapChainBuffer {
      vk::Image image;
      std::array<vk::ImageView, 2> views;
      vk::Framebuffer frameBuffer;
  };

  std::vector<SwapChainBuffer> mSwapchainBuffers;

  // Vertex buffer and attributes
  struct {
      vk::DeviceMemory memory;															// Handle to the device memory for this buffer
      vk::Buffer buffer;																// Handle to the Vulkan buffer object that the memory is bound to
      vk::PipelineVertexInputStateCreateInfo inputState;
      vk::VertexInputBindingDescription inputBinding;
      std::vector<vk::VertexInputAttributeDescription> inputAttributes;
  } mVertices;

  // Index buffer
  struct
  {
      vk::DeviceMemory memory;
      vk::Buffer buffer;
      uint32_t count;
} mIndices;

  // Uniform block object
  struct {
      vk::DeviceMemory memory;
      vk::Buffer buffer;
      vk::DescriptorBufferInfo descriptor;
  }  mUniformDataVS;

  // For simplicity we use the same uniform block layout as in the shader:
  //
  //	layout(set = 0, binding = 0) uniform UBO
  //	{
  //		mat4 projectionMatrix;
  //		mat4 modelMatrix;
  //		mat4 viewMatrix;
  //	} ubo;
  //
  // This way we can just memcopy the ubo data to the ubo
  // Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
  struct {
      Matrix4 projectionMatrix;
      Matrix4 modelMatrix;
      Matrix4 viewMatrix;
} uboVS;


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



};