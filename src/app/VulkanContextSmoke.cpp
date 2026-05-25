#include "app/VulkanContextSmoke.hpp"

#include "app/Application.hpp"
#include "app/SdlSurfaceProvider.hpp"
#include "app/Window.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanDevice.hpp"
#include "render/vulkan/VulkanFrameResources.hpp"
#include "render/vulkan/RayTracingPipeline.hpp"
#include "render/vulkan/VulkanAllocator.hpp"
#include "render/vulkan/VulkanBuffer.hpp"
#include "render/vulkan/VulkanRenderer.hpp"
#include "render/vulkan/VulkanRendererConfig.hpp"
#include "render/vulkan/ShaderModule.hpp"
#include "render/vulkan/VulkanSwapchain.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"
#include "util/logger.h"

#include <SDL3/SDL.h>
#include <fmt/format.h>
#include <internal_use_only/config.hpp>

#include <cstdint>
#include <filesystem>
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

int vulkan_shader_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan shader smoke", vulkan_rt::cmake::project_name),
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

  const std::filesystem::path shader_dir{vulkan_rt::cmake::shader_dir};
  const render::vulkan::ShaderModule raygen{device, shader_dir / "raygen.rgen.spv"};
  const render::vulkan::ShaderModule miss{device, shader_dir / "miss.rmiss.spv"};
  const render::vulkan::ShaderModule closest_hit{device, shader_dir / "closesthit.rchit.spv"};

  LOGI("Vulkan shader module smoke passed:");
  LOGI("  shader dir: {}", shader_dir.string());
  LOGI("  raygen module created: {}", raygen.module() != VK_NULL_HANDLE);
  LOGI("  miss module created: {}", miss.module() != VK_NULL_HANDLE);
  LOGI("  closest hit module created: {}", closest_hit.module() != VK_NULL_HANDLE);

  device.wait_idle();
  return 0;
}

int vulkan_buffer_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan buffer smoke", vulkan_rt::cmake::project_name),
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
  render::vulkan::VulkanAllocator allocator{context, device};

  render::vulkan::VulkanBuffer staging{
    allocator,
    render::vulkan::BufferCreateInfo{
      .size = 256,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    }};

  auto *data = static_cast<std::uint8_t *>(staging.map());
  for(std::uint32_t i = 0; i < 256; ++i) { data[i] = static_cast<std::uint8_t>(i); }
  staging.flush();
  staging.unmap();

  render::vulkan::VulkanBuffer device_buffer{
    allocator,
    render::vulkan::BufferCreateInfo{
      .size = 1024,
      .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = 0,
    }};
  const VkDeviceAddress device_buffer_address = device_buffer.device_address(device);

  LOGI("Vulkan buffer smoke passed:");
  LOGI("  allocator created: {}", allocator.allocator() != VK_NULL_HANDLE);
  LOGI("  staging buffer handle: {}", staging.buffer() != VK_NULL_HANDLE);
  LOGI("  staging size: {} bytes", staging.size());
  LOGI("  device buffer handle: {}", device_buffer.buffer() != VK_NULL_HANDLE);
  LOGI("  device buffer size: {} bytes", device_buffer.size());
  LOGI("  device buffer address: {}", device_buffer_address);

  device.wait_idle();
  return 0;
}

int vulkan_rt_pipeline_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan ray tracing pipeline smoke", vulkan_rt::cmake::project_name),
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

  const std::filesystem::path shader_dir{vulkan_rt::cmake::shader_dir};
  const render::vulkan::ShaderModule raygen{device, shader_dir / "raygen.rgen.spv"};
  const render::vulkan::ShaderModule miss{device, shader_dir / "miss.rmiss.spv"};
  const render::vulkan::ShaderModule closest_hit{device, shader_dir / "closesthit.rchit.spv"};
  const render::vulkan::RayTracingPipeline pipeline{device, raygen, miss, closest_hit};

  LOGI("Vulkan ray tracing pipeline smoke passed:");
  LOGI("  pipeline created: {}", pipeline.pipeline() != VK_NULL_HANDLE);
  LOGI("  pipeline layout created: {}", pipeline.layout() != VK_NULL_HANDLE);
  LOGI("  shader group count: {}", pipeline.shader_group_count());
  LOGI("  max recursion depth used: 1");

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
