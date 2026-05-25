#pragma once

#include "AccelerationStructure.hpp"
#include "VulkanAllocator.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"
#include "scene/Scene.hpp"

#include <cstdint>
#include <memory>

namespace vulkan_rt::render::vulkan
{
class RayTracingScene
{
public:
  RayTracingScene(const VulkanDevice &device, const VulkanAllocator &allocator, const scene::Scene &scene);
  ~RayTracingScene();

  RayTracingScene(const RayTracingScene &) = delete;
  RayTracingScene &operator=(const RayTracingScene &) = delete;

  [[nodiscard]] const AccelerationStructure &tlas() const;
  [[nodiscard]] const VulkanBuffer &material_index_buffer() const;
  [[nodiscard]] const VulkanBuffer &material_buffer() const;

private:
  void build_scene(const VulkanDevice &device, const VulkanAllocator &allocator, const scene::Scene &scene);

  std::uint32_t triangle_count_ = 0;
  std::unique_ptr<VulkanBuffer> vertex_buffer_;
  std::unique_ptr<VulkanBuffer> material_index_buffer_;
  std::unique_ptr<VulkanBuffer> material_buffer_;
  std::unique_ptr<AccelerationStructure> blas_;
  std::unique_ptr<VulkanBuffer> instance_buffer_;
  std::unique_ptr<AccelerationStructure> tlas_;
};
}
