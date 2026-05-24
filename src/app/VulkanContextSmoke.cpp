#include "app/VulkanContextSmoke.hpp"

#include "app/Application.hpp"
#include "app/SdlSurfaceProvider.hpp"
#include "app/Window.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanDevice.hpp"
#include "render/vulkan/VulkanRendererConfig.hpp"
#include "render/vulkan/VulkanSwapchain.hpp"
#include "util/logger.h"

#include <fmt/format.h>
#include <internal_use_only/config.hpp>

#include <string>

namespace vulkan_rt::app
{
int vulkan_context_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan context smoke", vulkan_rt::cmake::project_name),
    config.width,
    config.height,
  };

  SdlSurfaceProvider surface_provider{window.native_handle()};
  const std::string application_name{vulkan_rt::cmake::project_name};
  render::vulkan::VulkanRendererConfig vulkan_config{
    .validation = config.validation,
    .application_name = application_name.c_str(),
    .gpu_index = config.gpu_index,
  };

  render::vulkan::VulkanContext context{vulkan_config, surface_provider};

  LOGI("Vulkan context smoke passed:");
  LOGI("  instance created: {}", context.instance() != VK_NULL_HANDLE);
  LOGI("  surface created: {}", context.surface() != VK_NULL_HANDLE);
  LOGI("  validation enabled: {}", context.validation_enabled());

  return 0;
}

int vulkan_device_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan device smoke", vulkan_rt::cmake::project_name),
    config.width,
    config.height,
  };

  SdlSurfaceProvider surface_provider{window.native_handle()};
  const std::string application_name{vulkan_rt::cmake::project_name};
  render::vulkan::VulkanRendererConfig vulkan_config{
    .validation = config.validation,
    .application_name = application_name.c_str(),
    .gpu_index = config.gpu_index,
  };

  render::vulkan::VulkanContext context{vulkan_config, surface_provider};
  render::vulkan::VulkanDevice device{context, vulkan_config};

  LOGI("Vulkan device smoke passed:");
  LOGI("  physical device: {}", device.properties().deviceName);
  LOGI("  logical device created: {}", device.device() != VK_NULL_HANDLE);
  LOGI("  graphics queue family: {}", device.queue_families().graphics.value());
  LOGI("  present queue family: {}", device.queue_families().present.value());

  device.wait_idle();
  return 0;
}

int vulkan_swapchain_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan swapchain smoke", vulkan_rt::cmake::project_name),
    config.width,
    config.height,
  };

  SdlSurfaceProvider surface_provider{window.native_handle()};
  const std::string application_name{vulkan_rt::cmake::project_name};
  render::vulkan::VulkanRendererConfig vulkan_config{
    .validation = config.validation,
    .application_name = application_name.c_str(),
    .gpu_index = config.gpu_index,
  };

  render::vulkan::VulkanContext context{vulkan_config, surface_provider};
  render::vulkan::VulkanDevice device{context, vulkan_config};
  render::vulkan::VulkanSwapchain swapchain{
    context,
    device,
    render::vulkan::SwapchainExtent{
      .width = static_cast<std::uint32_t>(config.width),
      .height = static_cast<std::uint32_t>(config.height),
    },
  };

  LOGI("Vulkan swapchain smoke passed:");
  LOGI("  image count: {}", swapchain.images().size());
  LOGI("  image view count: {}", swapchain.image_views().size());
  LOGI("  image format: {}", static_cast<int>(swapchain.image_format()));
  LOGI("  extent: {}x{}", swapchain.extent().width, swapchain.extent().height);

  device.wait_idle();
  return 0;
}
}
