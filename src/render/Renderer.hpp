#pragma once

#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <cstdint>

namespace vulkan_rt::render
{
struct RenderFrameInfo
{
  std::uint64_t frame_index = 0;
  double frame_time_ms = 0.0;
  double fps = 0.0;
};

class Renderer
{
public:
  virtual ~Renderer() = default;

  virtual void render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera) = 0;
  virtual void resize(int width, int height) = 0;
  virtual void wait_idle() = 0;
};
}
