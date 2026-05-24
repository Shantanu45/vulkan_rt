#include "app/VulkanContextSmoke.hpp"

#include "app/Application.hpp"
#include "app/SdlSurfaceProvider.hpp"
#include "app/Window.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanDevice.hpp"
#include "render/vulkan/VulkanFrameResources.hpp"
#include "render/vulkan/VulkanRenderer.hpp"
#include "render/vulkan/VulkanRendererConfig.hpp"
#include "render/vulkan/VulkanSwapchain.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"
#include "util/logger.h"

#include <SDL3/SDL.h>
#include <fmt/format.h>
#include <internal_use_only/config.hpp>

#include <cstdint>
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
  LOGI("  shader group handle size: {}", device.ray_tracing_pipeline_properties().shaderGroupHandleSize);
  LOGI("  shader group base alignment: {}", device.ray_tracing_pipeline_properties().shaderGroupBaseAlignment);
  LOGI("  max ray recursion depth: {}", device.ray_tracing_pipeline_properties().maxRayRecursionDepth);
  LOGI("  max geometry count: {}", device.acceleration_structure_properties().maxGeometryCount);

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

int vulkan_frame_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan frame smoke", vulkan_rt::cmake::project_name),
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
  render::vulkan::VulkanFrameResources frames{device, 2};

  LOGI("Vulkan frame resources smoke passed:");
  LOGI("  frames in flight: {}", frames.frames_in_flight());
  LOGI("  command pool created: {}", frames.command_pool() != VK_NULL_HANDLE);
  LOGI("  command buffers: {}", frames.command_buffers().size());
  LOGI("  image available semaphores: {}", frames.image_available_semaphores().size());
  LOGI("  render finished semaphores: {}", frames.render_finished_semaphores().size());
  LOGI("  fences: {}", frames.in_flight_fences().size());
  LOGI("  swapchain images available: {}", swapchain.images().size());

  device.wait_idle();
  return 0;
}

int vulkan_clear_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan clear smoke", vulkan_rt::cmake::project_name),
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

  render::vulkan::VulkanRenderer renderer{
    vulkan_config,
    surface_provider,
    render::vulkan::SwapchainExtent{
      .width = static_cast<std::uint32_t>(config.width),
      .height = static_cast<std::uint32_t>(config.height),
    },
  };

  const auto procedural_scene = scene::make_procedural_scene();
  const scene::Camera camera;

  for(std::uint64_t frame = 0; frame < 8; ++frame)
  {
    renderer.render(render::RenderFrameInfo{.frame_index = frame}, procedural_scene, camera);
    SDL_Delay(16);
  }

  renderer.wait_idle();
  LOGI("Vulkan clear smoke passed: presented 8 clear frames");
  return 0;
}

int vulkan_resize_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan resize smoke", vulkan_rt::cmake::project_name),
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

  render::vulkan::VulkanRenderer renderer{
    vulkan_config,
    surface_provider,
    render::vulkan::SwapchainExtent{
      .width = static_cast<std::uint32_t>(config.width),
      .height = static_cast<std::uint32_t>(config.height),
    },
  };

  const auto procedural_scene = scene::make_procedural_scene();
  const scene::Camera camera;

  for(std::uint64_t frame = 0; frame < 4; ++frame)
  {
    renderer.render(render::RenderFrameInfo{.frame_index = frame}, procedural_scene, camera);
    SDL_Delay(16);
  }

  const int resized_width = config.width + 64;
  const int resized_height = config.height + 64;
  SDL_SetWindowSize(window.native_handle(), resized_width, resized_height);
  renderer.resize(resized_width, resized_height);

  for(std::uint64_t frame = 4; frame < 8; ++frame)
  {
    renderer.render(render::RenderFrameInfo{.frame_index = frame}, procedural_scene, camera);
    SDL_Delay(16);
  }

  renderer.wait_idle();
  LOGI("Vulkan resize smoke passed: recreated swapchain through renderer.resize()");
  return 0;
}
}
