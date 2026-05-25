#pragma once

#include <string>

namespace vulkan_rt::app {
struct AppConfig
{
  int width = 1280;
  int height = 720;
  bool validation = false;
  int gpu_index = -1;
  std::string scene = "procedural";
  bool verbose = false;
  bool dry_run_config = false;
  bool app_smoke = false;
  bool check_vulkan = false;
  bool vulkan_context_smoke = false;
  bool vulkan_device_smoke = false;
  bool vulkan_swapchain_smoke = false;
  bool vulkan_frame_smoke = false;
  bool vulkan_shader_smoke = false;
  bool vulkan_rt_pipeline_smoke = false;
  bool vulkan_sbt_smoke = false;
  bool vulkan_acceleration_structure_smoke = false;
  bool vulkan_triangle_blas_smoke = false;
  bool vulkan_tlas_smoke = false;
  bool vulkan_buffer_smoke = false;
  bool vulkan_clear_smoke = false;
  bool vulkan_resize_smoke = false;
};

AppConfig parse_app_config(int argc, char **argv);
}// namespace vulkan_rt::app
