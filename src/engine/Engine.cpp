#include "engine/Engine.hpp"

#include "render/NullRenderer.hpp"
#include "util/logger.h"

#include <memory>
#include <utility>

namespace vulkan_rt::engine
{
Engine::Engine(EngineConfig config)
  : config_(std::move(config))
  , scene_(scene::make_procedural_scene())
  , renderer_(std::make_unique<render::NullRenderer>())
{
  LOGD(
    "Engine initialized: validation={}, gpu={}, scene={}", config_.validation, config_.gpu_index, config_.scene);
  LOGD("Scene initialized: materials={}, triangles={}", scene_.materials().size(), scene_.triangles().size());
}

Engine::~Engine() = default;

void Engine::update(double delta_seconds)
{
  frame_stats_.frame_time_ms = delta_seconds * 1000.0;
  frame_stats_.fps = delta_seconds > 0.0 ? 1.0 / delta_seconds : 0.0;
}

void Engine::render()
{
  renderer_->render(render::RenderFrameInfo{
    .frame_index = frame_stats_.frame_index,
    .frame_time_ms = frame_stats_.frame_time_ms,
    .fps = frame_stats_.fps,
  });
  ++frame_stats_.frame_index;
}

void Engine::resize(int width, int height)
{
  camera_.set_viewport_size(width, height);
  renderer_->resize(width, height);
}

const FrameStats &Engine::frame_stats() const
{
  return frame_stats_;
}

const EngineConfig &Engine::config() const
{
  return config_;
}

const scene::Scene &Engine::scene() const
{
  return scene_;
}

const scene::Camera &Engine::camera() const
{
  return camera_;
}
}
