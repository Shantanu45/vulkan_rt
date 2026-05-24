#include "app/VulkanContextSmoke.hpp"

#include "app/Application.hpp"
#include "app/SdlSurfaceProvider.hpp"
#include "app/Window.hpp"
#include "render/vulkan/VulkanContext.hpp"
#include "render/vulkan/VulkanRendererConfig.hpp"
#include "util/logger.h"

#include <fmt/format.h>
#include <internal_use_only/config.hpp>

#include <string>

namespace vulkan_rt::app
{
int vulkan_context_smoke_test(const AppConfig &config)
{
  SdlRuntime sdl_runtime;
  Window window{
    fmt::format("{} Vulkan context smoke", vulkan_rt::cmake::project_name),
    config.width,
    config.height,
  };

  SdlSurfaceProvider surface_provider{window.native_handle()};
  const std::string application_name{vulkan_rt::cmake::project_name};
  render::vulkan::VulkanRendererConfig vulkan_config{
    .validation = config.validation,
    .application_name = application_name.c_str(),
    .gpu_index = config.gpu_index,
  };

  render::vulkan::VulkanContext context{vulkan_config, surface_provider};

  LOGI("Vulkan context smoke passed:");
  LOGI("  instance created: {}", context.instance() != VK_NULL_HANDLE);
  LOGI("  surface created: {}", context.surface() != VK_NULL_HANDLE);
  LOGI("  validation enabled: {}", context.validation_enabled());

  return 0;
}
}
