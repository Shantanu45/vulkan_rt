/*****************************************************************//**
 * \file   VulkanRenderer.hpp
 * \brief  VulkanRenderer
 * high-level renderer backend
 * owns context, device, swapchain, frame resources
 * implements Renderer::render / resize / wait_idle
 * 
 * \author Shantanu Kumar
 * \date   May 2026
 *********************************************************************/
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
