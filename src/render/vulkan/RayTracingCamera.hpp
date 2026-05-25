#pragma once

#include "VulkanAllocator.hpp"
#include "VulkanBuffer.hpp"
#include "scene/Camera.hpp"

namespace vulkan_rt::render::vulkan
{
class RayTracingCamera
{
public:
  RayTracingCamera(const VulkanAllocator &allocator, const scene::Camera &camera);

  void update(const scene::Camera &camera);

  [[nodiscard]] const VulkanBuffer &buffer() const;

private:
  VulkanBuffer buffer_;
};
}
