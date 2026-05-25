#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include "VulkanContext.hpp"
#include "VulkanDevice.hpp"

namespace vulkan_rt::render::vulkan
{
class VulkanAllocator
{
public:
  VulkanAllocator(const VulkanContext &context, const VulkanDevice &device);
  ~VulkanAllocator();

  VulkanAllocator(const VulkanAllocator &) = delete;
  VulkanAllocator &operator=(const VulkanAllocator &) = delete;

  VulkanAllocator(VulkanAllocator &&other) noexcept;
  VulkanAllocator &operator=(VulkanAllocator &&other) noexcept;

  [[nodiscard]] VmaAllocator allocator() const;

private:
  void create(const VulkanContext &context, const VulkanDevice &device);
  void destroy();

  VmaAllocator allocator_ = VK_NULL_HANDLE;
};
}
