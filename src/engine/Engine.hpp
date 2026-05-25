#pragma once

#include "engine/EngineConfig.hpp"
#include "engine/FrameStats.hpp"
#include "render/Renderer.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <memory>

namespace vulkan_rt::engine {
struct CameraControlInput
{
  bool move_forward = false;
  bool move_backward = false;
  bool move_left = false;
  bool move_right = false;
  bool move_up = false;
  bool move_down = false;
  bool fast_move = false;
  bool rotate = false;
  float mouse_delta_x = 0.0F;
  float mouse_delta_y = 0.0F;
};

class Engine
{
public:
  explicit Engine(EngineConfig config);
  ~Engine();

  void update(double delta_seconds);
  bool update_camera(const CameraControlInput &input, double delta_seconds);
  void render();
  void resize(int width, int height);
  void set_renderer(std::unique_ptr<render::Renderer> renderer);

  [[nodiscard]] const FrameStats &frame_stats() const;
  [[nodiscard]] const EngineConfig &config() const;
  [[nodiscard]] const scene::Scene &scene() const;
  [[nodiscard]] const scene::Camera &camera() const;

private:
  EngineConfig config_;
  FrameStats frame_stats_;
  scene::Scene scene_;
  scene::Camera camera_;
  std::unique_ptr<render::Renderer> renderer_;
};
}// namespace vulkan_rt::engine
