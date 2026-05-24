#pragma once

#include "app/Input.hpp"
#include "engine/EngineConfig.hpp"
#include "engine/FrameStats.hpp"

namespace vulkan_rt::engine
{
class Engine
{
public:
  explicit Engine(EngineConfig config);

  void update(const app::Input &input, double delta_seconds);
  void render();

  [[nodiscard]] const FrameStats &frame_stats() const;
  [[nodiscard]] const EngineConfig &config() const;

private:
  EngineConfig config_;
  FrameStats frame_stats_;
};
}
