#include "RayTracingScene.hpp"

#include "OneShotCommandBuffer.hpp"

#include <algorithm>
#include <cmath>
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

struct GpuLightTriangle
{
  float v0[4]{};
  float v1[4]{};
  float v2[4]{};
  float emission[4]{};
  float normal_area[4]{};
};

scene::Vec3 subtract(scene::Vec3 lhs, scene::Vec3 rhs)
{
  return scene::Vec3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

scene::Vec3 cross(scene::Vec3 lhs, scene::Vec3 rhs)
{
  return scene::Vec3{
    lhs.y * rhs.z - lhs.z * rhs.y,
    lhs.z * rhs.x - lhs.x * rhs.z,
    lhs.x * rhs.y - lhs.y * rhs.x,
  };
}

scene::Vec3 scale(scene::Vec3 value, float scalar)
{
  return scene::Vec3{value.x * scalar, value.y * scalar, value.z * scalar};
}

float length(scene::Vec3 value)
{
  return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

void write_vec3(float (&target)[4], scene::Vec3 value, float w)
{
  target[0] = value.x;
  target[1] = value.y;
  target[2] = value.z;
  target[3] = w;
}

bool is_emissive(scene::Material material)
{
  return material.emission.x > 0.0F || material.emission.y > 0.0F || material.emission.z > 0.0F;
}
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

const VulkanBuffer &RayTracingScene::vertex_buffer() const
{
  if(!vertex_buffer_)
  {
    throw std::runtime_error("Ray tracing scene has no vertex buffer.");
  }

  return *vertex_buffer_;
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

const VulkanBuffer &RayTracingScene::light_buffer() const
{
  if(!light_buffer_)
  {
    throw std::runtime_error("Ray tracing scene has no light buffer.");
  }

  return *light_buffer_;
}

std::uint32_t RayTracingScene::light_count() const
{
  return light_count_;
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
  std::vector<GpuLightTriangle> lights;
  vertices.reserve(scene_triangles.size() * 9);     // 3 floats per vertex * num of vertexes
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
    material_indices.push_back(triangle.material_index);        // material index per triangle

    const auto material = scene_materials[triangle.material_index];
    if(is_emissive(material))
    {
      const auto normal = cross(subtract(triangle.v1, triangle.v0), subtract(triangle.v2, triangle.v0));
      const float double_area = length(normal);
      const float area = double_area * 0.5F;
      scene::Vec3 normalized_normal = double_area > 0.0F
                                        ? scene::Vec3{normal.x / double_area, normal.y / double_area, normal.z / double_area}
                                        : scene::Vec3{0.0F, -1.0F, 0.0F};
      if(normalized_normal.y > 0.0F)
      {
        normalized_normal = scale(normalized_normal, -1.0F);
      }
      GpuLightTriangle light{};
      write_vec3(light.v0, triangle.v0, 0.0F);
      write_vec3(light.v1, triangle.v1, 0.0F);
      write_vec3(light.v2, triangle.v2, 0.0F);
      write_vec3(light.emission, material.emission, 0.0F);
      write_vec3(light.normal_area, normalized_normal, area);
      lights.push_back(light);
    }
  }
  if(lights.empty())
  {
    lights.push_back(GpuLightTriangle{});
  }
  light_count_ = static_cast<std::uint32_t>(lights.size());

  std::vector<GpuMaterial> materials;
  materials.reserve(scene_materials.size());
  for(const auto &material : scene_materials)
  {
    materials.push_back(GpuMaterial{
      .albedo = {material.albedo.x, material.albedo.y, material.albedo.z, material.roughness},
      .emission = {material.emission.x, material.emission.y, material.emission.z, 0.0F},
    });
  }

  //no VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : Because we are not binding it as a raster vertex buffer.
  // You can also add VK_BUFFER_USAGE_VERTEX_BUFFER_BIT if the same buffer will later be reused for raster rendering/debug drawing. It would not hurt, but it is unnecessary for BLAS building alone.
  vertex_buffer_ = std::make_unique<VulkanBuffer>(allocator,
    BufferCreateInfo{
      .size = static_cast<VkDeviceSize>(vertices.size() * sizeof(float)),
      .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |       // VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR means: The buffer may be read by vkCmdBuildAccelerationStructuresKHR while building the BLAS. In our case, those reads are the triangle positions.
               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,// VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT means: The buffer can be addressed through a raw GPU device address, which is exactly how the BLAS geometry points at it
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,        // tells VMA "This allocation should be CPU-mappable, and I mostly plan to write it once/sequentially from the CPU."
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

  light_buffer_ = std::make_unique<VulkanBuffer>(allocator,
    BufferCreateInfo{
      .size = static_cast<VkDeviceSize>(lights.size() * sizeof(GpuLightTriangle)),
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
      .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    });

  auto *light_data = static_cast<GpuLightTriangle *>(light_buffer_->map());
  std::copy(lights.begin(), lights.end(), light_data);
  light_buffer_->flush();
  light_buffer_->unmap();

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

  // build a BLAS
  // optimize it for ray traversal speed
  // this is a fresh build
  // it has one geometry block
  // PREFER_FAST_TRACE means the build may take more work or memory, but tracing rays through it should be faster.
  VkAccelerationStructureBuildGeometryInfoKHR blas_build_info{};
  blas_build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  blas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  blas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  blas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  blas_build_info.geometryCount = 1;
  blas_build_info.pGeometries = &blas_geometry;

  // Before creating the BLAS, Vulkan must tell us how much memory it needs.
  // VkAccelerationStructureBuildSizesInfoKHR ->accelerationStructureSize: persistent BLAS storage,
  // VkAccelerationStructureBuildSizesInfoKHR ->buildScratchSize: scratch is temporary GPU working memory used during the build command.
  VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes{};
  blas_build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  vkGetAccelerationStructureBuildSizesKHR(device.device(),
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &blas_build_info,
    &triangle_count_,
    &blas_build_sizes);

  // Allocate the BLAS Object: This creates the actual VkAccelerationStructureKHR plus its backing buffer.
  // This memory stays alive as long as the BLAS is used.
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

  // This says how many primitives from the geometry to build.
  VkAccelerationStructureBuildRangeInfoKHR blas_build_range{};
  blas_build_range.primitiveCount = triangle_count_;
  const VkAccelerationStructureBuildRangeInfoKHR *blas_build_ranges[] = {&blas_build_range};

  {
    // Execute the BLAS Build
    // This records the actual GPU command that builds the BLAS. 
    // OneShotCommandBuffer begins a temporary command buffer, submits it, waits for it to finish, then cleans up. After this point, the BLAS is ready.
    OneShotCommandBuffer command_buffer{device};
    command_buffer.begin();
    vkCmdBuildAccelerationStructuresKHR(command_buffer.command_buffer(), 1, &blas_build_info, blas_build_ranges);
    command_buffer.end_submit_and_wait();
  }

  // Create a TLAS Instance Pointing to the BLAS
  // A TLAS does not directly contain triangles. It contains instances of BLASes.
  // This instance says:
  // use this BLAS place it with identity transform make it visible to rays with 
  // mask 0xFF use hit group 0 disable triangle facing cull
  VkAccelerationStructureInstanceKHR instance{};
  // The identity transform means the BLAS appears in world space exactly as built.
  instance.transform.matrix[0][0] = 1.0F;
  instance.transform.matrix[1][1] = 1.0F;
  instance.transform.matrix[2][2] = 1.0F;
  instance.instanceCustomIndex = 0;
  instance.mask = 0xFF;
  instance.instanceShaderBindingTableRecordOffset = 0;
  instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
  instance.accelerationStructureReference = blas_->device_address();

  // Upload the Instance to a Buffer
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

  // Describe TLAS Geometry
  // This tells Vulkan where the instance array is. arrayOfPointers = VK_FALSE means 
  // the buffer contains actual VkAccelerationStructureInstanceKHR structs directly, not pointers to them
  VkAccelerationStructureGeometryInstancesDataKHR instances_data{};
  instances_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  instances_data.arrayOfPointers = VK_FALSE;
  instances_data.data.deviceAddress = instance_buffer_->device_address(device);

  // This says: "the TLAS geometry is an array of BLAS instances."
  VkAccelerationStructureGeometryKHR tlas_geometry{};
  tlas_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  tlas_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  tlas_geometry.geometry.instances = instances_data;

  // Describe the TLAS Build
  VkAccelerationStructureBuildGeometryInfoKHR tlas_build_info{};
  tlas_build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  tlas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  tlas_build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  tlas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  tlas_build_info.geometryCount = 1;
  tlas_build_info.pGeometries = &tlas_geometry;

  // Query TLAS Size
  constexpr std::uint32_t instance_count = 1;
  VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes{};
  tlas_build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  vkGetAccelerationStructureBuildSizesKHR(device.device(),
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &tlas_build_info,
    &instance_count,
    &tlas_build_sizes);

  // Allocate TLAS and Scratch. tlas_ stays alive and is what the shader traces against.
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

  // Execute the TLAS Build
  OneShotCommandBuffer command_buffer{device};
  command_buffer.begin();
  vkCmdBuildAccelerationStructuresKHR(command_buffer.command_buffer(), 1, &tlas_build_info, tlas_build_ranges);
  command_buffer.end_submit_and_wait();

  // terms:
  // BLAS = actual mesh geometry
  // instance = one placement / reference of a BLAS 
  // TLAS = collection of instances
}
}

/*
A BLAS contains triangles:

BLAS:
  triangle 0
  triangle 1
  triangle 2
An instance says:

Use this BLAS,
with this transform,
with this visibility mask,
with this hit shader offset.
A TLAS contains one or more of those instances:

TLAS:
  instance 0 -> BLAS A at transform A
  instance 1 -> BLAS A at transform B
  instance 2 -> BLAS B at transform C
So if you have one mesh and want to place it 100 times in the world, you can build the BLAS once and create 100 TLAS
instances pointing to it.

https://docs.vulkan.org/tutorial/latest/courses/18_Ray_tracing/02_Acceleration_structures.html
https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
*/


