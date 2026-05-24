#include "app/AppConfig.hpp"

#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <internal_use_only/config.hpp>

#include <cstdlib>

namespace vulkan_rt::app {
AppConfig parse_app_config(int argc, char **argv)
{
  AppConfig config;

  CLI::App cli{ fmt::format("{} desktop application", vulkan_rt::cmake::project_name) };
  cli.set_version_flag("--version", std::string{ vulkan_rt::cmake::project_version });

  cli.add_flag("-v,--verbose", config.verbose, "Enable verbose logging");
  cli.add_option("--width", config.width, "Window width")->check(CLI::PositiveNumber);
  cli.add_option("--height", config.height, "Window height")->check(CLI::PositiveNumber);
  cli.add_flag("--validation", config.validation, "Enable Vulkan validation when the renderer is available");
  cli.add_option("--gpu", config.gpu_index, "Requested GPU index; -1 selects the default device");
  cli.add_option("--scene", config.scene, "Scene source to load")->check(CLI::IsMember({ "procedural" }));
  cli.add_flag("--dry-run-config", config.dry_run_config, "Validate CLI configuration without opening a window");
  cli.add_flag("--app-smoke", config.app_smoke, "Create the SDL application shell, tick once, and exit");
  cli.add_flag("--check-vulkan",
    config.check_vulkan,
    "Reports if Vulkan loader is preset, instance can be created, validation availability and physical devices.");
  cli.add_flag("--vulkan-context-smoke",
    config.vulkan_context_smoke,
    "Create an SDL Vulkan window and verify VulkanContext instance/surface creation.");
  cli.add_flag("--vulkan-device-smoke",
    config.vulkan_device_smoke,
    "Create VulkanContext and VulkanDevice, then report selected GPU and queue families.");
  cli.add_flag("--vulkan-swapchain-smoke",
    config.vulkan_swapchain_smoke,
    "Create VulkanContext, VulkanDevice, and VulkanSwapchain, then report swapchain details.");
  cli.add_flag("--vulkan-frame-smoke",
    config.vulkan_frame_smoke,
    "Create Vulkan frame command buffers and synchronization objects, then report counts.");

  try {
    cli.parse(argc, argv);
  } catch (const CLI::ParseError &error) {
    std::exit(cli.exit(error));
  }

  return config;
}
}// namespace vulkan_rt::app
