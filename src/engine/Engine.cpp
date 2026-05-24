#include "engine/Engine.hpp"

#include "util/logger.h"

#include <utility>

namespace vulkan_rt::engine
{
Engine::Engine(EngineConfig config)
  : config_(std::move(config))
{
  LOGD(
    "Engine initialized: validation={}, gpu={}, scene={}", config_.validation, config_.gpu_index, config_.scene);
}

void Engine::update([[maybe_unused]] const app::Input &input, double delta_seconds)
{
  frame_stats_.frame_time_ms = delta_seconds * 1000.0;
  frame_stats_.fps = delta_seconds > 0.0 ? 1.0 / delta_seconds : 0.0;
}

void Engine::render()
{
  ++frame_stats_.frame_index;
}

const FrameStats &Engine::frame_stats() const
{
  return frame_stats_;
}

const EngineConfig &Engine::config() const
{
  return config_;
}
}
