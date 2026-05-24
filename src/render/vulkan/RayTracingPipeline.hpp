#pragma once

#include <volk.h>

#include "ShaderModule.hpp"
#include "VulkanDevice.hpp"

#include <cstdint>

namespace vulkan_rt::render::vulkan
{
class RayTracingPipeline
{
public:
  RayTracingPipeline(
    const VulkanDevice &device,
    const ShaderModule &raygen_shader,
    const ShaderModule &miss_shader,
    const ShaderModule &closest_hit_shader);
  ~RayTracingPipeline();

  RayTracingPipeline(const RayTracingPipeline &) = delete;
  RayTracingPipeline &operator=(const RayTracingPipeline &) = delete;

  RayTracingPipeline(RayTracingPipeline &&other) noexcept;
  RayTracingPipeline &operator=(RayTracingPipeline &&other) noexcept;

  [[nodiscard]] VkPipeline pipeline() const;
  [[nodiscard]] VkPipelineLayout layout() const;
  [[nodiscard]] std::uint32_t shader_group_count() const;

private:
  void create(
    const VulkanDevice &device,
    const ShaderModule &raygen_shader,
    const ShaderModule &miss_shader,
    const ShaderModule &closest_hit_shader);
  void destroy();

  VkDevice device_ = VK_NULL_HANDLE;
  VkPipelineLayout layout_ = VK_NULL_HANDLE;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
  std::uint32_t shader_group_count_ = 0;
};
}
