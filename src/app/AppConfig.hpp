#pragma once

#include <string>

namespace vulkan_rt::app
{
struct AppConfig
{
  int width = 1280;
  int height = 720;
  bool validation = false;
  int gpu_index = -1;
  std::string scene = "procedural";
  bool verbose = false;
  bool dry_run_config = false;
};

AppConfig parse_app_config(int argc, char **argv);
}
