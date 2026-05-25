#include "ShaderBindingTable.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

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

VkDeviceSize aligned_size(VkDeviceSize value, VkDeviceSize alignment)
{
  if(alignment == 0)
  {
    return value;
  }

  return ((value + alignment - 1) / alignment) * alignment;
}
}

ShaderBindingTable::ShaderBindingTable(
  const VulkanDevice &device, const VulkanAllocator &allocator, const RayTracingPipeline &pipeline)
{
  create(device, allocator, pipeline);
}

ShaderBindingTable::~ShaderBindingTable() = default;

ShaderBindingTable::ShaderBindingTable(ShaderBindingTable &&other) noexcept = default;

ShaderBindingTable &ShaderBindingTable::operator=(ShaderBindingTable &&other) noexcept = default;

const VkStridedDeviceAddressRegionKHR &ShaderBindingTable::raygen_region() const
{
  return raygen_region_;
}

const VkStridedDeviceAddressRegionKHR &ShaderBindingTable::miss_region() const
{
  return miss_region_;
}

const VkStridedDeviceAddressRegionKHR &ShaderBindingTable::hit_region() const
{
  return hit_region_;
}

const VkStridedDeviceAddressRegionKHR &ShaderBindingTable::callable_region() const
{
  return callable_region_;
}

VkBuffer ShaderBindingTable::buffer() const
{
  return buffer_ ? buffer_->buffer() : VK_NULL_HANDLE;
}

VkDeviceSize ShaderBindingTable::size() const
{
  return size_;
}

void ShaderBindingTable::create(
  const VulkanDevice &device, const VulkanAllocator &allocator, const RayTracingPipeline &pipeline)
{
  constexpr std::uint32_t expected_group_count = 3;
  if(pipeline.shader_group_count() != expected_group_count)
  {
    throw std::runtime_error("ShaderBindingTable expects exactly three shader groups.");
  }

  const auto &properties = device.ray_tracing_pipeline_properties();
  const VkDeviceSize handle_size = properties.shaderGroupHandleSize;
  const VkDeviceSize handle_alignment = properties.shaderGroupHandleAlignment;
  const VkDeviceSize base_alignment = properties.shaderGroupBaseAlignment;
  const VkDeviceSize record_stride = aligned_size(handle_size, handle_alignment);

  const VkDeviceSize raygen_offset = 0;
  const VkDeviceSize miss_offset = aligned_size(raygen_offset + record_stride, base_alignment);
  const VkDeviceSize hit_offset = aligned_size(miss_offset + record_stride, base_alignment);
  size_ = hit_offset + record_stride;

  std::vector<std::uint8_t> shader_handles(static_cast<std::size_t>(expected_group_count * handle_size));
  throw_if_failed(
    vkGetRayTracingShaderGroupHandlesKHR(device.device(),
      pipeline.pipeline(),
      0,
      expected_group_count,
      shader_handles.size(),
      shader_handles.data()),
    "vkGetRayTracingShaderGroupHandlesKHR");

  buffer_ = std::make_unique<VulkanBuffer>(allocator,
    BufferCreateInfo{
      .size = size_,
      .usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    });

  auto *mapped = static_cast<std::uint8_t *>(buffer_->map());
  std::fill(mapped, mapped + static_cast<std::size_t>(size_), std::uint8_t{0});

  auto copy_group = [&](std::uint32_t group_index, VkDeviceSize destination_offset) {
    const auto source_offset = static_cast<std::size_t>(group_index * handle_size);
    std::memcpy(mapped + static_cast<std::size_t>(destination_offset),
      shader_handles.data() + source_offset,
      static_cast<std::size_t>(handle_size));
  };

  copy_group(0, raygen_offset);
  copy_group(1, miss_offset);
  copy_group(2, hit_offset);

  buffer_->flush();
  buffer_->unmap();

  const VkDeviceAddress base_address = buffer_->device_address(device);
  raygen_region_ = VkStridedDeviceAddressRegionKHR{
    .deviceAddress = base_address + raygen_offset,
    .stride = record_stride,
    .size = record_stride,
  };
  miss_region_ = VkStridedDeviceAddressRegionKHR{
    .deviceAddress = base_address + miss_offset,
    .stride = record_stride,
    .size = record_stride,
  };
  hit_region_ = VkStridedDeviceAddressRegionKHR{
    .deviceAddress = base_address + hit_offset,
    .stride = record_stride,
    .size = record_stride,
  };
  callable_region_ = VkStridedDeviceAddressRegionKHR{};
}
}
