#include "app/AppConfig.hpp"
#include "app/Application.hpp"

#include <fmt/format.h>
#include <internal_use_only/config.hpp>
#include <spdlog/spdlog.h>
#include <utility>

int main(int argc, char **argv)
{
  auto config = vulkan_rt::app::parse_app_config(argc, argv);
  if(config.verbose) { spdlog::set_level(spdlog::level::debug); }

  spdlog::debug("Git SHA: {}", vulkan_rt::cmake::git_sha);
  if(config.dry_run_config)
  {
    fmt::println(
      "{} {} config: {}x{}, validation={}, gpu={}, scene={}",
      vulkan_rt::cmake::project_name,
      vulkan_rt::cmake::project_version,
      config.width,
      config.height,
      config.validation,
      config.gpu_index,
      config.scene);
    return 0;
  }

  vulkan_rt::app::Application application{std::move(config)};
  return application.run();
}
