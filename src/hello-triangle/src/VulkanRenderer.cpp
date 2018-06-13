#define XGFX_IMPL
#include "Renderer.h"
#include <fstream>

namespace
{
    uint32_t getQueueIndex(vk::PhysicalDevice& physicalDevice, vk::QueueFlagBits flags)
    {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();

        for (size_t i = 0; i < queueProps.size(); ++i)
        {
            if (queueProps[i].queueFlags & flags) {
                return static_cast<uint32_t>(i);
            }
        }

        // Default queue index
        return 0;
    }

    uint32_t getMemoryTypeIndex(vk::PhysicalDevice& physicalDevice, uint32_t typeBits, vk::MemoryPropertyFlags properties)
    {
        auto gpuMemoryProps = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < gpuMemoryProps.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((gpuMemoryProps.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }
            typeBits >>= 1;
        }
        return 0;
    };

    std::vector<char> readFile(const std::string& filename) {
        std::string path = filename;
#ifdef XWIN_WIN32
        char pBuf[256];
        _getcwd(pBuf, 256);
        path = pBuf;
        path += "\\";
#endif
        path += filename;
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        bool exists = (bool)file;

        if (!exists || !file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    };
}

Renderer::Renderer(xwin::Window& window)
{
    initializeAPI(window);
    initializeResources();
}

Renderer::~Renderer()
{
    mDevice.freeCommandBuffers(mCommandPool, mCommandBuffers.size(), mCommandBuffers.data());
    mDevice.waitIdle();
}

void Renderer::initializeAPI(xwin::Window& window)
{
    vk::ApplicationInfo appInfo(
        "Hello Triangle",
        0,
        "HelloTriangleEngine",
        0,
        VK_API_VERSION_1_0
    );

    std::vector<const char*> layers = {};

    std::vector<const char*> extensions =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif VK_USE_PLATFORM_MACOS_MVK
        VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
#elif VK_USE_PLATFORM_XCB_KHR
        VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif VK_USE_PLATFORM_ANDROID_KHR
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
#elif VK_USE_PLATFORM_XLIB_KHR
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#elif VK_USE_PLATFORM_XCB_KHR
        VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif VK_USE_PLATFORM_WAYLAND_KHR
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
#elif VK_USE_PLATFORM_MIR_KHR || VK_USE_PLATFORM_DISPLAY_KHR
        VK_KHR_DISPLAY_EXTENSION_NAME;
#elif VK_USE_PLATFORM_ANDROID_KHR
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
#elif VK_USE_PLATFORM_IOS_MVK
        VK_MVK_IOS_SURFACE_EXTENSION_NAME;
#endif
    };

    vk::InstanceCreateInfo info(
        vk::InstanceCreateFlags(),
        &appInfo,
        static_cast<uint32_t>(layers.size()),
        layers.data(),
        static_cast<uint32_t>(extensions.size()),
        extensions.data()
    );

    mInstance = vk::createInstance(info);
    mSurface = xgfx::getSurface(&window, mInstance);

    std::vector<vk::PhysicalDevice> physicalDevices = mInstance.enumeratePhysicalDevices();
    mPhysicalDevice = physicalDevices[0];

    mQueueFamilyIndex = getQueueIndex(mPhysicalDevice, vk::QueueFlagBits::eGraphics);

    // Queue Creation
    vk::DeviceQueueCreateInfo qcinfo;
    qcinfo.setQueueFamilyIndex(mQueueFamilyIndex);
    qcinfo.setQueueCount(1);
    mQueuePriority = 0.5f;
    qcinfo.setPQueuePriorities(&mQueuePriority);

    // Logical Device
    vk::DeviceCreateInfo dinfo;
    dinfo.setPQueueCreateInfos(&qcinfo);
    dinfo.setQueueCreateInfoCount(1);
    const char *extensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    dinfo.setPpEnabledExtensionNames(&extensionName);
    dinfo.setEnabledExtensionCount(1);
    mDevice = mPhysicalDevice.createDevice(dinfo);

    // Queue
    mQueue = mDevice.getQueue(mQueueFamilyIndex, 0);

    // Command Pool
    mCommandPool = mDevice.createCommandPool(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer),
            mQueueFamilyIndex)
    );

    //Swapchain
    const xwin::WindowDesc wdesc = window.getDesc();
    resize(wdesc.width, wdesc.height);
    std::vector<vk::Image> swapchainImages = mDevice.getSwapchainImagesKHR(mSwapchain);

    // Command Buffers
    mCommandBuffers = mDevice.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(
            mCommandPool,
            vk::CommandBufferLevel::ePrimary,
            swapchainImages.size()));

    //Descriptor Pool
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes =
    {
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformBuffer,
            1
        )
    };

    mDescriptorPool = mDevice.createDescriptorPool(
        vk::DescriptorPoolCreateInfo(
            vk::DescriptorPoolCreateFlags(),
            1,
            descriptorPoolSizes.size(),
            descriptorPoolSizes.data()
        )
    );

    //Descriptor Set Layout
    // Binding 0: Uniform buffer (Vertex shader)
    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings =
    {
        vk::DescriptorSetLayoutBinding(
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        )
    };

    mDescriptorSetLayouts = {
        mDevice.createDescriptorSetLayout(
            vk::DescriptorSetLayoutCreateInfo(
                vk::DescriptorSetLayoutCreateFlags(),
                descriptorSetLayoutBindings.size(),
                descriptorSetLayoutBindings.data()
            )
        )
    };

    mDescriptorSets = mDevice.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo(
            mDescriptorPool,
            mDescriptorSetLayouts.size(),
            mDescriptorSetLayouts.data()
        )
    );

    // ColorFormats
    // Check to see if we can display rgb colors.
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = mPhysicalDevice.getSurfaceFormatsKHR(mSurface);

    vk::Format surfaceColorFormat;
    vk::ColorSpaceKHR surfaceColorSpace;

    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
        surfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
    else
        surfaceColorFormat = surfaceFormats[0].format;

    surfaceColorSpace = surfaceFormats[0].colorSpace;

    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector<vk::Format> depthFormats =
    {
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD32Sfloat,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD16Unorm
    };

    vk::Format surfaceDepthFormat;

    for (vk::Format& format : depthFormats)
    {
        vk::FormatProperties depthFormatProperties = mPhysicalDevice.getFormatProperties(format);
        // Format must support depth stencil attachment for optimal tiling
        if (depthFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        {
            surfaceDepthFormat = format;
            break;
        }
    }

    // Create Depth Image Data
    vk::Image depthImage = mDevice.createImage(
        vk::ImageCreateInfo(
            vk::ImageCreateFlags(),
            vk::ImageType::e2D,
            surfaceDepthFormat,
            vk::Extent3D(mSurfaceSize.width, mSurfaceSize.height, 1),
            1U,
            1U,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive,
            1,
            &mQueueFamilyIndex,
            vk::ImageLayout::eUndefined
        )
    );

    vk::MemoryRequirements depthMemoryReq = mDevice.getImageMemoryRequirements(depthImage);

    // Search through GPU memory properies to see if this can be device local.

    vk::DeviceMemory depthMemory = mDevice.allocateMemory(
        vk::MemoryAllocateInfo(
            depthMemoryReq.size,
            getMemoryTypeIndex(mPhysicalDevice, depthMemoryReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        )
    );

    mDevice.bindImageMemory(
        depthImage,
        depthMemory,
        0
    );

    vk::ImageView depthImageView = mDevice.createImageView(
        vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags(),
            depthImage,
            vk::ImageViewType::e2D,
            surfaceDepthFormat,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                0,
                1,
                0,
                1
            )
        )
    );

    mSwapchainBuffers.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        mSwapchainBuffers[i].image = swapchainImages[i];

        // Color
        mSwapchainBuffers[i].views[0] =
            mDevice.createImageView(
                vk::ImageViewCreateInfo(
                    vk::ImageViewCreateFlags(),
                    swapchainImages[i],
                    vk::ImageViewType::e2D,
                    surfaceColorFormat,
                    vk::ComponentMapping(),
                    vk::ImageSubresourceRange(
                        vk::ImageAspectFlagBits::eColor,
                        0,
                        1,
                        0,
                        1
                    )
                )
            );

        // Depth
        mSwapchainBuffers[i].views[1] = depthImageView;

        mSwapchainBuffers[i].frameBuffer = mDevice.createFramebuffer(
            vk::FramebufferCreateInfo(
                vk::FramebufferCreateFlags(),
                mRenderPass,
                mSwapchainBuffers[i].views.size(),
                mSwapchainBuffers[i].views.data(),
                mSurfaceSize.width,
                mSurfaceSize.height,
                1
            )
        );
    }

    std::vector<vk::AttachmentDescription> attachmentDescriptions =
    {
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            surfaceColorFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR
        ),
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            surfaceDepthFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        )
    };

    std::vector<vk::AttachmentReference> colorReferences =
    {
        vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal)
    };

    std::vector<vk::AttachmentReference> depthReferences = {
        vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
    };

    std::vector<vk::SubpassDescription> subpasses =
    {
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            0,
            nullptr,
            colorReferences.size(),
            colorReferences.data(),
            nullptr,
            depthReferences.data(),
            0,
            nullptr
        )
    };

    std::vector<vk::SubpassDependency> dependencies =
    {
        vk::SubpassDependency(
            ~0U,
            0,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eMemoryRead,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::DependencyFlagBits::eByRegion
        ),
        vk::SubpassDependency(
            0,
            ~0U,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::DependencyFlagBits::eByRegion
        )
    };

    mRenderPass = mDevice.createRenderPass(
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags(),
            attachmentDescriptions.size(),
            attachmentDescriptions.data(),
            subpasses.size(),
            subpasses.data(),
            dependencies.size(),
            dependencies.data()
        )
    );

    //Synchronization

    // Semaphore used to ensures that image presentation is complete before starting to submit again
    mPresentCompleteSemaphore = mDevice.createSemaphore(vk::SemaphoreCreateInfo());

    // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
    mRenderCompleteSemaphore = mDevice.createSemaphore(vk::SemaphoreCreateInfo());

    // Fence for command buffer completion
    mWaitFences.resize(mSwapchainBuffers.size());
    for (size_t i = 0; i < mWaitFences.size(); i++)
    {
        mWaitFences[i] = mDevice.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }

}

void Renderer::initializeResources()
{
    std::vector<vk::ClearValue> clearValues =
    {
        vk::ClearColorValue(
            std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f}),
        vk::ClearDepthStencilValue(1.0f, 0)
    };

    vk::PipelineLayout pipelineLayout = mDevice.createPipelineLayout(
        vk::PipelineLayoutCreateInfo(
            vk::PipelineLayoutCreateFlags(),
            mDescriptorSetLayouts.size(),
            mDescriptorSetLayouts.data(),
            0,
            nullptr
        )
    );

    std::vector<char> vertShaderCode = readFile("bin/triangle.vert.spv");
    std::vector<char> fragShaderCode = readFile("bin/triangle.frag.spv");

    mVertModule = mDevice.createShaderModule(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            vertShaderCode.size(),
            (uint32_t*)vertShaderCode.data()
        )
    );

    mFragModule = mDevice.createShaderModule(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            fragShaderCode.size(),
            (uint32_t*)fragShaderCode.data()
        )
    );

    auto pipelineCache = mDevice.createPipelineCache(vk::PipelineCacheCreateInfo());

    std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
        vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eVertex,
            mVertModule,
            "main",
            nullptr
        ),
        vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eFragment,
            mFragModule,
            "main",
            nullptr
        )
    };

    auto pvi = vertices.inputState;

    vk::PipelineInputAssemblyStateCreateInfo pia(
        vk::PipelineInputAssemblyStateCreateFlags(),
        vk::PrimitiveTopology::eTriangleList
    );

    vk::PipelineViewportStateCreateInfo pv(
        vk::PipelineViewportStateCreateFlagBits(),
        0,
        nullptr,
        0,
        nullptr
    );

    vk::PipelineRasterizationStateCreateInfo pr(
        vk::PipelineRasterizationStateCreateFlags(),
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0,
        0,
        0,
        1.0f
    );

    vk::PipelineMultisampleStateCreateInfo pm(
        vk::PipelineMultisampleStateCreateFlags(),
        vk::SampleCountFlagBits::e1
    );

    // Dept and Stencil state for primative compare/test operations

    auto pds = vk::PipelineDepthStencilStateCreateInfo(
        vk::PipelineDepthStencilStateCreateFlags(),
        VK_TRUE,
        VK_TRUE,
        vk::CompareOp::eLessOrEqual,
        VK_FALSE,
        VK_FALSE,
        vk::StencilOpState(),
        vk::StencilOpState(),
        0,
        0
    );

    // Blend State - How two primatives should draw on top of each other.
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments =
    {
        vk::PipelineColorBlendAttachmentState(
            VK_FALSE,
            vk::BlendFactor::eZero,
            vk::BlendFactor::eOne,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eZero,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        )
    };

    vk::PipelineColorBlendStateCreateInfo pbs(
        vk::PipelineColorBlendStateCreateFlags(),
        0,
        vk::LogicOp::eClear,
        colorBlendAttachments.size(),
        colorBlendAttachments.data()
    );

    std::vector<vk::DynamicState> dynamicStates =
    {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo pdy(
        vk::PipelineDynamicStateCreateFlags(),
        dynamicStates.size(),
        dynamicStates.data()
    );

    mPipeline = mDevice.createGraphicsPipeline(
        pipelineCache,
        vk::GraphicsPipelineCreateInfo(
            vk::PipelineCreateFlags(vk::PipelineCreateFlagBits::eDerivative),
            pipelineShaderStages.size(),
            pipelineShaderStages.data(),
            &pvi,
            &pia,
            nullptr,
            &pv,
            &pr,
            &pm,
            &pds,
            &pbs,
            &pdy,
            pipelineLayout,
            mRenderPass,
            0
        )
    );

    struct Vertex
    {
        float position[3];
        float color[3];
    };

    // Vertex buffer and attributes
    struct {
        vk::DeviceMemory memory;															// Handle to the device memory for this buffer
        vk::Buffer buffer;																// Handle to the Vulkan buffer object that the memory is bound to
        vk::PipelineVertexInputStateCreateInfo inputState;
        vk::VertexInputBindingDescription inputBinding;
        std::vector<vk::VertexInputAttributeDescription> inputAttributes;
    } vertices;

    // Index buffer
    struct
    {
        vk::DeviceMemory memory;
        vk::Buffer buffer;
        uint32_t count;
    } indices;

    // Uniform block object
    struct {
        vk::DeviceMemory memory;
        vk::Buffer buffer;
        vk::DescriptorBufferInfo descriptor;
    }  uniformDataVS;

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

    // Setup vertices data
    std::vector<Vertex> vertexBuffer =
    {
    { { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
    { { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
    { { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
    };

    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

    // Setup indices data
    std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
    indices.count = static_cast<uint32_t>(indexBuffer.size());
    uint32_t indexBufferSize = indices.count * sizeof(uint32_t);

    void *data;
    // Static data like vertex and index buffer should be stored on the device memory 
    // for optimal (and fastest) access by the GPU
    //
    // To achieve this we use so-called "staging buffers" :
    // - Create a buffer that's visible to the host (and can be mapped)
    // - Copy the data to this buffer
    // - Create another buffer that's local on the device (VRAM) with the same size
    // - Copy the data from the host to the device using a command buffer
    // - Delete the host visible (staging) buffer
    // - Use the device local buffers for rendering

    struct StagingBuffer {
        vk::DeviceMemory memory;
        vk::Buffer buffer;
    };

    struct {
        StagingBuffer vertices;
        StagingBuffer indices;
    } stagingBuffers;

    // Vertex buffer
    stagingBuffers.vertices.buffer = mDevice.createBuffer(
        vk::BufferCreateInfo(
            vk::BufferCreateFlags(),
            vertexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive,
            1,
            &mQueueFamilyIndex
        )
    );

    auto memReqs = mDevice.getBufferMemoryRequirements(stagingBuffers.vertices.buffer);

    // Request a host visible memory type that can be used to copy our data do
    // Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
    stagingBuffers.vertices.memory = mDevice.allocateMemory(
        vk::MemoryAllocateInfo(
            memReqs.size,
            getMemoryTypeIndex(mPhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
        )
    );

    // Map and copy
    data = mDevice.mapMemory(stagingBuffers.vertices.memory, 0, memReqs.size, vk::MemoryMapFlags());
    memcpy(data, vertexBuffer.data(), vertexBufferSize);
    mDevice.unmapMemory(stagingBuffers.vertices.memory);
    mDevice.bindBufferMemory(stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

    // Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
    vertices.buffer = mDevice.createBuffer(
        vk::BufferCreateInfo(
            vk::BufferCreateFlags(),
            vertexBufferSize,
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::SharingMode::eExclusive,
            1,
            &mQueueFamilyIndex
        )
    );

    memReqs = mDevice.getBufferMemoryRequirements(vertices.buffer);

    vertices.memory = mDevice.allocateMemory(
        vk::MemoryAllocateInfo(
            memReqs.size,
            getMemoryTypeIndex(mPhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        )
    );

    mDevice.bindBufferMemory(vertices.buffer, vertices.memory, 0);

    // Index buffer
    // Copy index data to a buffer visible to the host (staging buffer)
    stagingBuffers.indices.buffer = mDevice.createBuffer(
        vk::BufferCreateInfo(
            vk::BufferCreateFlags(),
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive,
            1,
            &mQueueFamilyIndex
        )
    );
    memReqs = mDevice.getBufferMemoryRequirements(stagingBuffers.indices.buffer);
    stagingBuffers.indices.memory = mDevice.allocateMemory(
        vk::MemoryAllocateInfo(
            memReqs.size,
            getMemoryTypeIndex(mPhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
        )
    );

    data = mDevice.mapMemory(stagingBuffers.indices.memory, 0, indexBufferSize, vk::MemoryMapFlags());
    memcpy(data, indexBuffer.data(), indexBufferSize);
    mDevice.unmapMemory(stagingBuffers.indices.memory);
    mDevice.bindBufferMemory(stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

    // Create destination buffer with device only visibility
    indices.buffer = mDevice.createBuffer(
        vk::BufferCreateInfo(
            vk::BufferCreateFlags(),
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::SharingMode::eExclusive,
            0,
            nullptr
        )
    );

    memReqs = mDevice.getBufferMemoryRequirements(indices.buffer);
    indices.memory = mDevice.allocateMemory(
        vk::MemoryAllocateInfo(
            memReqs.size,
            getMemoryTypeIndex(mPhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal
            )
        )
    );

    mDevice.bindBufferMemory(indices.buffer, indices.memory, 0);

    auto getCommandBuffer = [&](bool begin)
    {
        vk::CommandBuffer cmdBuffer = mDevice.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(
                mCommandPool,
                vk::CommandBufferLevel::ePrimary,
                1)
        )[0];

        // If requested, also start the new command buffer
        if (begin)
        {
            cmdBuffer.begin(
                vk::CommandBufferBeginInfo()
            );
        }

        return cmdBuffer;
    };

    // Buffer copies have to be submitted to a queue, so we need a command buffer for them
    // Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
    vk::CommandBuffer copyCmd = getCommandBuffer(true);

    // Put buffer region copies into command buffer
    std::vector<vk::BufferCopy> copyRegions =
    {
        vk::BufferCopy(0, 0, vertexBufferSize)
    };

    // Vertex buffer
    copyCmd.copyBuffer(stagingBuffers.vertices.buffer, vertices.buffer, copyRegions);

    // Index buffer
    copyRegions =
    {
        vk::BufferCopy(0, 0,  indexBufferSize)
    };

    copyCmd.copyBuffer(stagingBuffers.indices.buffer, indices.buffer, copyRegions);

    // Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
    auto flushCommandBuffer = [&](vk::CommandBuffer commandBuffer)
    {
        commandBuffer.end();

        std::vector<vk::SubmitInfo> submitInfos = {
            vk::SubmitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr)
        };

        // Create fence to ensure that the command buffer has finished executing
        vk::Fence fence = mDevice.createFence(vk::FenceCreateInfo());

        // Submit to the queue
        mQueue.submit(submitInfos, fence);
        // Wait for the fence to signal that command buffer has finished executing
        mDevice.waitForFences(1, &fence, VK_TRUE, UINT_MAX);
        mDevice.destroyFence(fence);
        mDevice.freeCommandBuffers(mCommandPool, 1, &commandBuffer);
    };

    flushCommandBuffer(copyCmd);

    // Destroy staging buffers
    // Note: Staging buffer must not be deleted before the copies have been submitted and executed
    mDevice.destroyBuffer(stagingBuffers.vertices.buffer);
    mDevice.freeMemory(stagingBuffers.vertices.memory);
    mDevice.destroyBuffer(stagingBuffers.indices.buffer);
    mDevice.freeMemory(stagingBuffers.indices.memory);


    // Vertex input binding
    vertices.inputBinding.binding = 0;
    vertices.inputBinding.stride = sizeof(Vertex);
    vertices.inputBinding.inputRate = vk::VertexInputRate::eVertex;

    // Inpute attribute binding describe shader attribute locations and memory layouts
    // These match the following shader layout (see triangle.vert):
    //	layout (location = 0) in vec3 inPos;
    //	layout (location = 1) in vec3 inColor;
    vertices.inputAttributes.resize(2);
    // Attribute location 0: Position
    vertices.inputAttributes[0].binding = 0;
    vertices.inputAttributes[0].location = 0;
    vertices.inputAttributes[0].format = vk::Format::eR32G32B32Sfloat;
    vertices.inputAttributes[0].offset = offsetof(Vertex, position);
    // Attribute location 1: Color
    vertices.inputAttributes[1].binding = 0;
    vertices.inputAttributes[1].location = 1;
    vertices.inputAttributes[1].format = vk::Format::eR32G32B32Sfloat;
    vertices.inputAttributes[1].offset = offsetof(Vertex, color);

    // Assign to the vertex input state used for pipeline creation
    vertices.inputState.flags = vk::PipelineVertexInputStateCreateFlags();
    vertices.inputState.vertexBindingDescriptionCount = 1;
    vertices.inputState.pVertexBindingDescriptions = &vertices.inputBinding;
    vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.inputAttributes.size());
    vertices.inputState.pVertexAttributeDescriptions = vertices.inputAttributes.data();

#pragma region UniformBuffers
    // Prepare and initialize a uniform buffer block containing shader uniforms
    // Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks

    // Vertex shader uniform buffer block
    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = 0;
    allocInfo.memoryTypeIndex = 0;

    // Create a new buffer
    uniformDataVS.buffer = mDevice.createBuffer(
        vk::BufferCreateInfo(
            vk::BufferCreateFlags(),
            sizeof(uboVS),
            vk::BufferUsageFlagBits::eUniformBuffer
        )
    );
    // Get memory requirements including size, alignment and memory type 
    memReqs = mDevice.getBufferMemoryRequirements(uniformDataVS.buffer);
    allocInfo.allocationSize = memReqs.size;
    // Get the memory type index that supports host visibile memory access
    // Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
    // We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
    // Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
    allocInfo.memoryTypeIndex = getMemoryTypeIndex(mPhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    // Allocate memory for the uniform buffer
    uniformDataVS.memory = mDevice.allocateMemory(allocInfo);
    // Bind memory to buffer
    mDevice.bindBufferMemory(uniformDataVS.buffer, uniformDataVS.memory, 0);

    // Store information in the uniform's descriptor that is used by the descriptor set
    uniformDataVS.descriptor.buffer = uniformDataVS.buffer;
    uniformDataVS.descriptor.offset = 0;
    uniformDataVS.descriptor.range = sizeof(uboVS);
#pragma endregion

#pragma region UpdateUniforms

    float zoom = -2.5f;
    auto rotation = Vector3();

    // Update matrices
    uboVS.projectionMatrix = Matrix4::perspective(60.0f, (float)mViewport.width / (float)mViewport.height, 0.1f, 256.0f);

    uboVS.viewMatrix = Matrix4::translation(Vector3(0.0f, 0.0f, zoom));

    uboVS.modelMatrix = Matrix4();
    uboVS.modelMatrix = uboVS.modelMatrix.rotation(rotation.getX(), Vector3(1.0f, 0.0f, 0.0f));
    uboVS.modelMatrix = uboVS.modelMatrix.rotation(rotation.getY(), Vector3(0.0f, 1.0f, 0.0f));
    uboVS.modelMatrix = uboVS.modelMatrix.rotation(rotation.getZ(), Vector3(0.0f, 0.0f, 1.0f));

    // Map uniform buffer and update it
    void *pData;
    pData = mDevice.mapMemory(uniformDataVS.memory, 0, sizeof(uboVS));
    memcpy(pData, &uboVS, sizeof(uboVS));
    mDevice.unmapMemory(uniformDataVS.memory);

#pragma endregion

#pragma region DescriptorSetUpdate
    std::vector<vk::WriteDescriptorSet> descriptorWrites =
    {
        vk::WriteDescriptorSet(
            mDescriptorSets[0],
            0,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &uniformDataVS.descriptor,
            nullptr
        )
    };

    mDevice.updateDescriptorSets(descriptorWrites, nullptr);
#pragma endregion

    for (size_t i = 0; i < mCommandBuffers.size(); ++i)
    {
        vk::CommandBuffer& cmd = mCommandBuffers[i];
        cmd.begin(vk::CommandBufferBeginInfo());
        cmd.beginRenderPass(
            vk::RenderPassBeginInfo(
                mRenderPass,
                mSwapchainBuffers[i].frameBuffer,
                mRenderArea,
                clearValues.size(),
                clearValues.data()),
            vk::SubpassContents::eInline);

        cmd.setViewport(0, nullptr);

        cmd.setScissor(0, nullptr);

        // Bind Descriptor Sets, these are attribute/uniform "descriptions"
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);

        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            pipelineLayout,
            0,
            mDescriptorSets,
            nullptr);
        vk::DeviceSize offsets = { 0 };
        cmd.bindVertexBuffers(0, 1, &vertices.buffer, &offsets);
        cmd.bindIndexBuffer(indices.buffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(3, 1, 0, 0, 1);
        cmd.endRenderPass();
        cmd.end();
    }

}

void Renderer::render()
{
    mDevice.acquireNextImageKHR(mSwapchain, ULLONG_MAX, mPresentCompleteSemaphore, nullptr, &mCurrentBuffer);
    mDevice.waitForFences(1, &mWaitFences[mCurrentBuffer], VK_TRUE, UINT64_MAX);
    mDevice.resetFences(1, &mWaitFences[mCurrentBuffer]);


    vk::SubmitInfo submitInfo;

    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    submitInfo
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&mPresentCompleteSemaphore)
        .setPWaitDstStageMask(&waitDstStageMask)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&mCommandBuffers[mCurrentBuffer])
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&mRenderCompleteSemaphore);

    mQueue.submit(1, &submitInfo, mWaitFences[mCurrentBuffer]);

    mQueue.presentKHR(
        vk::PresentInfoKHR(
            1,
            &mRenderCompleteSemaphore,
            1,
            &mSwapchain,
            &mCurrentBuffer,
            nullptr
        )
    );
}

void Renderer::resize(unsigned width, unsigned height)
{
    mSurfaceSize = vk::Extent2D(width, height);
    mRenderArea = vk::Rect2D(vk::Offset2D(), mSurfaceSize);
    mViewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0, 1.0f);

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
    std::vector<vk::PresentModeKHR> surfacePresentModes = mPhysicalDevice.getSurfacePresentModesKHR(mSurface);

    // check the surface width/height.
    if (!(surfaceCapabilities.currentExtent.width == -1 || surfaceCapabilities.currentExtent.height == -1)) {
        mSurfaceSize = surfaceCapabilities.currentExtent;
    }

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;

    for (vk::PresentModeKHR& pm : surfacePresentModes) {
        if (pm == vk::PresentModeKHR::eMailbox) {
            presentMode = vk::PresentModeKHR::eMailbox;
            break;
        }
    }

    std::vector<vk::SurfaceFormatKHR> surfaceFormats = mPhysicalDevice.getSurfaceFormatsKHR(mSurface);

    vk::Format surfaceColorFormat;
    vk::ColorSpaceKHR surfaceColorSpace;

    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
        surfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
    else
        surfaceColorFormat = surfaceFormats[0].format;

    surfaceColorSpace = surfaceFormats[0].colorSpace;

    mSwapchain = mDevice.createSwapchainKHR(
        vk::SwapchainCreateInfoKHR(
            vk::SwapchainCreateFlagsKHR(),
            mSurface,
            surfaceCapabilities.maxImageCount,
            surfaceColorFormat,
            surfaceColorSpace,
            mSurfaceSize,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive,
            1,
            &mQueueFamilyIndex,
            vk::SurfaceTransformFlagBitsKHR::eIdentity,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            presentMode,
            VK_TRUE,
            mSwapchain
        )
    );
}