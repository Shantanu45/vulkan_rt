#include "RayTracingDescriptorSet.hpp"

#include <array>
#include <cstdint>
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

RayTracingDescriptorSet::RayTracingDescriptorSet(
  const VulkanDevice &device, const AccelerationStructure &tlas, const VulkanImage &output_image)
{
  try
  {
    create(device, tlas, output_image);
  }
  catch(...)
  {
    destroy();
    throw;
  }
}

RayTracingDescriptorSet::~RayTracingDescriptorSet()
{
  destroy();
}

RayTracingDescriptorSet::RayTracingDescriptorSet(RayTracingDescriptorSet &&other) noexcept
  : device_(other.device_)
  , layout_(other.layout_)
  , pool_(other.pool_)
  , descriptor_set_(other.descriptor_set_)
{
  other.device_ = VK_NULL_HANDLE;
  other.layout_ = VK_NULL_HANDLE;
  other.pool_ = VK_NULL_HANDLE;
  other.descriptor_set_ = VK_NULL_HANDLE;
}

RayTracingDescriptorSet &RayTracingDescriptorSet::operator=(RayTracingDescriptorSet &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    device_ = other.device_;
    layout_ = other.layout_;
    pool_ = other.pool_;
    descriptor_set_ = other.descriptor_set_;

    other.device_ = VK_NULL_HANDLE;
    other.layout_ = VK_NULL_HANDLE;
    other.pool_ = VK_NULL_HANDLE;
    other.descriptor_set_ = VK_NULL_HANDLE;
  }

  return *this;
}

VkDescriptorSetLayout RayTracingDescriptorSet::layout() const
{
  return layout_;
}

VkDescriptorPool RayTracingDescriptorSet::pool() const
{
  return pool_;
}

VkDescriptorSet RayTracingDescriptorSet::descriptor_set() const
{
  return descriptor_set_;
}

void RayTracingDescriptorSet::create(
  const VulkanDevice &device, const AccelerationStructure &tlas, const VulkanImage &output_image)
{
  device_ = device.device();

  std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
  layout_info.pBindings = bindings.data();
  throw_if_failed(vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &layout_), "vkCreateDescriptorSetLayout");

  std::array<VkDescriptorPoolSize, 2> pool_sizes{};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  pool_sizes[0].descriptorCount = 1;
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  pool_sizes[1].descriptorCount = 1;

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.maxSets = 1;
  pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();
  throw_if_failed(vkCreateDescriptorPool(device_, &pool_info, nullptr, &pool_), "vkCreateDescriptorPool");

  VkDescriptorSetAllocateInfo allocate_info{};
  allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocate_info.descriptorPool = pool_;
  allocate_info.descriptorSetCount = 1;
  allocate_info.pSetLayouts = &layout_;
  throw_if_failed(vkAllocateDescriptorSets(device_, &allocate_info, &descriptor_set_), "vkAllocateDescriptorSets");

  VkWriteDescriptorSetAccelerationStructureKHR acceleration_structure_write{};
  acceleration_structure_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  acceleration_structure_write.accelerationStructureCount = 1;
  const auto acceleration_structure = tlas.acceleration_structure();
  acceleration_structure_write.pAccelerationStructures = &acceleration_structure;

  VkWriteDescriptorSet tlas_write{};
  tlas_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  tlas_write.pNext = &acceleration_structure_write;
  tlas_write.dstSet = descriptor_set_;
  tlas_write.dstBinding = 0;
  tlas_write.descriptorCount = 1;
  tlas_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

  VkDescriptorImageInfo output_image_info{};
  output_image_info.imageView = output_image.image_view();
  output_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkWriteDescriptorSet output_write{};
  output_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  output_write.dstSet = descriptor_set_;
  output_write.dstBinding = 1;
  output_write.descriptorCount = 1;
  output_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  output_write.pImageInfo = &output_image_info;

  std::array<VkWriteDescriptorSet, 2> writes{tlas_write, output_write};
  vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void RayTracingDescriptorSet::destroy()
{
  if(pool_ != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device_, pool_, nullptr);
    pool_ = VK_NULL_HANDLE;
    descriptor_set_ = VK_NULL_HANDLE;
  }

  if(layout_ != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device_, layout_, nullptr);
    layout_ = VK_NULL_HANDLE;
  }

  device_ = VK_NULL_HANDLE;
}
}
