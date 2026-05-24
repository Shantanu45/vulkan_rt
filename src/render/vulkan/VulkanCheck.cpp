#include "render/vulkan/VulkanCheck.hpp"

#include <volk.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace vulkan_rt::render::vulkan {
namespace {
  constexpr const char *ValidationLayerName = "VK_LAYER_KHRONOS_validation";

  std::string vk_result_to_string(VkResult result)
  {
    switch (result) {
    case VK_SUCCESS:
      return "VK_SUCCESS";
    case VK_NOT_READY:
      return "VK_NOT_READY";
    case VK_TIMEOUT:
      return "VK_TIMEOUT";
    case VK_EVENT_SET:
      return "VK_EVENT_SET";
    case VK_EVENT_RESET:
      return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
      return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
      return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
      return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
      return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return "VK_ERROR_INCOMPATIBLE_DRIVER";
    default:
      return "VkResult(" + std::to_string(static_cast<int>(result)) + ")";
    }
  }

  bool validation_layer_available()
  {
    uint32_t layer_count = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    if (result != VK_SUCCESS || layer_count == 0) { return false; }

    std::vector<VkLayerProperties> layers(layer_count);
    result = vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
    if (result != VK_SUCCESS) { return false; }

    return std::any_of(layers.begin(), layers.end(), [](const VkLayerProperties &layer) {
      return std::strcmp(layer.layerName, ValidationLayerName) == 0;
    });
  }
}// namespace

VulkanCheckResult check_vulkan(bool request_validation)
{
  VulkanCheckResult check;

  VkResult result = volkInitialize();
  if (result != VK_SUCCESS) {
    check.error = "Failed to initialize Vulkan loader via volk: " + vk_result_to_string(result);
    return check;
  }
  check.loader_present = true;

  check.validation_layer_available = validation_layer_available();
  if (request_validation && !check.validation_layer_available) {
    check.error = std::string{ "Requested validation layer is not available: " } + ValidationLayerName;
    return check;
  }

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "vulkan_rt";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 2);
  app_info.pEngineName = "vulkan_rt";
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 2);
  app_info.apiVersion = VK_API_VERSION_1_2;

  std::vector<const char *> enabled_layers;
  if (request_validation) { enabled_layers.push_back(ValidationLayerName); }

  VkInstanceCreateInfo instance_info = {};
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());
  instance_info.ppEnabledLayerNames = enabled_layers.empty() ? nullptr : enabled_layers.data();

  VkInstance instance = VK_NULL_HANDLE;
  result = vkCreateInstance(&instance_info, nullptr, &instance);
  if (result != VK_SUCCESS) {
    check.error = "Failed to create Vulkan instance: " + vk_result_to_string(result);
    return check;
  }
  check.instance_created = true;

  volkLoadInstance(instance);

  uint32_t physical_device_count = 0;
  result = vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
  if (result != VK_SUCCESS) {
    check.error = "Failed to enumerate Vulkan physical devices: " + vk_result_to_string(result);
    vkDestroyInstance(instance, nullptr);
    return check;
  }

  check.physical_device_count = physical_device_count;
  if (physical_device_count == 0) { check.error = "No Vulkan physical devices were found."; }

  vkDestroyInstance(instance, nullptr);
  return check;
}
}// namespace vulkan_rt::render::vulkan
