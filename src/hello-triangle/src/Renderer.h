#pragma once

#include "CrossWindow/CrossWindow.h"
#include "CrossWindow/Graphics.h"
#include "vectormath.hpp"

#include <vector>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <iostream>

#if defined(XWIN_WIN32)
#include <direct.h>
#else
#include <unistd.h>
#endif

class Renderer
{
public:
    Renderer(xwin::Window& window);

    ~Renderer();

    // Render onto the render target
    void render();

    // Resize the window and internal data structures
    void resize(unsigned width, unsigned height);

protected:

    // Initialize your Graphics API
    void initializeAPI(xwin::Window& window);

    // Destroy any Graphics API data structures used in this example
    void destroyAPI();

    // Initialize any resources such as VBOs, IBOs, used in this example
    void initializeResources();

    // Destroy any resources used in this example
    void destroyResources();

    // Create graphics API specific data structures to send commands to the GPU
    void createCommands();

    // Set up commands used when rendering frame by this app
    void setupCommands();

    // Destroy all commands
    void destroyCommands();

    // Set up the FrameBuffer
    void initFrameBuffer();

    void destroyFrameBuffer();

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

    std::chrono::time_point<std::chrono::steady_clock> tStart, tEnd;
    float mElapsedTime = 0.0f;

    // Uniform data
    struct {
        Matrix4 projectionMatrix;
        Matrix4 modelMatrix;
        Matrix4 viewMatrix;
    } uboVS;

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

    // Swpachain
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

#elif defined(XGFX_DIRECTX12)

	static const UINT backbufferCount = 2;

    // Initialization
    IDXGIFactory4* mFactory;
    IDXGIAdapter1* mAdapter;
#if defined(_DEBUG)
    ID3D12Debug* mDebugController;
#endif
    xwin::Window* mWindow;
    IDXGISwapChain3* mSwapchain;
    ID3D12Device* mDevice;
    ID3D12CommandAllocator* mCommandAllocator;
    ID3D12CommandQueue* mCommandQueue;
    ID3D12GraphicsCommandList* mCommandList;
    
	// Resources
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mSurfaceSize;
	ID3D12Resource* mVertexBuffer;
	ID3D12Resource* mIndexBuffer;
	ID3D12Resource* mWVPConstantBuffer;
	UINT8* mMappedWVPBuffer;

    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
	ID3D12DescriptorHeap* mRtvHeap;
	ID3D12DescriptorHeap* mCbvHeap;
	UINT mRtvDescriptorSize;
	ID3D12RootSignature* mRootSignature;
	ID3D12PipelineState* mPipelineState;

    // Current Frame
    UINT mCurrentBuffer;
	ID3D12Resource* mRenderTargets[backbufferCount];

    // Sync
	UINT mFrameIndex;
	HANDLE mFenceEvent;
	ID3D12Fence* mFence;
	UINT64 mFenceValue;

#elif defined(XGFX_OPENGL)
    //Initialization
    xgfx::OpenGLState mOGLState;

    // Resources
    GLuint mVertexShader;
    GLuint mFragmentShader;
    GLuint mProgram;
    GLuint mVertexArray;
    GLuint mVertexBuffer;
    GLuint mIndexBuffer;

    GLuint mUniformUBO;

    GLint mPositionAttrib;
    GLint mColorAttrib;
#elif defined(XGFX_METAL)
    // The device (aka GPU) we're using to render
    void* mDevice;
	
	void* mLayer;
    // The command Queue from which we'll obtain command buffers
    void* mCommandQueue;

    // The current size of our view so we can use this in our render pipeline
    unsigned mViewportSize[2];

    //Resources
    void* vertLibrary;
	void* fragLibrary;
    void* vertexFunction;
    void* fragmentFunction;
	void* mVertexBuffer;
	void* mIndexBuffer;
	void* mUniformBuffer;
    void* mPipelineState;
    void* mCommandBuffer;
    //Sync

#endif
};
