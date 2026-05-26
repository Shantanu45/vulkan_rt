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
  std::uint32_t light_count = 0;
  std::uint32_t max_bounces = 2;
  std::uint32_t direct_lighting_enabled = 1;
  std::uint32_t jitter_enabled = 1;
  std::uint32_t _pad0 = 0;
  float exposure = 1.0F;
  float _pad1 = 0.0F;
  float _pad2 = 0.0F;
  float _pad3 = 0.0F;
};

static_assert(sizeof(GpuRayTracingFrameData) == 48);

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
