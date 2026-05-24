/*****************************************************************
* \file   VulkanDevice.hpp
* \brief  VulkanDevice
* physical device selection
* logical device
* queues
* queue family indices
* feature/extension checks
*
* \author Shantanu Kumar
* \date   May 2026
*********************************************************************/
#pragma once

#include <volk.h>

#include "VulkanContext.hpp"
#include "VulkanRendererConfig.hpp"

#include <cstdint>
#include <optional>

namespace vulkan_rt::render::vulkan
{
struct QueueFamilyIndices
{
  std::optional<std::uint32_t> graphics;
  std::optional<std::uint32_t> present;

  [[nodiscard]] bool complete() const;
};

class VulkanDevice
{
public:
  VulkanDevice(const VulkanContext &context, const VulkanRendererConfig &config);
  ~VulkanDevice();

  VulkanDevice(const VulkanDevice &) = delete;
  VulkanDevice &operator=(const VulkanDevice &) = delete;

  VulkanDevice(VulkanDevice &&other) noexcept;
  VulkanDevice &operator=(VulkanDevice &&other) noexcept;

  void wait_idle() const;

  [[nodiscard]] VkPhysicalDevice physical_device() const;
  [[nodiscard]] VkDevice device() const;
  [[nodiscard]] VkQueue graphics_queue() const;
  [[nodiscard]] VkQueue present_queue() const;
  [[nodiscard]] const QueueFamilyIndices &queue_families() const;
  [[nodiscard]] const VkPhysicalDeviceProperties &properties() const;
  [[nodiscard]] const VkPhysicalDeviceRayTracingPipelinePropertiesKHR &ray_tracing_pipeline_properties() const;
  [[nodiscard]] const VkPhysicalDeviceAccelerationStructurePropertiesKHR &acceleration_structure_properties() const;

private:
  void pick_physical_device(const VulkanContext &context, const VulkanRendererConfig &config);
  void create_logical_device();
  void query_selected_device_properties();
  void destroy();

  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;
  QueueFamilyIndices queue_families_{};
  VkPhysicalDeviceProperties properties_{};
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties_{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties_{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
};
}
