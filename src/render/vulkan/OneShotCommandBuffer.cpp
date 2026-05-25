#include "OneShotCommandBuffer.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace vulkan_rt::render::vulkan
{
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
}

OneShotCommandBuffer::OneShotCommandBuffer(const VulkanDevice &device)
  : device_(device.device())
  , graphics_queue_(device.graphics_queue())
{
  VkCommandPoolCreateInfo command_pool_info{};
  command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  command_pool_info.queueFamilyIndex = device.queue_families().graphics.value();
  throw_if_failed(vkCreateCommandPool(device_, &command_pool_info, nullptr, &command_pool_), "vkCreateCommandPool");

  VkCommandBufferAllocateInfo command_buffer_info{};
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.commandPool = command_pool_;
  command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_info.commandBufferCount = 1;
  throw_if_failed(vkAllocateCommandBuffers(device_, &command_buffer_info, &command_buffer_), "vkAllocateCommandBuffers");
}

OneShotCommandBuffer::~OneShotCommandBuffer()
{
  if(command_pool_ != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(device_, command_pool_, nullptr);
  }
}

VkCommandBuffer OneShotCommandBuffer::command_buffer() const
{
  return command_buffer_;
}

void OneShotCommandBuffer::begin() const
{
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  throw_if_failed(vkBeginCommandBuffer(command_buffer_, &begin_info), "vkBeginCommandBuffer");
}

void OneShotCommandBuffer::end_submit_and_wait() const
{
  throw_if_failed(vkEndCommandBuffer(command_buffer_), "vkEndCommandBuffer");

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  VkFence fence = VK_NULL_HANDLE;
  throw_if_failed(vkCreateFence(device_, &fence_info, nullptr, &fence), "vkCreateFence");

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;

  const VkResult submit_result = vkQueueSubmit(graphics_queue_, 1, &submit_info, fence);
  if(submit_result != VK_SUCCESS)
  {
    vkDestroyFence(device_, fence, nullptr);
    throw_if_failed(submit_result, "vkQueueSubmit");
  }

  const VkResult wait_result =
    vkWaitForFences(device_, 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
  vkDestroyFence(device_, fence, nullptr);
  throw_if_failed(wait_result, "vkWaitForFences");
}
}
