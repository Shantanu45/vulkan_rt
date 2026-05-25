#pragma once

#include <volk.h>

#include "RayTracingPipeline.hpp"
#include "VulkanAllocator.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"

#include <memory>

namespace vulkan_rt::render::vulkan
{
class ShaderBindingTable
{
public:
  ShaderBindingTable(
    const VulkanDevice &device, const VulkanAllocator &allocator, const RayTracingPipeline &pipeline);
  ~ShaderBindingTable();

  ShaderBindingTable(ShaderBindingTable &&other) noexcept;
  ShaderBindingTable &operator=(ShaderBindingTable &&other) noexcept;

  [[nodiscard]] const VkStridedDeviceAddressRegionKHR &raygen_region() const;
  [[nodiscard]] const VkStridedDeviceAddressRegionKHR &miss_region() const;
  [[nodiscard]] const VkStridedDeviceAddressRegionKHR &hit_region() const;
  [[nodiscard]] const VkStridedDeviceAddressRegionKHR &callable_region() const;
  [[nodiscard]] VkBuffer buffer() const;
  [[nodiscard]] VkDeviceSize size() const;

  ShaderBindingTable(const ShaderBindingTable &) = delete;
  ShaderBindingTable &operator=(const ShaderBindingTable &) = delete;

private:
  void create(const VulkanDevice &device, const VulkanAllocator &allocator, const RayTracingPipeline &pipeline);

  std::unique_ptr<VulkanBuffer> buffer_;
  VkDeviceSize size_ = 0;
  VkStridedDeviceAddressRegionKHR raygen_region_{};
  VkStridedDeviceAddressRegionKHR miss_region_{};
  VkStridedDeviceAddressRegionKHR hit_region_{};
  VkStridedDeviceAddressRegionKHR callable_region_{};
};
}
