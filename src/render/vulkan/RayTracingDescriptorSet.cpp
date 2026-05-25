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
  const VulkanDevice &device,
  const AccelerationStructure &tlas,
  const VulkanImage &output_image,
  const VulkanBuffer &material_indices,
  const VulkanBuffer &materials,
  const VulkanBuffer &camera)
{
  try
  {
    create(device, tlas, output_image, material_indices, materials, camera);
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

/**
 * Shader binding 0 = TLAS
 * Shader binding 1 = output image
 * Shader binding 2 = triangle -> material index buffer
 * Shader binding 3 = material data buffer.
 * Shader binding 4 = camera data uniform buffer.
 * 
 * \param device
 * \param tlas
 * \param output_image
 * \param material_indices
 * \param materials
 */
void RayTracingDescriptorSet::create(
  const VulkanDevice &device,
  const AccelerationStructure &tlas,
  const VulkanImage &output_image,
  const VulkanBuffer &material_indices,
  const VulkanBuffer &materials,
  const VulkanBuffer &camera)
{
  device_ = device.device();

  std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

  bindings[3].binding = 3;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  bindings[3].descriptorCount = 1;
  bindings[3].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

  bindings[4].binding = 4;
  bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[4].descriptorCount = 1;
  bindings[4].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
  layout_info.pBindings = bindings.data();
  throw_if_failed(vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &layout_), "vkCreateDescriptorSetLayout");

  // A descriptor pool is where descriptor sets are allocated from. Here we say the pool must have enough descriptors fo
  std::array<VkDescriptorPoolSize, 4> pool_sizes{};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  pool_sizes[0].descriptorCount = 1;
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  pool_sizes[1].descriptorCount = 1;
  pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_sizes[2].descriptorCount = 2;
  pool_sizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[3].descriptorCount = 1;

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

  // Write Binding 0: TLAS
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

  // Write Binding 1: Output Image
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

  // Write Binding 2: Material Indices
  VkDescriptorBufferInfo material_index_info{};
  material_index_info.buffer = material_indices.buffer();
  material_index_info.offset = 0;
  material_index_info.range = material_indices.size();

  VkWriteDescriptorSet material_index_write{};
  material_index_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  material_index_write.dstSet = descriptor_set_;
  material_index_write.dstBinding = 2;
  material_index_write.descriptorCount = 1;
  material_index_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  material_index_write.pBufferInfo = &material_index_info;

  // Write Binding 3: Materials
  VkDescriptorBufferInfo material_info{};
  material_info.buffer = materials.buffer();
  material_info.offset = 0;
  material_info.range = materials.size();

  VkWriteDescriptorSet material_write{};
  material_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  material_write.dstSet = descriptor_set_;
  material_write.dstBinding = 3;
  material_write.descriptorCount = 1;
  material_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  material_write.pBufferInfo = &material_info;

  // Write Binding 4: Camera
  VkDescriptorBufferInfo camera_info{};
  camera_info.buffer = camera.buffer();
  camera_info.offset = 0;
  camera_info.range = camera.size();

  VkWriteDescriptorSet camera_write{};
  camera_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  camera_write.dstSet = descriptor_set_;
  camera_write.dstBinding = 4;
  camera_write.descriptorCount = 1;
  camera_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  camera_write.pBufferInfo = &camera_info;

  // Update the Descriptor Set
  std::array<VkWriteDescriptorSet, 5> writes{
      tlas_write, 
      output_write, 
      material_index_write, 
      material_write,
      camera_write};
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
