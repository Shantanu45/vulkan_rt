#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include "VulkanAllocator.hpp"
#include "VulkanDevice.hpp"

#include <cstdint>

namespace vulkan_rt::render::vulkan
{
struct ImageCreateInfo
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  VkImageUsageFlags usage = 0;
  VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO;
};

class VulkanImage
{
public:
  VulkanImage(const VulkanDevice &device, const VulkanAllocator &allocator, const ImageCreateInfo &create_info);
  ~VulkanImage();

  VulkanImage(const VulkanImage &) = delete;
  VulkanImage &operator=(const VulkanImage &) = delete;

  VulkanImage(VulkanImage &&other) noexcept;
  VulkanImage &operator=(VulkanImage &&other) noexcept;

  [[nodiscard]] VkImage image() const;
  [[nodiscard]] VkImageView image_view() const;
  [[nodiscard]] VkFormat format() const;
  [[nodiscard]] VkExtent2D extent() const;

private:
  void create(const VulkanDevice &device, const VulkanAllocator &allocator, const ImageCreateInfo &create_info);
  void destroy();

  VmaAllocator allocator_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkImage image_ = VK_NULL_HANDLE;
  VkImageView image_view_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  VkFormat format_ = VK_FORMAT_UNDEFINED;
  VkExtent2D extent_{};
};
}
