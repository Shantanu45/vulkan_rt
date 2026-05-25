#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include "VulkanAllocator.hpp"

namespace vulkan_rt::render::vulkan
{
struct BufferCreateInfo
{
  VkDeviceSize size = 0;
  VkBufferUsageFlags usage = 0;
  VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO;
  VmaAllocationCreateFlags alloc_flags = 0;
};

class VulkanBuffer
{
public:
  VulkanBuffer(const VulkanAllocator &allocator, const BufferCreateInfo &create_info);
  ~VulkanBuffer();

  VulkanBuffer(const VulkanBuffer &) = delete;
  VulkanBuffer &operator=(const VulkanBuffer &) = delete;

  VulkanBuffer(VulkanBuffer &&other) noexcept;
  VulkanBuffer &operator=(VulkanBuffer &&other) noexcept;

  [[nodiscard]] VkBuffer buffer() const;
  [[nodiscard]] VmaAllocation allocation() const;
  [[nodiscard]] VkDeviceSize size() const;

  [[nodiscard]] void *map();
  void unmap();
  void flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
  void invalidate(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

private:
  void create(const VulkanAllocator &allocator, const BufferCreateInfo &create_info);
  void destroy();

  VmaAllocator allocator_ = VK_NULL_HANDLE;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  VkDeviceSize size_ = 0;
  bool mapped_ = false;
};
}
