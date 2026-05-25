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
  cli.add_flag("--vulkan-shader-smoke",
    config.vulkan_shader_smoke,
    "Create Vulkan shader modules from the compiled ray tracing SPIR-V files, then report handles.");
  cli.add_flag("--vulkan-rt-pipeline-smoke",
    config.vulkan_rt_pipeline_smoke,
    "Create a minimal Vulkan ray tracing pipeline from the compiled shader modules.");
  cli.add_flag("--vulkan-sbt-smoke",
    config.vulkan_sbt_smoke,
    "Create a shader binding table for the minimal Vulkan ray tracing pipeline.");
  cli.add_flag("--vulkan-rt-descriptor-smoke",
    config.vulkan_rt_descriptor_smoke,
    "Create ray tracing descriptors for a TLAS and storage output image.");
  cli.add_flag("--vulkan-trace-smoke",
    config.vulkan_trace_smoke,
    "Dispatch a minimal offscreen vkCmdTraceRaysKHR pass into a storage image.");
  cli.add_flag("--vulkan-acceleration-structure-smoke",
    config.vulkan_acceleration_structure_smoke,
    "Create a minimal Vulkan acceleration structure handle backed by VMA memory.");
  cli.add_flag("--vulkan-triangle-blas-smoke",
    config.vulkan_triangle_blas_smoke,
    "Build a bottom-level acceleration structure for one triangle.");
  cli.add_flag("--vulkan-tlas-smoke",
    config.vulkan_tlas_smoke,
    "Build a top-level acceleration structure with one triangle BLAS instance.");
  cli.add_flag("--vulkan-buffer-smoke",
    config.vulkan_buffer_smoke,
    "Create VulkanAllocator and allocate a staging and device-local buffer via VMA.");
  cli.add_flag("--vulkan-clear-smoke",
    config.vulkan_clear_smoke,
    "Create VulkanRenderer, clear/present a few frames, and exit.");
  cli.add_flag("--vulkan-resize-smoke",
    config.vulkan_resize_smoke,
    "Create VulkanRenderer, recreate the swapchain through resize(), and exit.");

  try {
    cli.parse(argc, argv);
  } catch (const CLI::ParseError &error) {
    std::exit(cli.exit(error));
  }

  return config;
}
}// namespace vulkan_rt::app
