/*****************************************************************
 * \file   VulkanFrameResources.hpp
 * \brief  VulkanFrameResources
 *	command pools
 *	command buffers
 *	fences
 *	semaphores
 *	per-frame indexing
 *
 * \author Shantanu Kumar
 * \date   May 2026
*********************************************************************/
#pragma once

#include <volk.h>

#include "VulkanDevice.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace vulkan_rt::render::vulkan
{
class VulkanFrameResources
{
public:
  VulkanFrameResources(const VulkanDevice &device, std::uint32_t frames_in_flight);
  ~VulkanFrameResources();

  VulkanFrameResources(const VulkanFrameResources &) = delete;
  VulkanFrameResources &operator=(const VulkanFrameResources &) = delete;

  VulkanFrameResources(VulkanFrameResources &&other) noexcept;
  VulkanFrameResources &operator=(VulkanFrameResources &&other) noexcept;

  [[nodiscard]] std::uint32_t frames_in_flight() const;
  [[nodiscard]] std::uint32_t current_frame_index() const;
  [[nodiscard]] VkCommandPool command_pool() const;
  [[nodiscard]] std::span<const VkCommandBuffer> command_buffers() const;
  [[nodiscard]] std::span<const VkSemaphore> image_available_semaphores() const;
  [[nodiscard]] std::span<const VkSemaphore> render_finished_semaphores() const;
  [[nodiscard]] std::span<const VkFence> in_flight_fences() const;

  void advance_frame();

private:
  void create(const VulkanDevice &device, std::uint32_t frames_in_flight);
  void destroy();

  VkDevice device_ = VK_NULL_HANDLE;
  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> command_buffers_;
  std::vector<VkSemaphore> image_available_semaphores_;
  std::vector<VkSemaphore> render_finished_semaphores_;
  std::vector<VkFence> in_flight_fences_;
  std::uint32_t current_frame_index_ = 0;
};
}
