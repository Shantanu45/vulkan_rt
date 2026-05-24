#include "VulkanRenderer.hpp"
#include <stdexcept>

namespace vulkan_rt::render::vulkan {

VulkanRenderer::VulkanRenderer(const VulkanRendererConfig &config, const SurfaceProvider &surface_provider)
  : context_(config, surface_provider)
  , device_(context_, config)
{
}

void VulkanRenderer::render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera)
{
  static_cast<void>(frame_info);
  static_cast<void>(scene);
  static_cast<void>(camera);
  throw std::logic_error("The method or operation is not implemented.");
}

void VulkanRenderer::resize(int width, int height)
{
  static_cast<void>(width);
  static_cast<void>(height);
  throw std::logic_error("The method or operation is not implemented.");
}

void VulkanRenderer::wait_idle()
{
  device_.wait_idle();
}

VulkanRenderer::~VulkanRenderer() {}

}// namespace vulkan_rt::render::vulkan
