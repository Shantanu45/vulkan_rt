#include "AccelerationStructure.hpp"

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

AccelerationStructure::AccelerationStructure(
  const VulkanDevice &device,
  const VulkanAllocator &allocator,
  const AccelerationStructureCreateInfo &create_info)
{
  try
  {
    create(device, allocator, create_info);
  }
  catch(...)
  {
    destroy();
    throw;
  }
}

AccelerationStructure::~AccelerationStructure()
{
  destroy();
}

AccelerationStructure::AccelerationStructure(AccelerationStructure &&other) noexcept
  : device_(other.device_)
  , acceleration_structure_(other.acceleration_structure_)
  , device_address_(other.device_address_)
  , size_(other.size_)
  , type_(other.type_)
  , buffer_(std::move(other.buffer_))
{
  other.device_ = VK_NULL_HANDLE;
  other.acceleration_structure_ = VK_NULL_HANDLE;
  other.device_address_ = 0;
  other.size_ = 0;
  other.type_ = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
}

AccelerationStructure &AccelerationStructure::operator=(AccelerationStructure &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    device_ = other.device_;
    acceleration_structure_ = other.acceleration_structure_;
    device_address_ = other.device_address_;
    size_ = other.size_;
    type_ = other.type_;
    buffer_ = std::move(other.buffer_);

    other.device_ = VK_NULL_HANDLE;
    other.acceleration_structure_ = VK_NULL_HANDLE;
    other.device_address_ = 0;
    other.size_ = 0;
    other.type_ = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  }

  return *this;
}

VkAccelerationStructureKHR AccelerationStructure::acceleration_structure() const
{
  return acceleration_structure_;
}

VkDeviceAddress AccelerationStructure::device_address() const
{
  return device_address_;
}

VkDeviceSize AccelerationStructure::size() const
{
  return size_;
}

VkAccelerationStructureTypeKHR AccelerationStructure::type() const
{
  return type_;
}

VkBuffer AccelerationStructure::backing_buffer() const
{
  return buffer_ ? buffer_->buffer() : VK_NULL_HANDLE;
}

void AccelerationStructure::create(
  const VulkanDevice &device,
  const VulkanAllocator &allocator,
  const AccelerationStructureCreateInfo &create_info)
{
  if(create_info.size == 0)
  {
    throw std::runtime_error("Cannot create zero-sized Vulkan acceleration structure.");
  }

  device_ = device.device();
  size_ = create_info.size;
  type_ = create_info.type;

  buffer_ = std::make_unique<VulkanBuffer>(allocator,
    BufferCreateInfo{
      .size = create_info.size,
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = 0,
    });

  VkAccelerationStructureCreateInfoKHR acceleration_structure_info{};
  acceleration_structure_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  acceleration_structure_info.buffer = buffer_->buffer();
  acceleration_structure_info.offset = 0;
  acceleration_structure_info.size = create_info.size;
  acceleration_structure_info.type = create_info.type;

  throw_if_failed(
    vkCreateAccelerationStructureKHR(
      device.device(), &acceleration_structure_info, nullptr, &acceleration_structure_),
    "vkCreateAccelerationStructureKHR");

  VkAccelerationStructureDeviceAddressInfoKHR address_info{};
  address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  address_info.accelerationStructure = acceleration_structure_;
  device_address_ = vkGetAccelerationStructureDeviceAddressKHR(device.device(), &address_info);
}

void AccelerationStructure::destroy()
{
  if(acceleration_structure_ != VK_NULL_HANDLE)
  {
    vkDestroyAccelerationStructureKHR(device_, acceleration_structure_, nullptr);
    acceleration_structure_ = VK_NULL_HANDLE;
  }

  buffer_.reset();
  device_ = VK_NULL_HANDLE;
  device_address_ = 0;
  size_ = 0;
  type_ = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
}
}
