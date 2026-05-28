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

void transition_swapchain_color_attachment_to_present(VkCommandBuffer command_buffer, VkImage image)
{
  transition_image_layout(command_buffer,
    image,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    0,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

ImageCreateInfo make_storage_image_create_info(VkExtent2D extent, VkFormat format, VkImageUsageFlags extra_usage = 0)
{
  return ImageCreateInfo{
    .width = extent.width,
    .height = extent.height,
    .format = format,
    .usage = VK_IMAGE_USAGE_STORAGE_BIT | extra_usage,
    .memory_usage = VMA_MEMORY_USAGE_AUTO,
  };
}
}

VulkanRenderer::VulkanRenderer(
  const VulkanRendererConfig &config, const SurfaceProvider &surface_provider, SwapchainExtent requested_extent)
  : context_(config, surface_provider)
  , device_(context_, config)
  , allocator_(context_, device_)
  , swapchain_(context_, device_, requested_extent)
  , frames_(device_, 2)
  , imgui_(context_, device_, swapchain_, frames_.frames_in_flight())
{
  swapchain_image_layouts_.assign(swapchain_.images().size(), VK_IMAGE_LAYOUT_UNDEFINED);
}

void VulkanRenderer::render(const RenderFrameInfo &frame_info, const scene::Scene &scene, const scene::Camera &camera)
{
  recreate_swapchain_if_needed(scene, camera);
  if(!ray_tracing_)
  {
    create_ray_tracing_resources(scene, camera);
  }
  if(frame_info.reset_accumulation)
  {
    sample_index_ = 0;
  }
  ray_tracing_->camera->update(camera);
  ray_tracing_->frame_data->update(GpuRayTracingFrameData{
    .sample_index = sample_index_,
    .frame_index = static_cast<std::uint32_t>(frame_info.frame_index),
    .reset_accumulation = frame_info.reset_accumulation ? 1U : 0U,
    .light_count = ray_tracing_->scene->light_count(),
    .max_bounces = frame_info.settings.max_bounces,
    .direct_lighting_enabled = frame_info.settings.direct_lighting_enabled ? 1U : 0U,
    .jitter_enabled = frame_info.settings.jitter_enabled ? 1U : 0U,
    ._pad0 = 0U,
    .exposure = frame_info.settings.exposure,
  });

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
  if(!imgui_.render(command_buffer, image_index, swapchain_))
  {
    transition_swapchain_color_attachment_to_present(command_buffer, swapchain_.images()[image_index]);
  }

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
  ++sample_index_;
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
  ray_tracing_ = std::make_unique<RayTracingResources>();
  ray_tracing_->scene = std::make_unique<RayTracingScene>(device_, allocator_, scene);
  ray_tracing_->camera = std::make_unique<RayTracingCamera>(allocator_, camera);
  ray_tracing_->frame_data = std::make_unique<RayTracingFrameData>(allocator_);
  ray_tracing_->output_image = std::make_unique<VulkanImage>(device_,
    allocator_,
    make_storage_image_create_info(extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
  ray_tracing_->accumulation_image = std::make_unique<VulkanImage>(
    device_, allocator_, make_storage_image_create_info(extent, VK_FORMAT_R32G32B32A32_SFLOAT));
  ray_tracing_->descriptors = std::make_unique<RayTracingDescriptorSet>(device_,
    ray_tracing_->scene->tlas(),
    *ray_tracing_->output_image,
    *ray_tracing_->accumulation_image,
    ray_tracing_->scene->vertex_buffer(),
    ray_tracing_->scene->material_index_buffer(),
    ray_tracing_->scene->material_buffer(),
    ray_tracing_->camera->buffer(),
    ray_tracing_->frame_data->buffer(),
    ray_tracing_->scene->light_buffer());

  const std::filesystem::path shader_dir{vulkan_rt::cmake::shader_dir};
  ray_tracing_->raygen_shader = std::make_unique<ShaderModule>(device_, shader_dir / "raygen_dome.rgen.spv");
  ray_tracing_->miss_shader = std::make_unique<ShaderModule>(device_, shader_dir / "miss.rmiss.spv");
  ray_tracing_->closest_hit_shader = std::make_unique<ShaderModule>(device_, shader_dir / "closesthit.rchit.spv");
  ray_tracing_->pipeline = std::make_unique<RayTracingPipeline>(device_,
    *ray_tracing_->raygen_shader,
    *ray_tracing_->miss_shader,
    *ray_tracing_->closest_hit_shader,
    ray_tracing_->descriptors->layout());
  ray_tracing_->sbt = std::make_unique<ShaderBindingTable>(device_, allocator_, *ray_tracing_->pipeline);
  output_image_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
  accumulation_image_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
  sample_index_ = 0;
}

void VulkanRenderer::destroy_ray_tracing_resources()
{
  ray_tracing_.reset();
  output_image_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
  accumulation_image_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
  sample_index_ = 0;
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
  imgui_.recreate_framebuffers(swapchain_);
  create_ray_tracing_resources(scene, camera);
}

void VulkanRenderer::trace_to_output_image(VkCommandBuffer command_buffer)
{
  if(!ray_tracing_ || !ray_tracing_->output_image || !ray_tracing_->accumulation_image || !ray_tracing_->pipeline ||
     !ray_tracing_->descriptors || !ray_tracing_->sbt)
  {
    throw std::runtime_error("Ray tracing resources are not initialized.");
  }

  const VkAccessFlags src_access =
    output_image_layout_ == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ? VK_ACCESS_TRANSFER_READ_BIT : 0;
  const VkPipelineStageFlags src_stage =
    output_image_layout_ == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT;

  transition_image_layout(command_buffer,
    ray_tracing_->output_image->image(),
    output_image_layout_,
    VK_IMAGE_LAYOUT_GENERAL,
    src_access,
    VK_ACCESS_SHADER_WRITE_BIT,
    src_stage,
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

  const VkAccessFlags accumulation_src_access =
    accumulation_image_layout_ == VK_IMAGE_LAYOUT_GENERAL ? (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT) : 0;
  const VkPipelineStageFlags accumulation_src_stage = accumulation_image_layout_ == VK_IMAGE_LAYOUT_UNDEFINED
                                                       ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                       : VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
  transition_image_layout(command_buffer,
    ray_tracing_->accumulation_image->image(),
    accumulation_image_layout_,
    VK_IMAGE_LAYOUT_GENERAL,
    accumulation_src_access,
    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    accumulation_src_stage,
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
  accumulation_image_layout_ = VK_IMAGE_LAYOUT_GENERAL;

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, ray_tracing_->pipeline->pipeline());
  const auto descriptor_set = ray_tracing_->descriptors->descriptor_set();
  vkCmdBindDescriptorSets(command_buffer,
    VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
    ray_tracing_->pipeline->layout(),
    0,
    1,
    &descriptor_set,
    0,
    nullptr);
  vkCmdTraceRaysKHR(command_buffer,
    &ray_tracing_->sbt->raygen_region(),
    &ray_tracing_->sbt->miss_region(),
    &ray_tracing_->sbt->hit_region(),
    &ray_tracing_->sbt->callable_region(),
    ray_tracing_->output_image->extent().width,
    ray_tracing_->output_image->extent().height,
    1);

  transition_image_layout(command_buffer,
    ray_tracing_->output_image->image(),
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
  copy_region.extent = extent_3d(ray_tracing_->output_image->extent());

  vkCmdCopyImage(command_buffer,
    ray_tracing_->output_image->image(),
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    swapchain_.images()[image_index],
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &copy_region);

  transition_image_layout(command_buffer,
    swapchain_.images()[image_index],
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

}// namespace vulkan_rt::render::vulkan
