#pragma once

#include "engine/EngineConfig.hpp"
#include "engine/FrameStats.hpp"
#include "render/Renderer.hpp"

#include <memory>

namespace vulkan_rt::engine
{
class Engine
{
public:
  explicit Engine(EngineConfig config);
  ~Engine();

  void update(double delta_seconds);
  void render();
  void resize(int width, int height);

  [[nodiscard]] const FrameStats &frame_stats() const;
  [[nodiscard]] const EngineConfig &config() const;

private:
  EngineConfig config_;
  FrameStats frame_stats_;
  std::unique_ptr<render::Renderer> renderer_;
};
}
