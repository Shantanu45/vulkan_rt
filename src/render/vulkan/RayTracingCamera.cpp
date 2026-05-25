#include "RayTracingCamera.hpp"

#include "RayTracingCameraData.hpp"

namespace vulkan_rt::render::vulkan
{
RayTracingCamera::RayTracingCamera(const VulkanAllocator &allocator, const scene::Camera &camera)
  : buffer_(allocator,
      BufferCreateInfo{
        .size = sizeof(RayTracingCameraData),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_usage = VMA_MEMORY_USAGE_AUTO,
        .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      })
{
  update(camera);
}

void RayTracingCamera::update(const scene::Camera &camera)
{
  const auto gpu_camera = make_ray_tracing_camera_data(camera);
  auto *data = static_cast<RayTracingCameraData *>(buffer_.map());
  *data = gpu_camera;
  buffer_.flush();
  buffer_.unmap();
}

const VulkanBuffer &RayTracingCamera::buffer() const
{
  return buffer_;
}
}
