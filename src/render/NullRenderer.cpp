#include "render/NullRenderer.hpp"

namespace vulkan_rt::render {
void NullRenderer::render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera)
{
  static_cast<void>(frame_info);
  static_cast<void>(scene);
  static_cast<void>(camera);
}

void NullRenderer::resize(int width, int height)
{
  static_cast<void>(width);
  static_cast<void>(height);
}

void NullRenderer::wait_idle() {}
}// namespace vulkan_rt::render
