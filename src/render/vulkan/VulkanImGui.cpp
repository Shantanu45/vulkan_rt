#include "VulkanImGui.hpp"

#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <array>
#include <stdexcept>
#include <string>

namespace vulkan_rt::render::vulkan
{
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
}

VulkanImGui::VulkanImGui(
  const VulkanContext &context,
  const VulkanDevice &device,
  const VulkanSwapchain &swapchain,
  std::uint32_t frames_in_flight)
  : device_(device.device())
{
  create_descriptor_pool();
  create_render_pass(swapchain.image_format());
  create_framebuffers(swapchain);
  initialize_backend(context, device, swapchain, frames_in_flight);
}

VulkanImGui::~VulkanImGui()
{
  destroy();
}

void VulkanImGui::recreate_framebuffers(const VulkanSwapchain &swapchain)
{
  destroy_framebuffers();
  create_framebuffers(swapchain);
}

bool VulkanImGui::render(VkCommandBuffer command_buffer, std::uint32_t image_index, const VulkanSwapchain &swapchain)
{
  if(ImGui::GetCurrentContext() == nullptr)
  {
    return false;
  }

  auto *draw_data = ImGui::GetDrawData();
  if(draw_data == nullptr)
  {
    return false;
  }

  VkRenderPassBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  begin_info.renderPass = render_pass_;
  begin_info.framebuffer = framebuffers_[image_index];
  begin_info.renderArea.offset = VkOffset2D{.x = 0, .y = 0};
  begin_info.renderArea.extent = swapchain.extent();

  vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
  if(draw_data->TotalVtxCount > 0)
  {
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
  }
  vkCmdEndRenderPass(command_buffer);
  return true;
}

void VulkanImGui::create_descriptor_pool()
{
  std::array<VkDescriptorPoolSize, 1> pool_sizes{};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[0].descriptorCount = 64;

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 64;
  pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();
  throw_if_failed(vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_), "vkCreateDescriptorPool");
}

void VulkanImGui::create_render_pass(VkFormat swapchain_format)
{
  VkAttachmentDescription color_attachment{};
  color_attachment.format = swapchain_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_reference{};
  color_reference.attachment = 0;
  color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_reference;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  throw_if_failed(vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_), "vkCreateRenderPass");
}

void VulkanImGui::create_framebuffers(const VulkanSwapchain &swapchain)
{
  const auto image_views = swapchain.image_views();
  framebuffers_.resize(image_views.size());
  for(std::size_t i = 0; i < image_views.size(); ++i)
  {
    const VkImageView attachments[] = {image_views[i]};
    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = swapchain.extent().width;
    framebuffer_info.height = swapchain.extent().height;
    framebuffer_info.layers = 1;
    throw_if_failed(
      vkCreateFramebuffer(device_, &framebuffer_info, nullptr, &framebuffers_[i]), "vkCreateFramebuffer");
  }
}

void VulkanImGui::initialize_backend(
  const VulkanContext &context,
  const VulkanDevice &device,
  const VulkanSwapchain &swapchain,
  std::uint32_t frames_in_flight)
{
  ImGui_ImplVulkan_InitInfo init_info{};
  init_info.Instance = context.instance();
  init_info.PhysicalDevice = device.physical_device();
  init_info.Device = device.device();
  init_info.QueueFamily = device.queue_families().graphics.value();
  init_info.Queue = device.graphics_queue();
  init_info.DescriptorPool = descriptor_pool_;
  init_info.ApiVersion = VK_API_VERSION_1_3;
  init_info.PipelineInfoMain.RenderPass = render_pass_;
  init_info.PipelineInfoMain.Subpass = 0;
  init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.MinImageCount = frames_in_flight;
  init_info.ImageCount = static_cast<std::uint32_t>(swapchain.images().size());
  ImGui_ImplVulkan_Init(&init_info);
}

void VulkanImGui::destroy_framebuffers()
{
  for(const auto framebuffer : framebuffers_)
  {
    vkDestroyFramebuffer(device_, framebuffer, nullptr);
  }
  framebuffers_.clear();
}

void VulkanImGui::destroy()
{
  if(device_ == VK_NULL_HANDLE)
  {
    return;
  }

  ImGui_ImplVulkan_Shutdown();
  destroy_framebuffers();
  if(render_pass_ != VK_NULL_HANDLE)
  {
    vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }
  if(descriptor_pool_ != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
    descriptor_pool_ = VK_NULL_HANDLE;
  }
  device_ = VK_NULL_HANDLE;
}
}
