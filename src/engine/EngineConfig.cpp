#include "engine/EngineConfig.hpp"

namespace vulkan_rt::engine
{
EngineConfig make_engine_config(const app::AppConfig &config)
{
  EngineConfig engine_config;
  engine_config.validation = config.validation;
  engine_config.gpu_index = config.gpu_index;
  engine_config.scene = config.scene;
  return engine_config;
}
}
