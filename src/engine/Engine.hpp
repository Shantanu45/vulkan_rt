#pragma once

#include "engine/EngineConfig.hpp"
#include "engine/FrameStats.hpp"
#include "render/Renderer.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <memory>

namespace vulkan_rt::engine {
class Engine
{
public:
  explicit Engine(EngineConfig config);
  ~Engine();

  void update(double delta_seconds);
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
