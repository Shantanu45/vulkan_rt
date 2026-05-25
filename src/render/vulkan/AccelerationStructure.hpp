#pragma once

#include <volk.h>

#include "VulkanAllocator.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"

#include <memory>

namespace vulkan_rt::render::vulkan
{
struct AccelerationStructureCreateInfo
{
  VkAccelerationStructureTypeKHR type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  VkDeviceSize size = 0;
};

class AccelerationStructure
{
public:
  AccelerationStructure(
    const VulkanDevice &device,
    const VulkanAllocator &allocator,
    const AccelerationStructureCreateInfo &create_info);
  ~AccelerationStructure();

  AccelerationStructure(AccelerationStructure &&other) noexcept;
  AccelerationStructure &operator=(AccelerationStructure &&other) noexcept;

  [[nodiscard]] VkAccelerationStructureKHR acceleration_structure() const;
  [[nodiscard]] VkDeviceAddress device_address() const;
  [[nodiscard]] VkDeviceSize size() const;
  [[nodiscard]] VkAccelerationStructureTypeKHR type() const;
  [[nodiscard]] VkBuffer backing_buffer() const;

  
  AccelerationStructure(const AccelerationStructure &) = delete;
  AccelerationStructure &operator=(const AccelerationStructure &) = delete;

private:
  void create(
    const VulkanDevice &device,
    const VulkanAllocator &allocator,
    const AccelerationStructureCreateInfo &create_info);
  void destroy();

  VkDevice device_ = VK_NULL_HANDLE;
  VkAccelerationStructureKHR acceleration_structure_ = VK_NULL_HANDLE;
  VkDeviceAddress device_address_ = 0;
  VkDeviceSize size_ = 0;
  VkAccelerationStructureTypeKHR type_ = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  std::unique_ptr<VulkanBuffer> buffer_;
};
}
