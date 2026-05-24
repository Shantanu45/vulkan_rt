#include "VulkanDevice.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace vulkan_rt::render::vulkan
{
namespace
{
constexpr std::array<const char *, 5> required_device_extensions{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
  VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
  VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
  VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
};

struct RayTracingFeatureSupport
{
  bool acceleration_structure = false;
  bool ray_tracing_pipeline = false;
  bool buffer_device_address = false;

  [[nodiscard]] bool complete() const
  {
    return acceleration_structure && ray_tracing_pipeline && buffer_device_address;
  }
};

struct DeviceSuitability
{
  QueueFamilyIndices queue_families;
  std::vector<std::string> missing_extensions;
  RayTracingFeatureSupport features;

  [[nodiscard]] bool suitable() const
  {
    return queue_families.complete() && missing_extensions.empty() && features.complete();
  }

  [[nodiscard]] std::string describe_failures() const
  {
    std::vector<std::string> reasons;

    if(!queue_families.graphics.has_value())
    {
      reasons.emplace_back("missing graphics queue");
    }

    if(!queue_families.present.has_value())
    {
      reasons.emplace_back("missing present queue");
    }

    for(const auto &extension : missing_extensions)
    {
      reasons.push_back("missing device extension " + extension);
    }

    if(!features.acceleration_structure)
    {
      reasons.emplace_back("missing accelerationStructure feature");
    }

    if(!features.ray_tracing_pipeline)
    {
      reasons.emplace_back("missing rayTracingPipeline feature");
    }

    if(!features.buffer_device_address)
    {
      reasons.emplace_back("missing bufferDeviceAddress feature");
    }

    std::string description;
    for(std::size_t index = 0; index < reasons.size(); ++index)
    {
      if(index > 0)
      {
        description += "; ";
      }
      description += reasons[index];
    }

    return description.empty() ? "unknown reason" : description;
  }
};

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

QueueFamilyIndices find_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  QueueFamilyIndices indices;

  std::uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  for(std::uint32_t index = 0; index < queue_families.size(); ++index)
  {
    if((queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
    {
      indices.graphics = index;
    }

    VkBool32 present_supported = VK_FALSE;
    throw_if_failed(
      vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface, &present_supported),
      "vkGetPhysicalDeviceSurfaceSupportKHR");

    if(present_supported == VK_TRUE)
    {
      indices.present = index;
    }

    if(indices.complete())
    {
      break;
    }
  }

  return indices;
}

std::vector<std::string> missing_required_extensions(VkPhysicalDevice physical_device)
{
  std::uint32_t extension_count = 0;
  throw_if_failed(
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr),
    "vkEnumerateDeviceExtensionProperties");

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  throw_if_failed(
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data()),
    "vkEnumerateDeviceExtensionProperties");

  std::vector<std::string> missing_extensions;
  for(const char *required : required_device_extensions)
  {
    const bool found = std::any_of(available_extensions.begin(), available_extensions.end(), [&](const auto &available) {
      return std::strcmp(available.extensionName, required) == 0;
    });

    if(!found)
    {
      missing_extensions.emplace_back(required);
    }
  }

  return missing_extensions;
}

RayTracingFeatureSupport query_ray_tracing_feature_support(VkPhysicalDevice physical_device)
{
  VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{};
  buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

  VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};
  acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
  acceleration_structure_features.pNext = &buffer_device_address_features;

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features{};
  ray_tracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
  ray_tracing_pipeline_features.pNext = &acceleration_structure_features;

  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &ray_tracing_pipeline_features;

  vkGetPhysicalDeviceFeatures2(physical_device, &features);

  return RayTracingFeatureSupport{
    .acceleration_structure = acceleration_structure_features.accelerationStructure == VK_TRUE,
    .ray_tracing_pipeline = ray_tracing_pipeline_features.rayTracingPipeline == VK_TRUE,
    .buffer_device_address = buffer_device_address_features.bufferDeviceAddress == VK_TRUE,
  };
}

DeviceSuitability query_device_suitability(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  return DeviceSuitability{
    .queue_families = find_queue_families(physical_device, surface),
    .missing_extensions = missing_required_extensions(physical_device),
    .features = query_ray_tracing_feature_support(physical_device),
  };
}

int device_score(VkPhysicalDevice physical_device)
{
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physical_device, &properties);

  if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
  {
    return 1000;
  }

  if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
  {
    return 500;
  }

  return 100;
}

std::string device_name(VkPhysicalDevice physical_device)
{
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physical_device, &properties);
  return properties.deviceName;
}
}

bool QueueFamilyIndices::complete() const
{
  return graphics.has_value() && present.has_value();
}

VulkanDevice::VulkanDevice(const VulkanContext &context, const VulkanRendererConfig &config)
{
  pick_physical_device(context, config);
  create_logical_device();
  volkLoadDevice(device_);
}

VulkanDevice::~VulkanDevice()
{
  destroy();
}

VulkanDevice::VulkanDevice(VulkanDevice &&other) noexcept
  : physical_device_(other.physical_device_)
  , device_(other.device_)
  , graphics_queue_(other.graphics_queue_)
  , present_queue_(other.present_queue_)
  , queue_families_(other.queue_families_)
  , properties_(other.properties_)
  , ray_tracing_pipeline_properties_(other.ray_tracing_pipeline_properties_)
  , acceleration_structure_properties_(other.acceleration_structure_properties_)
{
  other.physical_device_ = VK_NULL_HANDLE;
  other.device_ = VK_NULL_HANDLE;
  other.graphics_queue_ = VK_NULL_HANDLE;
  other.present_queue_ = VK_NULL_HANDLE;
  other.queue_families_ = {};
  other.properties_ = {};
  other.ray_tracing_pipeline_properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  other.acceleration_structure_properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
}

VulkanDevice &VulkanDevice::operator=(VulkanDevice &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    physical_device_ = other.physical_device_;
    device_ = other.device_;
    graphics_queue_ = other.graphics_queue_;
    present_queue_ = other.present_queue_;
    queue_families_ = other.queue_families_;
    properties_ = other.properties_;
    ray_tracing_pipeline_properties_ = other.ray_tracing_pipeline_properties_;
    acceleration_structure_properties_ = other.acceleration_structure_properties_;

    other.physical_device_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
    other.graphics_queue_ = VK_NULL_HANDLE;
    other.present_queue_ = VK_NULL_HANDLE;
    other.queue_families_ = {};
    other.properties_ = {};
    other.ray_tracing_pipeline_properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    other.acceleration_structure_properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
  }

  return *this;
}

void VulkanDevice::wait_idle() const
{
  if(device_ != VK_NULL_HANDLE)
  {
    throw_if_failed(vkDeviceWaitIdle(device_), "vkDeviceWaitIdle");
  }
}

VkPhysicalDevice VulkanDevice::physical_device() const
{
  return physical_device_;
}

VkDevice VulkanDevice::device() const
{
  return device_;
}

VkQueue VulkanDevice::graphics_queue() const
{
  return graphics_queue_;
}

VkQueue VulkanDevice::present_queue() const
{
  return present_queue_;
}

const QueueFamilyIndices &VulkanDevice::queue_families() const
{
  return queue_families_;
}

const VkPhysicalDeviceProperties &VulkanDevice::properties() const
{
  return properties_;
}

const VkPhysicalDeviceRayTracingPipelinePropertiesKHR &VulkanDevice::ray_tracing_pipeline_properties() const
{
  return ray_tracing_pipeline_properties_;
}

const VkPhysicalDeviceAccelerationStructurePropertiesKHR &VulkanDevice::acceleration_structure_properties() const
{
  return acceleration_structure_properties_;
}

void VulkanDevice::pick_physical_device(const VulkanContext &context, const VulkanRendererConfig &config)
{
  std::uint32_t device_count = 0;
  throw_if_failed(vkEnumeratePhysicalDevices(context.instance(), &device_count, nullptr), "vkEnumeratePhysicalDevices");

  if(device_count == 0)
  {
    throw std::runtime_error("No Vulkan physical devices found.");
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  throw_if_failed(vkEnumeratePhysicalDevices(context.instance(), &device_count, devices.data()), "vkEnumeratePhysicalDevices");

  if(config.gpu_index >= 0)
  {
    const auto requested_index = static_cast<std::size_t>(config.gpu_index);
    if(requested_index >= devices.size())
    {
      throw std::runtime_error("Requested Vulkan GPU index is out of range.");
    }

    const auto suitability = query_device_suitability(devices[requested_index], context.surface());
    if(!suitability.suitable())
    {
      throw std::runtime_error(
        "Requested Vulkan GPU '" + device_name(devices[requested_index]) +
        "' does not support required Vulkan ray tracing capabilities: " + suitability.describe_failures() + ".");
    }

    physical_device_ = devices[requested_index];
  }
  else
  {
    int best_score = -1;
    for(const auto candidate : devices)
    {
      const auto suitability = query_device_suitability(candidate, context.surface());
      if(!suitability.suitable())
      {
        continue;
      }

      const int score = device_score(candidate);
      if(score > best_score)
      {
        best_score = score;
        physical_device_ = candidate;
      }
    }
  }

  if(physical_device_ == VK_NULL_HANDLE)
  {
    std::string message = "No suitable Vulkan physical device found with required ray tracing capabilities.";
    for(const auto candidate : devices)
    {
      const auto suitability = query_device_suitability(candidate, context.surface());
      message += "\n  ";
      message += device_name(candidate);
      message += ": ";
      message += suitability.describe_failures();
    }
    throw std::runtime_error(message);
  }

  queue_families_ = find_queue_families(physical_device_, context.surface());
  query_selected_device_properties();
}

void VulkanDevice::create_logical_device()
{
  if(!queue_families_.complete())
  {
    throw std::runtime_error("Cannot create Vulkan device without complete queue family indices.");
  }

  constexpr float queue_priority = 1.0F;
  std::set<std::uint32_t> unique_queue_families{
    queue_families_.graphics.value(),
    queue_families_.present.value(),
  };

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  queue_create_infos.reserve(unique_queue_families.size());

  for(const auto queue_family : unique_queue_families)
  {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{};
  buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
  buffer_device_address_features.bufferDeviceAddress = VK_TRUE;

  VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};
  acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
  acceleration_structure_features.pNext = &buffer_device_address_features;
  acceleration_structure_features.accelerationStructure = VK_TRUE;

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features{};
  ray_tracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
  ray_tracing_pipeline_features.pNext = &acceleration_structure_features;
  ray_tracing_pipeline_features.rayTracingPipeline = VK_TRUE;

  VkPhysicalDeviceFeatures2 enabled_features{};
  enabled_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  enabled_features.pNext = &ray_tracing_pipeline_features;

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.enabledExtensionCount = static_cast<std::uint32_t>(required_device_extensions.size());
  create_info.ppEnabledExtensionNames = required_device_extensions.data();
  create_info.pNext = &enabled_features;

  throw_if_failed(vkCreateDevice(physical_device_, &create_info, nullptr, &device_), "vkCreateDevice");

  vkGetDeviceQueue(device_, queue_families_.graphics.value(), 0, &graphics_queue_);
  vkGetDeviceQueue(device_, queue_families_.present.value(), 0, &present_queue_);
}

void VulkanDevice::query_selected_device_properties()
{
  ray_tracing_pipeline_properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  acceleration_structure_properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
  ray_tracing_pipeline_properties_.pNext = &acceleration_structure_properties_;

  VkPhysicalDeviceProperties2 properties{};
  properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  properties.pNext = &ray_tracing_pipeline_properties_;

  vkGetPhysicalDeviceProperties2(physical_device_, &properties);
  properties_ = properties.properties;

  ray_tracing_pipeline_properties_.pNext = nullptr;
}

void VulkanDevice::destroy()
{
  if(device_ != VK_NULL_HANDLE)
  {
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }

  physical_device_ = VK_NULL_HANDLE;
  graphics_queue_ = VK_NULL_HANDLE;
  present_queue_ = VK_NULL_HANDLE;
  queue_families_ = {};
  properties_ = {};
  ray_tracing_pipeline_properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  acceleration_structure_properties_ = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
}
}
