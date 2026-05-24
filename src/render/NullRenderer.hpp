#pragma once

#include "render/Renderer.hpp"

namespace vulkan_rt::render
{
class NullRenderer final : public Renderer
{
public:
  void render(const RenderFrameInfo &frame_info) override;
  void resize(int width, int height) override;
  void wait_idle() override;
};
}
