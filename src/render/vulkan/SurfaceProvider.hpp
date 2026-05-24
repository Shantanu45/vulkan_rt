#pragma once

#include <volk.h>
#include <vector>

namespace vulkan_rt::render::vulkan
{
class SurfaceProvider
{
public:
  virtual ~SurfaceProvider() = default;

  [[nodiscard]] virtual std::vector<const char*> required_vulkan_instance_extensions() const = 0;
  [[nodiscard]] virtual VkSurfaceKHR create_vulkan_surface(VkInstance instance) const = 0;
};
}
