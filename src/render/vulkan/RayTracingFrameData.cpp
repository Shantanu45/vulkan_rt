#include "RayTracingFrameData.hpp"

namespace vulkan_rt::render::vulkan
{
RayTracingFrameData::RayTracingFrameData(const VulkanAllocator &allocator)
  : buffer_(allocator,
      BufferCreateInfo{
        .size = sizeof(GpuRayTracingFrameData),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_usage = VMA_MEMORY_USAGE_AUTO,
        .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      })
{
  update(GpuRayTracingFrameData{});
}

void RayTracingFrameData::update(const GpuRayTracingFrameData &frame_data)
{
  auto *data = static_cast<GpuRayTracingFrameData *>(buffer_.map());
  *data = frame_data;
  buffer_.flush();
  buffer_.unmap();
}

const VulkanBuffer &RayTracingFrameData::buffer() const
{
  return buffer_;
}
}
