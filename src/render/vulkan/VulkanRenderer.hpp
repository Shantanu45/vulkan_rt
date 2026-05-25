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
#include "RayTracingDescriptorSet.hpp"
#include "RayTracingPipeline.hpp"
#include "RayTracingScene.hpp"
#include "ShaderBindingTable.hpp"
#include "ShaderModule.hpp"
#include "VulkanAllocator.hpp"
#include "VulkanContext.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanFrameResources.hpp"
#include "VulkanImage.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace vulkan_rt::render::vulkan {
class VulkanRenderer final : public Renderer
{
public:
  VulkanRenderer(
    const VulkanRendererConfig &config, const SurfaceProvider &surface_provider, SwapchainExtent requested_extent);
  virtual ~VulkanRenderer() override;

  void render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera) override;

  void resize(int width, int height) override;

  void wait_idle() override;

private:
  void create_ray_tracing_resources(const scene::Scene &scene);
  void destroy_ray_tracing_resources();
  void recreate_swapchain_if_needed(const scene::Scene &scene);
  void recreate_swapchain(SwapchainExtent extent, const scene::Scene &scene);
  void trace_to_output_image(VkCommandBuffer command_buffer);
  void copy_output_image_to_swapchain(VkCommandBuffer command_buffer, std::uint32_t image_index);

  VulkanContext context_;
  VulkanDevice device_;
  VulkanAllocator allocator_;
  VulkanSwapchain swapchain_;
  VulkanFrameResources frames_;
  std::unique_ptr<RayTracingScene> ray_tracing_scene_;
  std::unique_ptr<VulkanImage> output_image_;
  std::unique_ptr<RayTracingDescriptorSet> descriptors_;
  std::unique_ptr<ShaderModule> raygen_shader_;
  std::unique_ptr<ShaderModule> miss_shader_;
  std::unique_ptr<ShaderModule> closest_hit_shader_;
  std::unique_ptr<RayTracingPipeline> pipeline_;
  std::unique_ptr<ShaderBindingTable> sbt_;
  std::vector<VkImageLayout> swapchain_image_layouts_;
  VkImageLayout output_image_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
  SwapchainExtent pending_extent_{};
  bool swapchain_recreate_requested_ = false;
};
}// namespace vulkan_rt::render::vulkan
