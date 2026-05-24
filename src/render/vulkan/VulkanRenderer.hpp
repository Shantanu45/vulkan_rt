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
#include "VulkanContext.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanFrameResources.hpp"

namespace vulkan_rt::render::vulkan {
class VulkanRenderer final : public Renderer
{
public:
  VulkanRenderer();
  virtual ~VulkanRenderer() override;

  void render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera) override;

  void resize(int width, int height) override;

  void wait_idle() override;

private:
  VulkanContext context_;
  VulkanDevice device_;
  VulkanSwapchain swapchain_;
  VulkanFrameResources frames_;
};
}// namespace vulkan_rt::render::vulkan
