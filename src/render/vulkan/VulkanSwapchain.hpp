/*****************************************************************
* \file   VulkanSwapchain.hpp
* \brief  VulkanSwapchain
*	swapchain
*	image handles
*	image views
*	format
*	extent
*	recreate logic
*
* \author Shantanu Kumar
* \date   May 2026
*********************************************************************/
#pragma once

#include <volk.h>

#include "VulkanContext.hpp"
#include "VulkanDevice.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace vulkan_rt::render::vulkan
{
struct SwapchainExtent
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

class VulkanSwapchain
{
public:
  VulkanSwapchain(const VulkanContext &context, const VulkanDevice &device, SwapchainExtent requested_extent);
  ~VulkanSwapchain();

  VulkanSwapchain(const VulkanSwapchain &) = delete;
  VulkanSwapchain &operator=(const VulkanSwapchain &) = delete;

  VulkanSwapchain(VulkanSwapchain &&other) noexcept;
  VulkanSwapchain &operator=(VulkanSwapchain &&other) noexcept;

  [[nodiscard]] VkSwapchainKHR swapchain() const;
  [[nodiscard]] VkFormat image_format() const;
  [[nodiscard]] VkExtent2D extent() const;
  [[nodiscard]] std::span<const VkImage> images() const;
  [[nodiscard]] std::span<const VkImageView> image_views() const;

private:
  void create(const VulkanContext &context, const VulkanDevice &device, SwapchainExtent requested_extent);
  void destroy();

  VkDevice device_ = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat image_format_ = VK_FORMAT_UNDEFINED;
  VkExtent2D extent_{};
  std::vector<VkImage> images_;
  std::vector<VkImageView> image_views_;
};
}
