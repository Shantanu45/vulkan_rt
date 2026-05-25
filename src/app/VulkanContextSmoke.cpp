#include "app/VulkanContextSmoke.hpp"

#include "app/Application.hpp"
#include "app/SdlSurfaceProvider.hpp"
#include "app/Window.hpp"
#include "render/vulkan/AccelerationStructure.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanDevice.hpp"
#include "render/vulkan/VulkanFrameResources.hpp"
#include "render/vulkan/RayTracingPipeline.hpp"
#include "render/vulkan/ShaderBindingTable.hpp"
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

#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

namespace vulkan_rt::app
{
namespace
{
std::string vulkan_result_message(const char *operation, VkResult result)
{
  return std::string{operation} + " failed with VkResult " + std::to_string(static_cast<int>(result));
}

void throw_if_failed(VkResult result, const char *operation)
{
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error(vulkan_result_message(operation, result));
  }
}

class OneShotCommandBuffer
{
public:
  explicit OneShotCommandBuffer(const render::vulkan::VulkanDevice &device)
    : device_(device.device())
    , graphics_queue_(device.graphics_queue())
  {
    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    command_pool_info.queueFamilyIndex = device.queue_families().graphics.value();
    throw_if_failed(vkCreateCommandPool(device_, &command_pool_info, nullptr, &command_pool_), "vkCreateCommandPool");

    VkCommandBufferAllocateInfo command_buffer_info{};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.commandPool = command_pool_;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;
    throw_if_failed(vkAllocateCommandBuffers(device_, &command_buffer_info, &command_buffer_), "vkAllocateCommandBuffers");
  }

  ~OneShotCommandBuffer()
  {
    if(command_pool_ != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(device_, command_pool_, nullptr);
    }
  }

  OneShotCommandBuffer(const OneShotCommandBuffer &) = delete;
  OneShotCommandBuffer &operator=(const OneShotCommandBuffer &) = delete;

  [[nodiscard]] VkCommandBuffer command_buffer() const
  {
    return command_buffer_;
  }

  void begin() const
  {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    throw_if_failed(vkBeginCommandBuffer(command_buffer_, &begin_info), "vkBeginCommandBuffer");
  }

  void end_submit_and_wait() const
  {
    throw_if_failed(vkEndCommandBuffer(command_buffer_), "vkEndCommandBuffer");

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = VK_NULL_HANDLE;
    throw_if_failed(vkCreateFence(device_, &fence_info, nullptr, &fence), "vkCreateFence");

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;

    const VkResult submit_result = vkQueueSubmit(graphics_queue_, 1, &submit_info, fence);
    if(submit_result != VK_SUCCESS)
    {
      vkDestroyFence(device_, fence, nullptr);
      throw_if_failed(submit_result, "vkQueueSubmit");
    }

    const VkResult wait_result =
      vkWaitForFences(device_, 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
    vkDestroyFence(device_, fence, nullptr);
    throw_if_failed(wait_result, "vkWaitForFences");
  }

private:
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
};
}

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

int vulkan_sbt_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan SBT smoke", vulkan_rt::cmake::project_name),
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

  const std::filesystem::path shader_dir{vulkan_rt::cmake::shader_dir};
  const render::vulkan::ShaderModule raygen{device, shader_dir / "raygen.rgen.spv"};
  const render::vulkan::ShaderModule miss{device, shader_dir / "miss.rmiss.spv"};
  const render::vulkan::ShaderModule closest_hit{device, shader_dir / "closesthit.rchit.spv"};
  const render::vulkan::RayTracingPipeline pipeline{device, raygen, miss, closest_hit};
  const render::vulkan::ShaderBindingTable sbt{device, allocator, pipeline};

  LOGI("Vulkan shader binding table smoke passed:");
  LOGI("  SBT buffer created: {}", sbt.buffer() != VK_NULL_HANDLE);
  LOGI("  SBT size: {} bytes", sbt.size());
  LOGI("  raygen address/stride/size: {}/{}/{}",
    sbt.raygen_region().deviceAddress,
    sbt.raygen_region().stride,
    sbt.raygen_region().size);
  LOGI("  miss address/stride/size: {}/{}/{}",
    sbt.miss_region().deviceAddress,
    sbt.miss_region().stride,
    sbt.miss_region().size);
  LOGI("  hit address/stride/size: {}/{}/{}",
    sbt.hit_region().deviceAddress,
    sbt.hit_region().stride,
    sbt.hit_region().size);

  device.wait_idle();
  return 0;
}

int vulkan_acceleration_structure_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan acceleration structure smoke", vulkan_rt::cmake::project_name),
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

  const render::vulkan::AccelerationStructure acceleration_structure{
    device,
    allocator,
    render::vulkan::AccelerationStructureCreateInfo{
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .size = 4096,
    },
  };

  LOGI("Vulkan acceleration structure smoke passed:");
  LOGI("  handle created: {}", acceleration_structure.acceleration_structure() != VK_NULL_HANDLE);
  LOGI("  backing buffer created: {}", acceleration_structure.backing_buffer() != VK_NULL_HANDLE);
  LOGI("  size: {} bytes", acceleration_structure.size());
  LOGI("  device address: {}", acceleration_structure.device_address());

  device.wait_idle();
  return 0;
}

int vulkan_triangle_blas_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan triangle BLAS smoke", vulkan_rt::cmake::project_name),
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

  constexpr std::array<float, 9> vertices{
    0.0F,
    -0.5F,
    0.0F,
    0.5F,
    0.5F,
    0.0F,
    -0.5F,
    0.5F,
    0.0F,
  };

  render::vulkan::VulkanBuffer vertex_buffer{
    allocator,
    render::vulkan::BufferCreateInfo{
      .size = static_cast<VkDeviceSize>(vertices.size() * sizeof(float)),
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    }};

  auto *vertex_data = static_cast<float *>(vertex_buffer.map());
  std::copy(vertices.begin(), vertices.end(), vertex_data);
  vertex_buffer.flush();
  vertex_buffer.unmap();

  VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
  triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
  triangles.vertexData.deviceAddress = vertex_buffer.device_address(device);
  triangles.vertexStride = 3 * sizeof(float);
  triangles.maxVertex = 3;
  triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

  VkAccelerationStructureGeometryKHR geometry{};
  geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  geometry.geometry.triangles = triangles;
  geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

  VkAccelerationStructureBuildGeometryInfoKHR build_info{};
  build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  build_info.geometryCount = 1;
  build_info.pGeometries = &geometry;

  constexpr std::uint32_t primitive_count = 1;
  VkAccelerationStructureBuildSizesInfoKHR build_sizes{};
  build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  vkGetAccelerationStructureBuildSizesKHR(device.device(),
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &build_info,
    &primitive_count,
    &build_sizes);

  render::vulkan::AccelerationStructure blas{
    device,
    allocator,
    render::vulkan::AccelerationStructureCreateInfo{
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .size = build_sizes.accelerationStructureSize,
    },
  };

  render::vulkan::VulkanBuffer scratch_buffer{
    allocator,
    render::vulkan::BufferCreateInfo{
      .size = build_sizes.buildScratchSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = 0,
    }};

  build_info.dstAccelerationStructure = blas.acceleration_structure();
  build_info.scratchData.deviceAddress = scratch_buffer.device_address(device);

  VkAccelerationStructureBuildRangeInfoKHR build_range{};
  build_range.primitiveCount = primitive_count;
  build_range.primitiveOffset = 0;
  build_range.firstVertex = 0;
  build_range.transformOffset = 0;
  const VkAccelerationStructureBuildRangeInfoKHR *build_ranges[] = {&build_range};

  OneShotCommandBuffer command_buffer{device};
  command_buffer.begin();
  vkCmdBuildAccelerationStructuresKHR(command_buffer.command_buffer(), 1, &build_info, build_ranges);
  command_buffer.end_submit_and_wait();

  LOGI("Vulkan triangle BLAS smoke passed:");
  LOGI("  vertex buffer address: {}", vertex_buffer.device_address(device));
  LOGI("  BLAS size: {} bytes", blas.size());
  LOGI("  BLAS device address: {}", blas.device_address());
  LOGI("  scratch size: {} bytes", scratch_buffer.size());

  device.wait_idle();
  return 0;
}

int vulkan_tlas_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan TLAS smoke", vulkan_rt::cmake::project_name),
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

  constexpr std::array<float, 9> vertices{
    0.0F,
    -0.5F,
    0.0F,
    0.5F,
    0.5F,
    0.0F,
    -0.5F,
    0.5F,
    0.0F,
  };

  render::vulkan::VulkanBuffer vertex_buffer{
    allocator,
    render::vulkan::BufferCreateInfo{
      .size = static_cast<VkDeviceSize>(vertices.size() * sizeof(float)),
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    }};

  auto *vertex_data = static_cast<float *>(vertex_buffer.map());
  std::copy(vertices.begin(), vertices.end(), vertex_data);
  vertex_buffer.flush();
  vertex_buffer.unmap();

  VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
  triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
  triangles.vertexData.deviceAddress = vertex_buffer.device_address(device);
  triangles.vertexStride = 3 * sizeof(float);
  triangles.maxVertex = 3;
  triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

  VkAccelerationStructureGeometryKHR blas_geometry{};
  blas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  blas_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  blas_geometry.geometry.triangles = triangles;
  blas_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

  VkAccelerationStructureBuildGeometryInfoKHR blas_build_info{};
  blas_build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  blas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  blas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  blas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  blas_build_info.geometryCount = 1;
  blas_build_info.pGeometries = &blas_geometry;

  constexpr std::uint32_t triangle_count = 1;
  VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes{};
  blas_build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  vkGetAccelerationStructureBuildSizesKHR(device.device(),
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &blas_build_info,
    &triangle_count,
    &blas_build_sizes);

  render::vulkan::AccelerationStructure blas{
    device,
    allocator,
    render::vulkan::AccelerationStructureCreateInfo{
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .size = blas_build_sizes.accelerationStructureSize,
    },
  };

  render::vulkan::VulkanBuffer blas_scratch{
    allocator,
    render::vulkan::BufferCreateInfo{
      .size = blas_build_sizes.buildScratchSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = 0,
    }};

  blas_build_info.dstAccelerationStructure = blas.acceleration_structure();
  blas_build_info.scratchData.deviceAddress = blas_scratch.device_address(device);

  VkAccelerationStructureBuildRangeInfoKHR blas_build_range{};
  blas_build_range.primitiveCount = triangle_count;
  const VkAccelerationStructureBuildRangeInfoKHR *blas_build_ranges[] = {&blas_build_range};

  {
    OneShotCommandBuffer command_buffer{device};
    command_buffer.begin();
    vkCmdBuildAccelerationStructuresKHR(command_buffer.command_buffer(), 1, &blas_build_info, blas_build_ranges);
    command_buffer.end_submit_and_wait();
  }

  VkAccelerationStructureInstanceKHR instance{};
  instance.transform.matrix[0][0] = 1.0F;
  instance.transform.matrix[1][1] = 1.0F;
  instance.transform.matrix[2][2] = 1.0F;
  instance.instanceCustomIndex = 0;
  instance.mask = 0xFF;
  instance.instanceShaderBindingTableRecordOffset = 0;
  instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
  instance.accelerationStructureReference = blas.device_address();

  render::vulkan::VulkanBuffer instance_buffer{
    allocator,
    render::vulkan::BufferCreateInfo{
      .size = sizeof(VkAccelerationStructureInstanceKHR),
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    }};

  auto *instance_data = static_cast<VkAccelerationStructureInstanceKHR *>(instance_buffer.map());
  *instance_data = instance;
  instance_buffer.flush();
  instance_buffer.unmap();

  VkAccelerationStructureGeometryInstancesDataKHR instances_data{};
  instances_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  instances_data.arrayOfPointers = VK_FALSE;
  instances_data.data.deviceAddress = instance_buffer.device_address(device);

  VkAccelerationStructureGeometryKHR tlas_geometry{};
  tlas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  tlas_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  tlas_geometry.geometry.instances = instances_data;

  VkAccelerationStructureBuildGeometryInfoKHR tlas_build_info{};
  tlas_build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  tlas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  tlas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  tlas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  tlas_build_info.geometryCount = 1;
  tlas_build_info.pGeometries = &tlas_geometry;

  constexpr std::uint32_t instance_count = 1;
  VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes{};
  tlas_build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  vkGetAccelerationStructureBuildSizesKHR(device.device(),
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &tlas_build_info,
    &instance_count,
    &tlas_build_sizes);

  render::vulkan::AccelerationStructure tlas{
    device,
    allocator,
    render::vulkan::AccelerationStructureCreateInfo{
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .size = tlas_build_sizes.accelerationStructureSize,
    },
  };

  render::vulkan::VulkanBuffer tlas_scratch{
    allocator,
    render::vulkan::BufferCreateInfo{
      .size = tlas_build_sizes.buildScratchSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = 0,
    }};

  tlas_build_info.dstAccelerationStructure = tlas.acceleration_structure();
  tlas_build_info.scratchData.deviceAddress = tlas_scratch.device_address(device);

  VkAccelerationStructureBuildRangeInfoKHR tlas_build_range{};
  tlas_build_range.primitiveCount = instance_count;
  const VkAccelerationStructureBuildRangeInfoKHR *tlas_build_ranges[] = {&tlas_build_range};

  {
    OneShotCommandBuffer command_buffer{device};
    command_buffer.begin();
    vkCmdBuildAccelerationStructuresKHR(command_buffer.command_buffer(), 1, &tlas_build_info, tlas_build_ranges);
    command_buffer.end_submit_and_wait();
  }

  LOGI("Vulkan TLAS smoke passed:");
  LOGI("  BLAS address: {}", blas.device_address());
  LOGI("  TLAS address: {}", tlas.device_address());
  LOGI("  instance buffer address: {}", instance_buffer.device_address(device));
  LOGI("  TLAS size: {} bytes", tlas.size());
  LOGI("  TLAS scratch size: {} bytes", tlas_scratch.size());

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
