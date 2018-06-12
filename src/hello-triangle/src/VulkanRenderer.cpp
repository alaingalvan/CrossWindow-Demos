#include "Renderer.h"
#include "CrossWindow/Graphics/Graphics.h"

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
}

Render::Renderer(xwin::Window& window)
{
  vk::ApplicationInfo appInfo(
    "Hello Triangle",
    0,
    "HelloTriangleEngine",
    0,
    VK_API_VERSION_1_0
  );

  std::vector<const char*> layers = {}, extensions =
  {
        VK_KHR_SURFACE_EXTENSION_NAME,
  #ifdef VK_USE_PLATFORM_WIN32_KHR
        eVK_KHR_WIN32_SURFACE_EXTENSION_NAME
  #elif VK_USE_PLATFORM_MACOS_MVK
        eVK_MVK_MACOS_SURFACE_EXTENSION_NAME;
  #elif VK_USE_PLATFORM_XCB_KHR
        eVK_KHR_XCB_SURFACE_EXTENSION_NAME;
  #elif VK_USE_PLATFORM_ANDROID_KHR
        eVK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
  #elif VK_USE_PLATFORM_XLIB_KHR
        eVK_KHR_XLIB_SURFACE_EXTENSION_NAME;
  #elif VK_USE_PLATFORM_XCB_KHR
        eVK_KHR_XCB_SURFACE_EXTENSION_NAME;
  #elif VK_USE_PLATFORM_WAYLAND_KHR
        eVK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
  #elif VK_USE_PLATFORM_MIR_KHR || VK_USE_PLATFORM_DISPLAY_KHR
        eVK_KHR_DISPLAY_EXTENSION_NAME;
  #elif VK_USE_PLATFORM_ANDROID_KHR
        eVK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
  #elif VK_USE_PLATFORM_IOS_MVK
        eVK_MVK_IOS_SURFACE_EXTENSION_NAME;
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
  vk::SurfaceKHR surface = xgfx::getSurface(&window, mInstance);

  std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
  mPhysicalDevice = physicalDevices[0];

  mQueueFamilyIndex = mag::getQueueIndex(mPhysicalDevice, vk::QueueFlagBits::eGraphics);

  // Queue Creation
  vk::DeviceQueueCreateInfo qcinfo;
  qcinfo.setQueueFamilyIndex(mQueueFamilyIndex);
  qcinfo.setQueueCount(1);
  mQueuePriority = 0.5f;
  qcinfo.setPQueuePriorities(&mQueuePriority);

  // Logical Device
  vk::DeviceCreateInfo dinfo;
  dinfo.setPQueueCreateInfos(&qcinfo);
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
          mQueueFamilyIndex));

  // Command Buffers
  mCommandBuffers = mDevice.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(
          mCommandPool,
          vk::CommandBufferLevel::ePrimary,
          1));

  //Swapchain
  const xwin::WindowDesc wdesc = window.getDesc();
  resize(wDesc.width, wDesc.height);

  //Data

}

Renderer::render()
{
  std::vector<vk::ClearValue> clearValues =
  {
      vk::ClearColorValue(
          std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f}),
      vk::ClearDepthStencilValue(1.0f, 0)
  };

  for (vk::CommandBuffer &cmd : mCommandBuffers)
  {
    cmd.begin(vk::CommandBufferBeginInfo());
    cmd.beginRenderPass(
    vk::RenderPassBeginInfo(
        renderpass,
        swapchainBuffers[i].frameBuffer,
        renderArea,
        clearValues.size(),
        clearValues.data()),
    vk::SubpassContents::eInline);

    cmd.setViewport(0, viewports);

    cmd.setScissor(0, scissors);

    // Bind Descriptor Sets, these are attribute/uniform "descriptions"
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipelineLayout,
    0,
    descriptorSets,
    nullptr);
    cmd.bindVertexBuffers(0, 1, &vertices.buffer, offsets.data());
    cmd.bindIndexBuffer(indices.buffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(indices.count, 1, 0, 0, 1);
    cmd.endRenderPass();
    cmd.end();
  }

  vk::SubmitInfo submitInfo;
  
  vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  submitInfo
    .setWaitSemaphoreCount(1)
    .setPWaitSemaphores(&presentCompleteSemaphore)
    .setPWaitDstStageMask(&waitDstStageMask)
    .setCommandBufferCount(mCommandBuffers.size())
    .setPCommandBuffers(mCommandBuffers.data())
    .setSignalSemaphoreCount(1)
    .setPSignalSemaphores(&mRenderCompleteSemaphore);

  mQueue.submit(1, &submitInfo, mWaitFence);

  device.freeCommandBuffers(mCommandPool, 1, &commandBuffer);
}

Renderer::resize(unsigned width, unsigned height)
{
  vk::Extent2D surfaceSize(width, height);

  vk::SurfaceCapabilitiesKHR surfaceCapabilities = mDevice.getSurfaceCapabilitiesKHR(surface);
	std::vector<vk::PresentModeKHR> surfacePresentModes = mDevice.getSurfacePresentModesKHR(surface);

	// check the surface width/height.
	if (!(surfaceCapabilities.currentExtent.width == -1 || surfaceCapabilities.currentExtent.height == -1)) {
		surfaceSize = surfaceCapabilities.currentExtent;
	}

	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;

	for (vk::PresentModeKHR& pm : surfacePresentModes) {
		if (pm == vk::PresentModeKHR::eMailbox) {
			presentMode = vk::PresentModeKHR::eMailbox;
			break;
		}
	}

  mSwapchain = device.createSwapchainKHR(
		vk::SwapchainCreateInfoKHR(
			vk::SwapchainCreateFlagsKHR(),
			mSurface,
			surfaceCapabilities.maxImageCount,
			surfaceColorFormat,
			surfaceColorSpace,
			surfaceSize,
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