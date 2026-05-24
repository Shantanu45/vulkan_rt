#include "VulkanRenderer.hpp"
#include <stdexcept>

namespace vulkan_rt::render::vulkan {

VulkanRenderer::VulkanRenderer() {}

void VulkanRenderer::render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera)
{ throw std::logic_error("The method or operation is not implemented."); }

void VulkanRenderer::resize(int width, int height)
{ throw std::logic_error("The method or operation is not implemented."); }

void VulkanRenderer::wait_idle() { throw std::logic_error("The method or operation is not implemented."); }

VulkanRenderer::~VulkanRenderer() {}

}// namespace vulkan_rt::render::vulkan
