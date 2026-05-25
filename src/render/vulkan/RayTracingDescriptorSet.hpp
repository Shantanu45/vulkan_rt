#pragma once

#include <volk.h>

#include "AccelerationStructure.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanImage.hpp"

namespace vulkan_rt::render::vulkan
{
class RayTracingDescriptorSet
{
public:
  RayTracingDescriptorSet(
    const VulkanDevice &device,
    const AccelerationStructure &tlas,
    const VulkanImage &output_image,
    const VulkanBuffer &material_indices,
    const VulkanBuffer &materials,
    const VulkanBuffer &camera);
  ~RayTracingDescriptorSet();

  RayTracingDescriptorSet(RayTracingDescriptorSet &&other) noexcept;
  RayTracingDescriptorSet &operator=(RayTracingDescriptorSet &&other) noexcept;

  [[nodiscard]] VkDescriptorSetLayout layout() const;
  [[nodiscard]] VkDescriptorPool pool() const;
  [[nodiscard]] VkDescriptorSet descriptor_set() const;

  RayTracingDescriptorSet(const RayTracingDescriptorSet &) = delete;
  RayTracingDescriptorSet &operator=(const RayTracingDescriptorSet &) = delete;

private:
  void create(
    const VulkanDevice &device,
    const AccelerationStructure &tlas,
    const VulkanImage &output_image,
    const VulkanBuffer &material_indices,
    const VulkanBuffer &materials,
    const VulkanBuffer &camera);
  void destroy();

  VkDevice device_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
  VkDescriptorPool pool_ = VK_NULL_HANDLE;
  VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
};
}
