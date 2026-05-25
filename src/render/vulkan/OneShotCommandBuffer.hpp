#pragma once

#include <volk.h>

#include "VulkanDevice.hpp"

namespace vulkan_rt::render::vulkan
{
class OneShotCommandBuffer
{
public:
  explicit OneShotCommandBuffer(const VulkanDevice &device);
  ~OneShotCommandBuffer();

  OneShotCommandBuffer(const OneShotCommandBuffer &) = delete;
  OneShotCommandBuffer &operator=(const OneShotCommandBuffer &) = delete;

  [[nodiscard]] VkCommandBuffer command_buffer() const;

  void begin() const;
  void end_submit_and_wait() const;

private:
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
};
}
