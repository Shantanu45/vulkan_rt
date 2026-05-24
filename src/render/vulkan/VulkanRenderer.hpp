#pragma once
#include "../Renderer.hpp"

namespace vulkan_rt::render {
class VulkanRenderer final : public Renderer
{
  VulkanRenderer();
  virtual ~VulkanRenderer();

  void render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera) override;

  void resize(int width, int height) override;

  void wait_idle() override;
};
}// namespace vulkan_rt::render
