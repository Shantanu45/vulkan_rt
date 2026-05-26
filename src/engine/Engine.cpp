#include "engine/Engine.hpp"

#include "render/NullRenderer.hpp"
#include "util/logger.h"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

namespace vulkan_rt::engine {
namespace
{
constexpr float camera_move_speed = 3.0F;
constexpr float camera_fast_move_speed = 10.0F;
constexpr float mouse_degrees_per_pixel = 0.1F;
}

Engine::Engine(EngineConfig config)
  : config_(std::move(config)), scene_(scene::make_procedural_scene()),
    renderer_(std::make_unique<render::NullRenderer>())
{
  LOGD("Engine initialized: validation={}, gpu={}, scene={}", config_.validation, config_.gpu_index, config_.scene);
  LOGD("Scene initialized: materials={}, triangles={}", scene_.materials().size(), scene_.triangles().size());
}

Engine::~Engine() = default;

void Engine::update(double delta_seconds)
{
  frame_stats_.frame_time_ms = delta_seconds * 1000.0;
  frame_stats_.fps = delta_seconds > 0.0 ? 1.0 / delta_seconds : 0.0;
}

bool Engine::update_camera(const CameraControlInput &input, double delta_seconds)
{
  bool changed = false;

  float forward = (input.move_forward ? 1.0F : 0.0F) - (input.move_backward ? 1.0F : 0.0F);
  float right = (input.move_right ? 1.0F : 0.0F) - (input.move_left ? 1.0F : 0.0F);
  float up = (input.move_up ? 1.0F : 0.0F) - (input.move_down ? 1.0F : 0.0F);

  const float movement_length = std::sqrt(forward * forward + right * right + up * up);
  if(movement_length > 0.0F)
  {
    const float speed = input.fast_move ? camera_fast_move_speed : camera_move_speed;
    const float distance = speed * static_cast<float>(delta_seconds) / movement_length;
    camera_.move_local(forward * distance, right * distance, up * distance);
    changed = true;
  }

  if(input.rotate && (input.mouse_delta_x != 0.0F || input.mouse_delta_y != 0.0F))
  {
    camera_.set_orientation(
      camera_.yaw_degrees() + input.mouse_delta_x * mouse_degrees_per_pixel,
      camera_.pitch_degrees() - input.mouse_delta_y * mouse_degrees_per_pixel);
    changed = true;
  }

  if(changed)
  {
    reset_accumulation_requested_ = true;
  }

  return changed;
}

void Engine::render()
{
  renderer_->render(
    render::RenderFrameInfo{
      .frame_index = frame_stats_.frame_index,
      .frame_time_ms = frame_stats_.frame_time_ms,
      .fps = frame_stats_.fps,
      .reset_accumulation = reset_accumulation_requested_,
      .settings = renderer_settings_,
    },
    scene_,
    camera_);
  reset_accumulation_requested_ = false;
  ++frame_stats_.frame_index;
}

void Engine::set_renderer(std::unique_ptr<render::Renderer> renderer)
{
  if(!renderer)
  {
    throw std::runtime_error("Cannot install a null renderer.");
  }

  renderer_ = std::move(renderer);
}

void Engine::set_renderer_settings(const render::RendererSettings &settings)
{
  if(settings.max_bounces != renderer_settings_.max_bounces ||
     settings.direct_lighting_enabled != renderer_settings_.direct_lighting_enabled ||
     settings.jitter_enabled != renderer_settings_.jitter_enabled)
  {
    reset_accumulation_requested_ = true;
  }

  renderer_settings_ = settings;
}

void Engine::request_accumulation_reset()
{
  reset_accumulation_requested_ = true;
}

void Engine::resize(int width, int height)
{
  camera_.set_viewport_size(width, height);
  renderer_->resize(width, height);
  reset_accumulation_requested_ = true;
}

const FrameStats &Engine::frame_stats() const { return frame_stats_; }

const EngineConfig &Engine::config() const { return config_; }

const scene::Scene &Engine::scene() const { return scene_; }

const scene::Camera &Engine::camera() const { return camera_; }

const render::RendererSettings &Engine::renderer_settings() const { return renderer_settings_; }
}// namespace vulkan_rt::engine
