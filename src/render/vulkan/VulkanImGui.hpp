#pragma once

#include <volk.h>

#include "VulkanDevice.hpp"
#include "VulkanContext.hpp"
#include "VulkanSwapchain.hpp"

#include <cstdint>
#include <vector>

namespace vulkan_rt::render::vulkan
{
class VulkanImGui
{
public:
  VulkanImGui(
    const VulkanContext &context,
    const VulkanDevice &device,
    const VulkanSwapchain &swapchain,
    std::uint32_t frames_in_flight);
  ~VulkanImGui();

  VulkanImGui(const VulkanImGui &) = delete;
  VulkanImGui &operator=(const VulkanImGui &) = delete;
  VulkanImGui(VulkanImGui &&) = delete;
  VulkanImGui &operator=(VulkanImGui &&) = delete;

  void recreate_framebuffers(const VulkanSwapchain &swapchain);
  bool render(VkCommandBuffer command_buffer, std::uint32_t image_index, const VulkanSwapchain &swapchain);

private:
  void create_descriptor_pool();
  void create_render_pass(VkFormat swapchain_format);
  void create_framebuffers(const VulkanSwapchain &swapchain);
  void initialize_backend(
    const VulkanContext &context,
    const VulkanDevice &device,
    const VulkanSwapchain &swapchain,
    std::uint32_t frames_in_flight);
  void destroy_framebuffers();
  void destroy();

  VkDevice device_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
  VkRenderPass render_pass_ = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> framebuffers_;
};
}
