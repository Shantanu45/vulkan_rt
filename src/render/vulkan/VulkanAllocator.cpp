#define VMA_IMPLEMENTATION
#include "VulkanAllocator.hpp"

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

VulkanAllocator::VulkanAllocator(const VulkanContext &context, const VulkanDevice &device)
{
  create(context, device);
}

VulkanAllocator::~VulkanAllocator()
{
  destroy();
}

VulkanAllocator::VulkanAllocator(VulkanAllocator &&other) noexcept
  : allocator_(other.allocator_)
{
  other.allocator_ = VK_NULL_HANDLE;
}

VulkanAllocator &VulkanAllocator::operator=(VulkanAllocator &&other) noexcept
{
  if(this != &other)
  {
    destroy();
    allocator_ = other.allocator_;
    other.allocator_ = VK_NULL_HANDLE;
  }
  return *this;
}

VmaAllocator VulkanAllocator::allocator() const
{
  return allocator_;
}

void VulkanAllocator::create(const VulkanContext &context, const VulkanDevice &device)
{
  VmaVulkanFunctions vma_functions{};
  vma_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  vma_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo create_info{};
  create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  create_info.vulkanApiVersion = VK_API_VERSION_1_3;
  create_info.physicalDevice = device.physical_device();
  create_info.device = device.device();
  create_info.instance = context.instance();
  create_info.pVulkanFunctions = &vma_functions;

  throw_if_failed(vmaCreateAllocator(&create_info, &allocator_), "vmaCreateAllocator");
}

void VulkanAllocator::destroy()
{
  if(allocator_ != VK_NULL_HANDLE)
  {
    vmaDestroyAllocator(allocator_);
    allocator_ = VK_NULL_HANDLE;
  }
}
}
