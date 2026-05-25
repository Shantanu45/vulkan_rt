#include "VulkanBuffer.hpp"

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

VulkanBuffer::VulkanBuffer(const VulkanAllocator &allocator, const BufferCreateInfo &create_info)
{
  create(allocator, create_info);
}

VulkanBuffer::~VulkanBuffer()
{
  destroy();
}

VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
  : allocator_(other.allocator_)
  , buffer_(other.buffer_)
  , allocation_(other.allocation_)
  , size_(other.size_)
{
  other.allocator_ = VK_NULL_HANDLE;
  other.buffer_ = VK_NULL_HANDLE;
  other.allocation_ = VK_NULL_HANDLE;
  other.size_ = 0;
}

VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept
{
  if(this != &other)
  {
    destroy();
    allocator_ = other.allocator_;
    buffer_ = other.buffer_;
    allocation_ = other.allocation_;
    size_ = other.size_;
    other.allocator_ = VK_NULL_HANDLE;
    other.buffer_ = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.size_ = 0;
  }
  return *this;
}

VkBuffer VulkanBuffer::buffer() const
{
  return buffer_;
}

VmaAllocation VulkanBuffer::allocation() const
{
  return allocation_;
}

VkDeviceSize VulkanBuffer::size() const
{
  return size_;
}

void *VulkanBuffer::map()
{
  void *data = nullptr;
  throw_if_failed(vmaMapMemory(allocator_, allocation_, &data), "vmaMapMemory");
  return data;
}

void VulkanBuffer::unmap()
{
  vmaUnmapMemory(allocator_, allocation_);
}

void VulkanBuffer::create(const VulkanAllocator &allocator, const BufferCreateInfo &create_info)
{
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = create_info.size;
  buffer_info.usage = create_info.usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = create_info.memory_usage;
  alloc_info.flags = create_info.alloc_flags;

  throw_if_failed(
    vmaCreateBuffer(allocator.allocator(), &buffer_info, &alloc_info, &buffer_, &allocation_, nullptr),
    "vmaCreateBuffer");

  allocator_ = allocator.allocator();
  size_ = create_info.size;
}

void VulkanBuffer::destroy()
{
  if(buffer_ != VK_NULL_HANDLE)
  {
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
    buffer_ = VK_NULL_HANDLE;
    allocation_ = VK_NULL_HANDLE;
  }
  allocator_ = VK_NULL_HANDLE;
  size_ = 0;
}
}
