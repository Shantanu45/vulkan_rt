#pragma once

#include "render/vulkan/SurfaceProvider.hpp"

struct SDL_Window;

namespace vulkan_rt::app
{
class SdlSurfaceProvider final : public render::vulkan::SurfaceProvider
{
public:
  explicit SdlSurfaceProvider(SDL_Window *window);

  [[nodiscard]] std::vector<const char *> required_vulkan_instance_extensions() const override;
  [[nodiscard]] VkSurfaceKHR create_vulkan_surface(VkInstance instance) const override;

private:
  SDL_Window *window_ = nullptr;
};
}
