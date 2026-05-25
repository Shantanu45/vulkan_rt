#pragma once

#include "VulkanAllocator.hpp"
#include "VulkanBuffer.hpp"

#include <cstdint>

namespace vulkan_rt::render::vulkan
{
struct GpuRayTracingFrameData
{
  std::uint32_t sample_index = 0;
  std::uint32_t frame_index = 0;
  std::uint32_t reset_accumulation = 0;
  std::uint32_t _padding = 0;
};

static_assert(sizeof(GpuRayTracingFrameData) == 16);

class RayTracingFrameData
{
public:
  explicit RayTracingFrameData(const VulkanAllocator &allocator);

  void update(const GpuRayTracingFrameData &frame_data);

  [[nodiscard]] const VulkanBuffer &buffer() const;

private:
  VulkanBuffer buffer_;
};
}
