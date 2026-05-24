#include "app/SdlSurfaceProvider.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <stdexcept>
#include <string>

namespace vulkan_rt::app
{
SdlSurfaceProvider::SdlSurfaceProvider(SDL_Window *window)
  : window_(window)
{
  if(window_ == nullptr)
  {
    throw std::invalid_argument("SdlSurfaceProvider requires a valid SDL window.");
  }
}

std::vector<const char *> SdlSurfaceProvider::required_vulkan_instance_extensions() const
{
  Uint32 extension_count = 0;
  const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);

  if(extensions == nullptr || extension_count == 0)
  {
    throw std::runtime_error(std::string{"SDL_Vulkan_GetInstanceExtensions failed: "} + SDL_GetError());
  }

  return std::vector<const char *>{extensions, extensions + extension_count};
}

VkSurfaceKHR SdlSurfaceProvider::create_vulkan_surface(VkInstance instance) const
{
  if(instance == VK_NULL_HANDLE)
  {
    throw std::invalid_argument("SdlSurfaceProvider cannot create a surface for VK_NULL_HANDLE instance.");
  }

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if(!SDL_Vulkan_CreateSurface(window_, instance, nullptr, &surface))
  {
    throw std::runtime_error(std::string{"SDL_Vulkan_CreateSurface failed: "} + SDL_GetError());
  }

  return surface;
}
}
