#include "VulkanFrameResources.hpp"

#include <stdexcept>
#include <string>
#include <utility>

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

VulkanFrameResources::VulkanFrameResources(const VulkanDevice &device, std::uint32_t frames_in_flight)
{
  create(device, frames_in_flight);
}

VulkanFrameResources::~VulkanFrameResources()
{
  destroy();
}

VulkanFrameResources::VulkanFrameResources(VulkanFrameResources &&other) noexcept
  : device_(other.device_)
  , command_pool_(other.command_pool_)
  , command_buffers_(std::move(other.command_buffers_))
  , image_available_semaphores_(std::move(other.image_available_semaphores_))
  , render_finished_semaphores_(std::move(other.render_finished_semaphores_))
  , in_flight_fences_(std::move(other.in_flight_fences_))
  , current_frame_index_(other.current_frame_index_)
{
  other.device_ = VK_NULL_HANDLE;
  other.command_pool_ = VK_NULL_HANDLE;
  other.current_frame_index_ = 0;
}

VulkanFrameResources &VulkanFrameResources::operator=(VulkanFrameResources &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    device_ = other.device_;
    command_pool_ = other.command_pool_;
    command_buffers_ = std::move(other.command_buffers_);
    image_available_semaphores_ = std::move(other.image_available_semaphores_);
    render_finished_semaphores_ = std::move(other.render_finished_semaphores_);
    in_flight_fences_ = std::move(other.in_flight_fences_);
    current_frame_index_ = other.current_frame_index_;

    other.device_ = VK_NULL_HANDLE;
    other.command_pool_ = VK_NULL_HANDLE;
    other.current_frame_index_ = 0;
  }

  return *this;
}

std::uint32_t VulkanFrameResources::frames_in_flight() const
{
  return static_cast<std::uint32_t>(command_buffers_.size());
}

std::uint32_t VulkanFrameResources::current_frame_index() const
{
  return current_frame_index_;
}

VkCommandPool VulkanFrameResources::command_pool() const
{
  return command_pool_;
}

std::span<const VkCommandBuffer> VulkanFrameResources::command_buffers() const
{
  return std::span<const VkCommandBuffer>{command_buffers_};
}

std::span<const VkSemaphore> VulkanFrameResources::image_available_semaphores() const
{
  return std::span<const VkSemaphore>{image_available_semaphores_};
}

std::span<const VkSemaphore> VulkanFrameResources::render_finished_semaphores() const
{
  return std::span<const VkSemaphore>{render_finished_semaphores_};
}

std::span<const VkFence> VulkanFrameResources::in_flight_fences() const
{
  return std::span<const VkFence>{in_flight_fences_};
}

void VulkanFrameResources::advance_frame()
{
  if(frames_in_flight() > 0)
  {
    current_frame_index_ = (current_frame_index_ + 1) % frames_in_flight();
  }
}

void VulkanFrameResources::create(const VulkanDevice &device, std::uint32_t frames_in_flight)
{
  if(frames_in_flight == 0)
  {
    throw std::invalid_argument("VulkanFrameResources requires at least one frame in flight.");
  }

  device_ = device.device();

  VkCommandPoolCreateInfo command_pool_info{};
  command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_info.queueFamilyIndex = device.queue_families().graphics.value();

  throw_if_failed(vkCreateCommandPool(device_, &command_pool_info, nullptr, &command_pool_), "vkCreateCommandPool");

  command_buffers_.resize(frames_in_flight);

  VkCommandBufferAllocateInfo command_buffer_info{};
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.commandPool = command_pool_;
  command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_info.commandBufferCount = frames_in_flight;

  throw_if_failed(
    vkAllocateCommandBuffers(device_, &command_buffer_info, command_buffers_.data()), "vkAllocateCommandBuffers");

  image_available_semaphores_.resize(frames_in_flight);
  render_finished_semaphores_.resize(frames_in_flight);
  in_flight_fences_.resize(frames_in_flight);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for(std::uint32_t i = 0; i < frames_in_flight; ++i)
  {
    throw_if_failed(
      vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[i]), "vkCreateSemaphore");
    throw_if_failed(
      vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[i]), "vkCreateSemaphore");
    throw_if_failed(vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]), "vkCreateFence");
  }
}

void VulkanFrameResources::destroy()
{
  if(device_ == VK_NULL_HANDLE)
  {
    return;
  }

  for(const auto fence : in_flight_fences_)
  {
    vkDestroyFence(device_, fence, nullptr);
  }
  in_flight_fences_.clear();

  for(const auto semaphore : render_finished_semaphores_)
  {
    vkDestroySemaphore(device_, semaphore, nullptr);
  }
  render_finished_semaphores_.clear();

  for(const auto semaphore : image_available_semaphores_)
  {
    vkDestroySemaphore(device_, semaphore, nullptr);
  }
  image_available_semaphores_.clear();

  command_buffers_.clear();

  if(command_pool_ != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(device_, command_pool_, nullptr);
    command_pool_ = VK_NULL_HANDLE;
  }

  current_frame_index_ = 0;
  device_ = VK_NULL_HANDLE;
}
}
