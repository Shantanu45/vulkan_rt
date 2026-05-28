#include "engine/Engine.hpp"

#include "render/NullRenderer.hpp"
#include "scene/SceneDescription.hpp"
#include "util/logger.h"

#include <cmath>
#include <filesystem>
#include <internal_use_only/config.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace vulkan_rt::engine {
namespace
{
scene::Scene load_engine_scene(const EngineConfig &config)
{
  if(config.scene_file.empty())
  {
    return scene::make_procedural_scene();
  }

  scene::SceneDescription description;
  std::string error_message;
  std::filesystem::path scene_file_path{config.scene_file};
  if(!scene_file_path.is_absolute() && !std::filesystem::exists(scene_file_path))
  {
    const auto source_relative_path = std::filesystem::path{vulkan_rt::cmake::source_dir} / scene_file_path;
    if(std::filesystem::exists(source_relative_path))
    {
      scene_file_path = source_relative_path;
    }
  }

  const auto resolved_scene_file = scene_file_path.string();
  if(!scene::load_scene_description(resolved_scene_file, description, error_message))
  {
    throw std::runtime_error("Failed to load scene file '" + resolved_scene_file + "': " + error_message);
  }

  auto loaded_scene = scene::make_scene_from_description(description, error_message);
  if(loaded_scene.empty())
  {
    throw std::runtime_error("Scene file '" + resolved_scene_file + "' produced no renderable scene: " + error_message);
  }

  LOGI("Loaded scene file '{}': {}", resolved_scene_file, description.name);
  return loaded_scene;
}
}

Engine::Engine(EngineConfig config)
  : config_(std::move(config)), scene_(load_engine_scene(config_)),
    renderer_(std::make_unique<render::NullRenderer>())
{
  LOGD("Engine initialized: validation={}, gpu={}, scene={}, scene_file={}",
    config_.validation,
    config_.gpu_index,
    config_.scene,
    config_.scene_file);
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
    const float speed = input.fast_move ? camera_settings_.fast_move_speed : camera_settings_.move_speed;
    const float distance = speed * static_cast<float>(delta_seconds) / movement_length;
    camera_.move_local(forward * distance, right * distance, up * distance);
    changed = true;
  }

  if(input.rotate && (input.mouse_delta_x != 0.0F || input.mouse_delta_y != 0.0F))
  {
    camera_.set_orientation(
      camera_.yaw_degrees() + input.mouse_delta_x * camera_settings_.mouse_degrees_per_pixel,
      camera_.pitch_degrees() - input.mouse_delta_y * camera_settings_.mouse_degrees_per_pixel);
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
  const bool reset_accumulation = reset_accumulation_requested_;
  renderer_->render(
    render::RenderFrameInfo{
      .frame_index = frame_stats_.frame_index,
      .frame_time_ms = frame_stats_.frame_time_ms,
      .fps = frame_stats_.fps,
      .reset_accumulation = reset_accumulation,
      .settings = renderer_settings_,
    },
    scene_,
    camera_);
  frame_stats_.accumulated_sample_count = reset_accumulation ? 1 : frame_stats_.accumulated_sample_count + 1;
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

void Engine::set_camera_settings(const CameraSettings &settings)
{
  const float previous_fov = camera_.vertical_fov_degrees();
  camera_settings_ = settings;
  camera_.set_vertical_fov_degrees(camera_settings_.vertical_fov_degrees);
  camera_settings_.vertical_fov_degrees = camera_.vertical_fov_degrees();

  if(camera_.vertical_fov_degrees() != previous_fov)
  {
    reset_accumulation_requested_ = true;
  }
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

const CameraSettings &Engine::camera_settings() const { return camera_settings_; }
}// namespace vulkan_rt::engine
