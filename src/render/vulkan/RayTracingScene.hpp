#pragma once

#include "AccelerationStructure.hpp"
#include "VulkanAllocator.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"

#include <memory>

namespace vulkan_rt::render::vulkan
{
class RayTracingScene
{
public:
  RayTracingScene(const VulkanDevice &device, const VulkanAllocator &allocator);
  ~RayTracingScene();

  RayTracingScene(const RayTracingScene &) = delete;
  RayTracingScene &operator=(const RayTracingScene &) = delete;

  [[nodiscard]] const AccelerationStructure &tlas() const;

private:
  void build_single_triangle(const VulkanDevice &device, const VulkanAllocator &allocator);

  std::unique_ptr<VulkanBuffer> vertex_buffer_;
  std::unique_ptr<AccelerationStructure> blas_;
  std::unique_ptr<VulkanBuffer> instance_buffer_;
  std::unique_ptr<AccelerationStructure> tlas_;
};
}
