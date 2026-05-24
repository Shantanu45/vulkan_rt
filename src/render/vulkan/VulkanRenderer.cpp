#include "VulkanRenderer.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace vulkan_rt::render::vulkan {
namespace
{
std::string vulkan_result_message(const char *operation, VkResult result)
{
  return std::string{operation} + " failed with VkResult " + std::to_string(static_cast<int>(result));
}

void throw_if_failed(VkResult result, const char *operation)
{
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error(vulkan_result_message(operation, result));
  }
}

void transition_image_layout(
  VkCommandBuffer command_buffer,
  VkImage image,
  VkImageLayout old_layout,
  VkImageLayout new_layout,
  VkAccessFlags src_access_mask,
  VkAccessFlags dst_access_mask,
  VkPipelineStageFlags src_stage,
  VkPipelineStageFlags dst_stage)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = src_access_mask;
  barrier.dstAccessMask = dst_access_mask;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
}

VulkanRenderer::VulkanRenderer(
  const VulkanRendererConfig &config, const SurfaceProvider &surface_provider, SwapchainExtent requested_extent)
  : context_(config, surface_provider)
  , device_(context_, config)
  , swapchain_(context_, device_, requested_extent)
  , frames_(device_, 2)
{
  swapchain_image_layouts_.assign(swapchain_.images().size(), VK_IMAGE_LAYOUT_UNDEFINED);
}

void VulkanRenderer::render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera)
{
  static_cast<void>(frame_info);
  static_cast<void>(scene);
  static_cast<void>(camera);

  recreate_swapchain_if_needed();

  const auto frame_index = frames_.current_frame_index();
  const auto command_buffer = frames_.command_buffers()[frame_index];
  const auto image_available = frames_.image_available_semaphores()[frame_index];
  const auto render_finished = frames_.render_finished_semaphores()[frame_index];
  const auto in_flight_fence = frames_.in_flight_fences()[frame_index];

  throw_if_failed(
    vkWaitForFences(device_.device(), 1, &in_flight_fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()),
    "vkWaitForFences");

  std::uint32_t image_index = 0;
  const VkResult acquire_result = vkAcquireNextImageKHR(device_.device(),
    swapchain_.swapchain(),
    std::numeric_limits<std::uint64_t>::max(),
    image_available,
    VK_NULL_HANDLE,
    &image_index);
  if(acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
  {
    throw_if_failed(acquire_result, "vkAcquireNextImageKHR");
  }

  throw_if_failed(vkResetFences(device_.device(), 1, &in_flight_fence), "vkResetFences");
  throw_if_failed(vkResetCommandBuffer(command_buffer, 0), "vkResetCommandBuffer");

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  throw_if_failed(vkBeginCommandBuffer(command_buffer, &begin_info), "vkBeginCommandBuffer");

  const auto old_layout = swapchain_image_layouts_[image_index];
  transition_image_layout(command_buffer,
    swapchain_.images()[image_index],
    old_layout,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    0,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    old_layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT);

  const VkClearColorValue clear_color{{0.04F, 0.06F, 0.10F, 1.0F}};
  VkImageSubresourceRange clear_range{};
  clear_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  clear_range.baseMipLevel = 0;
  clear_range.levelCount = 1;
  clear_range.baseArrayLayer = 0;
  clear_range.layerCount = 1;

  vkCmdClearColorImage(command_buffer,
    swapchain_.images()[image_index],
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    &clear_color,
    1,
    &clear_range);

  transition_image_layout(command_buffer,
    swapchain_.images()[image_index],
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    0,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

  throw_if_failed(vkEndCommandBuffer(command_buffer), "vkEndCommandBuffer");

  const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_available;
  submit_info.pWaitDstStageMask = &wait_stage;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &render_finished;

  throw_if_failed(vkQueueSubmit(device_.graphics_queue(), 1, &submit_info, in_flight_fence), "vkQueueSubmit");

  const auto swapchain = swapchain_.swapchain();
  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &render_finished;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain;
  present_info.pImageIndices = &image_index;

  const VkResult present_result = vkQueuePresentKHR(device_.present_queue(), &present_info);
  if(present_result != VK_SUCCESS && present_result != VK_SUBOPTIMAL_KHR)
  {
    throw_if_failed(present_result, "vkQueuePresentKHR");
  }

  swapchain_image_layouts_[image_index] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  frames_.advance_frame();
}

void VulkanRenderer::resize(int width, int height)
{
  if(width <= 0 || height <= 0)
  {
    return;
  }

  pending_extent_ = SwapchainExtent{
    .width = static_cast<std::uint32_t>(width),
    .height = static_cast<std::uint32_t>(height),
  };
  swapchain_recreate_requested_ = true;
}

void VulkanRenderer::wait_idle()
{
  device_.wait_idle();
}

VulkanRenderer::~VulkanRenderer() {}

void VulkanRenderer::recreate_swapchain_if_needed()
{
  if(!swapchain_recreate_requested_)
  {
    return;
  }

  recreate_swapchain(pending_extent_);
  swapchain_recreate_requested_ = false;
}

void VulkanRenderer::recreate_swapchain(SwapchainExtent extent)
{
  device_.wait_idle();

  // Later: pass the old swapchain handle into recreation instead of blunt replacement.
  swapchain_.recreate(context_, device_, extent);
  swapchain_image_layouts_.assign(swapchain_.images().size(), VK_IMAGE_LAYOUT_UNDEFINED);
}

}// namespace vulkan_rt::render::vulkan
