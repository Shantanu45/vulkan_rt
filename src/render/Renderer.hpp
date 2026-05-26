#pragma once

#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <cstdint>

namespace vulkan_rt::render {
struct RendererSettings
{
  std::uint32_t max_bounces = 2;
  bool direct_lighting_enabled = true;
  bool jitter_enabled = true;
  float exposure = 1.0F;
};

struct RenderFrameInfo
{
  std::uint64_t frame_index = 0;
  double frame_time_ms = 0.0;
  double fps = 0.0;
  bool reset_accumulation = false;
  RendererSettings settings = {};
};

class Renderer
{
public:
  virtual ~Renderer() = default;

  virtual void render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera) = 0;
  virtual void resize(int width, int height) = 0;
  virtual void wait_idle() = 0;
};
}// namespace vulkan_rt::render
