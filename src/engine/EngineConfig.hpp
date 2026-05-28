#pragma once

#include "app/AppConfig.hpp"

#include <string>

namespace vulkan_rt::engine {
struct EngineConfig
{
  bool validation = false;
  int gpu_index = -1;
  std::string scene = "procedural";
  std::string scene_file;
};

[[nodiscard]] EngineConfig make_engine_config(const app::AppConfig &config);
}// namespace vulkan_rt::engine
