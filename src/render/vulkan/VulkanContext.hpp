/*****************************************************************
* \file   VulkanContext.hpp
* \brief  VulkanContext
* VkInstance
* debug messenger
* VkSurfaceKHR
* Vulkan loader/validation setup
*
* \author Shantanu Kumar
* \date   May 2026
*********************************************************************/
#pragma once
#include <volk.h>
#include "VulkanRendererConfig.hpp"
#include "SurfaceProvider.hpp"

namespace vulkan_rt::render::vulkan {

class VulkanContext
{
public:
  VulkanContext(const VulkanRendererConfig &config, SurfaceProvider &surface_provider);
  ~VulkanContext();

  [[nodiscard]] VkInstance instance() const;
  [[nodiscard]] VkSurfaceKHR surface() const;
  [[nodiscard]] bool validation_enabled() const;

  VulkanContext(const VulkanContext &) = delete;
  VulkanContext &operator=(const VulkanContext &) = delete;

  VulkanContext(VulkanContext &&) noexcept;
  VulkanContext &operator=(VulkanContext &&) noexcept;

private:
  void create_instance(const VulkanRendererConfig &config, std::span<const char *const> required_extensions);
  void create_debug_messenger();
  void create_surface(const SurfaceProvider &surface_provider);
  void destroy();

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  bool validation_enabled_ = false;
};
}// namespace vulkan_rt::render::vulkan
