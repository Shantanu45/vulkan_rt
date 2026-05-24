#include "app/AppConfig.hpp"
#include "app/Application.hpp"
#include "render/vulkan/VulkanCheck.hpp"

#include <fmt/format.h>
#include <internal_use_only/config.hpp>
#include <spdlog/spdlog.h>
#include <memory>
#include <utility>
#include "util/logger.h"

int main(int argc, char **argv)
{
  auto logger = std::make_unique<::Util::StdSpdLogger>();
  ::Util::set_logger_iface(logger.get());
  auto config = vulkan_rt::app::parse_app_config(argc, argv);
  if(config.verbose) { spdlog::set_level(spdlog::level::debug); }

  spdlog::debug("Git SHA: {}", vulkan_rt::cmake::git_sha);
  if(config.dry_run_config)
  {
    LOGI(
      "{} {} config: {}x{}, validation={}, gpu={}, scene={}, app_smoke={}",
      vulkan_rt::cmake::project_name,
      vulkan_rt::cmake::project_version,
      config.width,
      config.height,
      config.validation,
      config.gpu_index,
      config.scene,
      config.app_smoke);
    return 0;
  }

  if (config.check_vulkan)
  { 
      auto result = vulkan_rt::render::vulkan::check_vulkan(config.validation);
      LOGI("\n loader: {},\n instance creation: {},\n validation: {},\n physical device count: {}\n",
        result.loader_present,
        result.instance_created,
        result.validation_layer_available,
        result.physical_device_count);
      if(!result.error.empty())
      {
        LOGE("Vulkan check failed: {}", result.error);
        return 1;
      }
      return 0;
  }

  const bool app_smoke = config.app_smoke;
  vulkan_rt::app::Application application{std::move(config)};
  if(app_smoke) { return application.smoke_test(); }
  return application.run();
}
