#include "RayTracingScene.hpp"

#include "OneShotCommandBuffer.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace vulkan_rt::render::vulkan
{
namespace
{
struct GpuMaterial
{
  float albedo[4]{};
  float emission[4]{};
};
}

RayTracingScene::RayTracingScene(const VulkanDevice &device, const VulkanAllocator &allocator, const scene::Scene &scene)
{
  build_scene(device, allocator, scene);
}

RayTracingScene::~RayTracingScene() = default;

const AccelerationStructure &RayTracingScene::tlas() const
{
  if(!tlas_)
  {
    throw std::runtime_error("Ray tracing scene has no top-level acceleration structure.");
  }

  return *tlas_;
}

const VulkanBuffer &RayTracingScene::material_index_buffer() const
{
  if(!material_index_buffer_)
  {
    throw std::runtime_error("Ray tracing scene has no material index buffer.");
  }

  return *material_index_buffer_;
}

const VulkanBuffer &RayTracingScene::material_buffer() const
{
  if(!material_buffer_)
  {
    throw std::runtime_error("Ray tracing scene has no material buffer.");
  }

  return *material_buffer_;
}

void RayTracingScene::build_scene(const VulkanDevice &device, const VulkanAllocator &allocator, const scene::Scene &scene)
{
  if(scene.empty())
  {
    throw std::runtime_error("Cannot build a ray tracing scene from an empty scene.");
  }

  const auto scene_triangles = scene.triangles();
  const auto scene_materials = scene.materials();
  triangle_count_ = static_cast<std::uint32_t>(scene_triangles.size());

  std::vector<float> vertices;
  std::vector<std::uint32_t> material_indices;
  vertices.reserve(scene_triangles.size() * 9);
  material_indices.reserve(scene_triangles.size());
  const auto append_vertex = [&vertices](scene::Vec3 vertex) {
    vertices.push_back(vertex.x);
    vertices.push_back(vertex.y);
    vertices.push_back(vertex.z);
  };
  for(const auto &triangle : scene_triangles)
  {
    append_vertex(triangle.v0);
    append_vertex(triangle.v1);
    append_vertex(triangle.v2);
    material_indices.push_back(triangle.material_index);
  }

  std::vector<GpuMaterial> materials;
  materials.reserve(scene_materials.size());
  for(const auto &material : scene_materials)
  {
    materials.push_back(GpuMaterial{
      .albedo = {material.albedo.x, material.albedo.y, material.albedo.z, material.roughness},
      .emission = {material.emission.x, material.emission.y, material.emission.z, 0.0F},
    });
  }

  vertex_buffer_ = std::make_unique<VulkanBuffer>(allocator,
    BufferCreateInfo{
      .size = static_cast<VkDeviceSize>(vertices.size() * sizeof(float)),
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    });

  auto *vertex_data = static_cast<float *>(vertex_buffer_->map());
  std::copy(vertices.begin(), vertices.end(), vertex_data);
  vertex_buffer_->flush();
  vertex_buffer_->unmap();

  material_index_buffer_ = std::make_unique<VulkanBuffer>(allocator,
    BufferCreateInfo{
      .size = static_cast<VkDeviceSize>(material_indices.size() * sizeof(std::uint32_t)),
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    });

  auto *material_index_data = static_cast<std::uint32_t *>(material_index_buffer_->map());
  std::copy(material_indices.begin(), material_indices.end(), material_index_data);
  material_index_buffer_->flush();
  material_index_buffer_->unmap();

  material_buffer_ = std::make_unique<VulkanBuffer>(allocator,
    BufferCreateInfo{
      .size = static_cast<VkDeviceSize>(materials.size() * sizeof(GpuMaterial)),
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    });

  auto *material_data = static_cast<GpuMaterial *>(material_buffer_->map());
  std::copy(materials.begin(), materials.end(), material_data);
  material_buffer_->flush();
  material_buffer_->unmap();

  VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
  triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
  triangles.vertexData.deviceAddress = vertex_buffer_->device_address(device);
  triangles.vertexStride = 3 * sizeof(float);
  triangles.maxVertex = triangle_count_ * 3 - 1;
  triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

  VkAccelerationStructureGeometryKHR blas_geometry{};
  blas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  blas_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  blas_geometry.geometry.triangles = triangles;
  blas_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

  VkAccelerationStructureBuildGeometryInfoKHR blas_build_info{};
  blas_build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  blas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  blas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  blas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  blas_build_info.geometryCount = 1;
  blas_build_info.pGeometries = &blas_geometry;

  VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes{};
  blas_build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  vkGetAccelerationStructureBuildSizesKHR(device.device(),
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &blas_build_info,
    &triangle_count_,
    &blas_build_sizes);

  blas_ = std::make_unique<AccelerationStructure>(device,
    allocator,
    AccelerationStructureCreateInfo{
      .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      .size = blas_build_sizes.accelerationStructureSize,
    });

  VulkanBuffer blas_scratch{
    allocator,
    BufferCreateInfo{
      .size = blas_build_sizes.buildScratchSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = 0,
    }};

  blas_build_info.dstAccelerationStructure = blas_->acceleration_structure();
  blas_build_info.scratchData.deviceAddress = blas_scratch.device_address(device);

  VkAccelerationStructureBuildRangeInfoKHR blas_build_range{};
  blas_build_range.primitiveCount = triangle_count_;
  const VkAccelerationStructureBuildRangeInfoKHR *blas_build_ranges[] = {&blas_build_range};

  {
    OneShotCommandBuffer command_buffer{device};
    command_buffer.begin();
    vkCmdBuildAccelerationStructuresKHR(command_buffer.command_buffer(), 1, &blas_build_info, blas_build_ranges);
    command_buffer.end_submit_and_wait();
  }

  VkAccelerationStructureInstanceKHR instance{};
  instance.transform.matrix[0][0] = 1.0F;
  instance.transform.matrix[1][1] = 1.0F;
  instance.transform.matrix[2][2] = 1.0F;
  instance.instanceCustomIndex = 0;
  instance.mask = 0xFF;
  instance.instanceShaderBindingTableRecordOffset = 0;
  instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
  instance.accelerationStructureReference = blas_->device_address();

  instance_buffer_ = std::make_unique<VulkanBuffer>(allocator,
    BufferCreateInfo{
      .size = sizeof(VkAccelerationStructureInstanceKHR),
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    });

  auto *instance_data = static_cast<VkAccelerationStructureInstanceKHR *>(instance_buffer_->map());
  *instance_data = instance;
  instance_buffer_->flush();
  instance_buffer_->unmap();

  VkAccelerationStructureGeometryInstancesDataKHR instances_data{};
  instances_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  instances_data.arrayOfPointers = VK_FALSE;
  instances_data.data.deviceAddress = instance_buffer_->device_address(device);

  VkAccelerationStructureGeometryKHR tlas_geometry{};
  tlas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  tlas_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  tlas_geometry.geometry.instances = instances_data;

  VkAccelerationStructureBuildGeometryInfoKHR tlas_build_info{};
  tlas_build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  tlas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  tlas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  tlas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  tlas_build_info.geometryCount = 1;
  tlas_build_info.pGeometries = &tlas_geometry;

  constexpr std::uint32_t instance_count = 1;
  VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes{};
  tlas_build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  vkGetAccelerationStructureBuildSizesKHR(device.device(),
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &tlas_build_info,
    &instance_count,
    &tlas_build_sizes);

  tlas_ = std::make_unique<AccelerationStructure>(device,
    allocator,
    AccelerationStructureCreateInfo{
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .size = tlas_build_sizes.accelerationStructureSize,
    });

  VulkanBuffer tlas_scratch{
    allocator,
    BufferCreateInfo{
      .size = tlas_build_sizes.buildScratchSize,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = 0,
    }};

  tlas_build_info.dstAccelerationStructure = tlas_->acceleration_structure();
  tlas_build_info.scratchData.deviceAddress = tlas_scratch.device_address(device);

  VkAccelerationStructureBuildRangeInfoKHR tlas_build_range{};
  tlas_build_range.primitiveCount = instance_count;
  const VkAccelerationStructureBuildRangeInfoKHR *tlas_build_ranges[] = {&tlas_build_range};

  OneShotCommandBuffer command_buffer{device};
  command_buffer.begin();
  vkCmdBuildAccelerationStructuresKHR(command_buffer.command_buffer(), 1, &tlas_build_info, tlas_build_ranges);
  command_buffer.end_submit_and_wait();
}
}
