#include "RayTracingPipeline.hpp"

#include <array>
#include <cstdint>
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

RayTracingPipeline::RayTracingPipeline(
  const VulkanDevice &device,
  const ShaderModule &raygen_shader,
  const ShaderModule &miss_shader,
  const ShaderModule &closest_hit_shader)
{
  try
  {
    create(device, raygen_shader, miss_shader, closest_hit_shader);
  }
  catch(...)
  {
    destroy();
    throw;
  }
}

RayTracingPipeline::~RayTracingPipeline()
{
  destroy();
}

RayTracingPipeline::RayTracingPipeline(RayTracingPipeline &&other) noexcept
  : device_(other.device_)
  , layout_(other.layout_)
  , pipeline_(other.pipeline_)
  , shader_group_count_(other.shader_group_count_)
{
  other.device_ = VK_NULL_HANDLE;
  other.layout_ = VK_NULL_HANDLE;
  other.pipeline_ = VK_NULL_HANDLE;
  other.shader_group_count_ = 0;
}

RayTracingPipeline &RayTracingPipeline::operator=(RayTracingPipeline &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    device_ = other.device_;
    layout_ = other.layout_;
    pipeline_ = other.pipeline_;
    shader_group_count_ = other.shader_group_count_;

    other.device_ = VK_NULL_HANDLE;
    other.layout_ = VK_NULL_HANDLE;
    other.pipeline_ = VK_NULL_HANDLE;
    other.shader_group_count_ = 0;
  }

  return *this;
}

VkPipeline RayTracingPipeline::pipeline() const
{
  return pipeline_;
}

VkPipelineLayout RayTracingPipeline::layout() const
{
  return layout_;
}

std::uint32_t RayTracingPipeline::shader_group_count() const
{
  return shader_group_count_;
}

void RayTracingPipeline::create(
  const VulkanDevice &device,
  const ShaderModule &raygen_shader,
  const ShaderModule &miss_shader,
  const ShaderModule &closest_hit_shader)
{
  device_ = device.device();

  VkPipelineLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  throw_if_failed(vkCreatePipelineLayout(device.device(), &layout_info, nullptr, &layout_), "vkCreatePipelineLayout");

  constexpr std::uint32_t raygen_stage_index = 0;
  constexpr std::uint32_t miss_stage_index = 1;
  constexpr std::uint32_t closest_hit_stage_index = 2;

  std::array<VkPipelineShaderStageCreateInfo, 3> stages{};
  stages[raygen_stage_index].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[raygen_stage_index].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  stages[raygen_stage_index].module = raygen_shader.module();
  stages[raygen_stage_index].pName = "main";

  stages[miss_stage_index].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[miss_stage_index].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
  stages[miss_stage_index].module = miss_shader.module();
  stages[miss_stage_index].pName = "main";

  stages[closest_hit_stage_index].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[closest_hit_stage_index].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  stages[closest_hit_stage_index].module = closest_hit_shader.module();
  stages[closest_hit_stage_index].pName = "main";

  std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> groups{};
  groups[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  groups[0].generalShader = raygen_stage_index;
  groups[0].closestHitShader = VK_SHADER_UNUSED_KHR;
  groups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
  groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

  groups[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  groups[1].generalShader = miss_stage_index;
  groups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
  groups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
  groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

  groups[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  groups[2].generalShader = VK_SHADER_UNUSED_KHR;
  groups[2].closestHitShader = closest_hit_stage_index;
  groups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
  groups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

  VkRayTracingPipelineCreateInfoKHR pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  pipeline_info.stageCount = static_cast<std::uint32_t>(stages.size());
  pipeline_info.pStages = stages.data();
  pipeline_info.groupCount = static_cast<std::uint32_t>(groups.size());
  pipeline_info.pGroups = groups.data();
  pipeline_info.maxPipelineRayRecursionDepth = 1;
  pipeline_info.layout = layout_;

  throw_if_failed(
    vkCreateRayTracingPipelinesKHR(
      device.device(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_),
    "vkCreateRayTracingPipelinesKHR");

  shader_group_count_ = static_cast<std::uint32_t>(groups.size());
}

void RayTracingPipeline::destroy()
{
  if(pipeline_ != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(device_, pipeline_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
  }

  if(layout_ != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device_, layout_, nullptr);
    layout_ = VK_NULL_HANDLE;
  }

  device_ = VK_NULL_HANDLE;
  shader_group_count_ = 0;
}
}
