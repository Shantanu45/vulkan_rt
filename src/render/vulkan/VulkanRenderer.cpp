#include "VulkanRenderer.hpp"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include <internal_use_only/config.hpp>

namespace vulkan_rt::render::vulkan {
namespace
{
std::string vulkan_result_message(const char *operation, VkResult result)
{
  return std::string{operation} + " failed with VkResult " + std::to_string(static_cast<int>(result));
}

void throw_if_failed(VkResult result, const char *operation)
{
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error(vulkan_result_message(operation, result));
  }
}

void transition_image_layout(
  VkCommandBuffer command_buffer,
  VkImage image,
  VkImageLayout old_layout,
  VkImageLayout new_layout,
  VkAccessFlags src_access_mask,
  VkAccessFlags dst_access_mask,
  VkPipelineStageFlags src_stage,
  VkPipelineStageFlags dst_stage)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = src_access_mask;
  barrier.dstAccessMask = dst_access_mask;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

VkExtent3D extent_3d(VkExtent2D extent)
{
  return VkExtent3D{.width = extent.width, .height = extent.height, .depth = 1};
}
}

VulkanRenderer::VulkanRenderer(
  const VulkanRendererConfig &config, const SurfaceProvider &surface_provider, SwapchainExtent requested_extent)
  : context_(config, surface_provider)
  , device_(context_, config)
  , allocator_(context_, device_)
  , swapchain_(context_, device_, requested_extent)
  , frames_(device_, 2)
{
  swapchain_image_layouts_.assign(swapchain_.images().size(), VK_IMAGE_LAYOUT_UNDEFINED);
}

void VulkanRenderer::render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera)
{
  static_cast<void>(frame_info);

  recreate_swapchain_if_needed(scene, camera);
  if(!ray_tracing_scene_)
  {
    create_ray_tracing_resources(scene, camera);
  }
  ray_tracing_camera_->update(camera);

  const auto frame_index = frames_.current_frame_index();
  const auto command_buffer = frames_.command_buffers()[frame_index];
  const auto image_available = frames_.image_available_semaphores()[frame_index];
  const auto render_finished = frames_.render_finished_semaphores()[frame_index];
  const auto in_flight_fence = frames_.in_flight_fences()[frame_index];

  throw_if_failed(
    vkWaitForFences(device_.device(), 1, &in_flight_fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()),
    "vkWaitForFences");

  std::uint32_t image_index = 0;
  const VkResult acquire_result = vkAcquireNextImageKHR(device_.device(),
    swapchain_.swapchain(),
    std::numeric_limits<std::uint64_t>::max(),
    image_available,
    VK_NULL_HANDLE,
    &image_index);
  if(acquire_result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    swapchain_recreate_requested_ = true;
    return;
  }
  if(acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
  {
    throw_if_failed(acquire_result, "vkAcquireNextImageKHR");
  }
  if(acquire_result == VK_SUBOPTIMAL_KHR)
  {
    swapchain_recreate_requested_ = true;
  }

  throw_if_failed(vkResetFences(device_.device(), 1, &in_flight_fence), "vkResetFences");
  throw_if_failed(vkResetCommandBuffer(command_buffer, 0), "vkResetCommandBuffer");

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  throw_if_failed(vkBeginCommandBuffer(command_buffer, &begin_info), "vkBeginCommandBuffer");

  trace_to_output_image(command_buffer);
  copy_output_image_to_swapchain(command_buffer, image_index);

  throw_if_failed(vkEndCommandBuffer(command_buffer), "vkEndCommandBuffer");

  const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_available;
  submit_info.pWaitDstStageMask = &wait_stage;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &render_finished;

  throw_if_failed(vkQueueSubmit(device_.graphics_queue(), 1, &submit_info, in_flight_fence), "vkQueueSubmit");

  const auto swapchain = swapchain_.swapchain();
  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &render_finished;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain;
  present_info.pImageIndices = &image_index;

  const VkResult present_result = vkQueuePresentKHR(device_.present_queue(), &present_info);
  if(present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
  {
    swapchain_recreate_requested_ = true;
  }
  else if(present_result != VK_SUCCESS)
  {
    throw_if_failed(present_result, "vkQueuePresentKHR");
  }

  swapchain_image_layouts_[image_index] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  frames_.advance_frame();
}

void VulkanRenderer::resize(int width, int height)
{
  if(width <= 0 || height <= 0)
  {
    return;
  }

  pending_extent_ = SwapchainExtent{
    .width = static_cast<std::uint32_t>(width),
    .height = static_cast<std::uint32_t>(height),
  };
  swapchain_recreate_requested_ = true;
}

void VulkanRenderer::wait_idle()
{
  device_.wait_idle();
}

VulkanRenderer::~VulkanRenderer() {}

void VulkanRenderer::create_ray_tracing_resources(const scene::Scene &scene, const scene::Camera &camera)
{
  const auto extent = swapchain_.extent();
  ray_tracing_scene_ = std::make_unique<RayTracingScene>(device_, allocator_, scene);
  ray_tracing_camera_ = std::make_unique<RayTracingCamera>(allocator_, camera);
  output_image_ = std::make_unique<VulkanImage>(device_,
    allocator_,
    ImageCreateInfo{
      .width = extent.width,
      .height = extent.height,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO,
    });
  descriptors_ = std::make_unique<RayTracingDescriptorSet>(device_,
    ray_tracing_scene_->tlas(),
    *output_image_,
    ray_tracing_scene_->material_index_buffer(),
    ray_tracing_scene_->material_buffer(),
    ray_tracing_camera_->buffer());

  const std::filesystem::path shader_dir{vulkan_rt::cmake::shader_dir};
  raygen_shader_ = std::make_unique<ShaderModule>(device_, shader_dir / "raygen.rgen.spv");
  miss_shader_ = std::make_unique<ShaderModule>(device_, shader_dir / "miss.rmiss.spv");
  closest_hit_shader_ = std::make_unique<ShaderModule>(device_, shader_dir / "closesthit.rchit.spv");
  pipeline_ = std::make_unique<RayTracingPipeline>(
    device_, *raygen_shader_, *miss_shader_, *closest_hit_shader_, descriptors_->layout());
  sbt_ = std::make_unique<ShaderBindingTable>(device_, allocator_, *pipeline_);
  output_image_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

void VulkanRenderer::destroy_ray_tracing_resources()
{
  sbt_.reset();
  pipeline_.reset();
  closest_hit_shader_.reset();
  miss_shader_.reset();
  raygen_shader_.reset();
  descriptors_.reset();
  output_image_.reset();
  ray_tracing_camera_.reset();
  ray_tracing_scene_.reset();
  output_image_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}

void VulkanRenderer::recreate_swapchain_if_needed(const scene::Scene &scene, const scene::Camera &camera)
{
  if(!swapchain_recreate_requested_)
  {
    return;
  }

  recreate_swapchain(pending_extent_, scene, camera);
  swapchain_recreate_requested_ = false;
}

void VulkanRenderer::recreate_swapchain(SwapchainExtent extent, const scene::Scene &scene, const scene::Camera &camera)
{
  device_.wait_idle();
  destroy_ray_tracing_resources();

  // Later: pass the old swapchain handle into recreation instead of blunt replacement.
  swapchain_.recreate(context_, device_, extent);
  swapchain_image_layouts_.assign(swapchain_.images().size(), VK_IMAGE_LAYOUT_UNDEFINED);
  create_ray_tracing_resources(scene, camera);
}

void VulkanRenderer::trace_to_output_image(VkCommandBuffer command_buffer)
{
  if(!output_image_ || !pipeline_ || !descriptors_ || !sbt_)
  {
    throw std::runtime_error("Ray tracing resources are not initialized.");
  }

  const VkAccessFlags src_access =
    output_image_layout_ == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ? VK_ACCESS_TRANSFER_READ_BIT : 0;
  const VkPipelineStageFlags src_stage =
    output_image_layout_ == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT;

  transition_image_layout(command_buffer,
    output_image_->image(),
    output_image_layout_,
    VK_IMAGE_LAYOUT_GENERAL,
    src_access,
    VK_ACCESS_SHADER_WRITE_BIT,
    src_stage,
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_->pipeline());
  const auto descriptor_set = descriptors_->descriptor_set();
  vkCmdBindDescriptorSets(command_buffer,
    VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
    pipeline_->layout(),
    0,
    1,
    &descriptor_set,
    0,
    nullptr);
  vkCmdTraceRaysKHR(command_buffer,
    &sbt_->raygen_region(),
    &sbt_->miss_region(),
    &sbt_->hit_region(),
    &sbt_->callable_region(),
    output_image_->extent().width,
    output_image_->extent().height,
    1);

  transition_image_layout(command_buffer,
    output_image_->image(),
    VK_IMAGE_LAYOUT_GENERAL,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_ACCESS_SHADER_WRITE_BIT,
    VK_ACCESS_TRANSFER_READ_BIT,
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
    VK_PIPELINE_STAGE_TRANSFER_BIT);
  output_image_layout_ = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
}

void VulkanRenderer::copy_output_image_to_swapchain(VkCommandBuffer command_buffer, std::uint32_t image_index)
{
  const auto old_layout = swapchain_image_layouts_[image_index];
  transition_image_layout(command_buffer,
    swapchain_.images()[image_index],
    old_layout,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    0,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    old_layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT);

  VkImageCopy copy_region{};
  copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.srcSubresource.mipLevel = 0;
  copy_region.srcSubresource.baseArrayLayer = 0;
  copy_region.srcSubresource.layerCount = 1;
  copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.dstSubresource.mipLevel = 0;
  copy_region.dstSubresource.baseArrayLayer = 0;
  copy_region.dstSubresource.layerCount = 1;
  copy_region.extent = extent_3d(output_image_->extent());

  vkCmdCopyImage(command_buffer,
    output_image_->image(),
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    swapchain_.images()[image_index],
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &copy_region);

  transition_image_layout(command_buffer,
    swapchain_.images()[image_index],
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    0,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

}// namespace vulkan_rt::render::vulkan
