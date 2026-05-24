#include "VulkanContext.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace vulkan_rt::render::vulkan
{
namespace
{
constexpr const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
  VkDebugUtilsMessageTypeFlagsEXT message_type,
  const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
  void *user_data)
{
  static_cast<void>(message_severity);
  static_cast<void>(message_type);
  static_cast<void>(user_data);

  if(callback_data && callback_data->pMessage)
  {
    std::fprintf(stderr, "Vulkan validation: %s\n", callback_data->pMessage);
  }

  return VK_FALSE;
}

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

bool validation_layer_available()
{
  std::uint32_t layer_count = 0;
  throw_if_failed(vkEnumerateInstanceLayerProperties(&layer_count, nullptr), "vkEnumerateInstanceLayerProperties");

  std::vector<VkLayerProperties> layers(layer_count);
  throw_if_failed(vkEnumerateInstanceLayerProperties(&layer_count, layers.data()), "vkEnumerateInstanceLayerProperties");

  return std::any_of(layers.begin(), layers.end(), [](const VkLayerProperties &layer) {
    return std::strcmp(layer.layerName, validation_layer_name) == 0;
  });
}

bool contains_extension(const std::vector<const char *> &extensions, const char *extension)
{
  return std::any_of(extensions.begin(), extensions.end(), [extension](const char *candidate) {
    return std::strcmp(candidate, extension) == 0;
  });
}

VkDebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info()
{
  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debug_callback;
  return create_info;
}
}

VulkanContext::VulkanContext(const VulkanRendererConfig &config, const SurfaceProvider &surface_provider)
{
  throw_if_failed(volkInitialize(), "volkInitialize");

  validation_enabled_ = config.validation;
  if(validation_enabled_ && !validation_layer_available())
  {
    throw std::runtime_error("Validation layer requested but VK_LAYER_KHRONOS_validation is not available.");
  }

  const auto required_extensions = surface_provider.required_vulkan_instance_extensions();
  create_instance(config, std::span<const char *const>{required_extensions.data(), required_extensions.size()});
  volkLoadInstance(instance_);

  if(validation_enabled_)
  {
    create_debug_messenger();
  }

  create_surface(surface_provider);
}

VulkanContext::~VulkanContext()
{
  destroy();
}

VulkanContext::VulkanContext(VulkanContext &&other) noexcept
  : instance_(other.instance_)
  , debug_messenger_(other.debug_messenger_)
  , surface_(other.surface_)
  , validation_enabled_(other.validation_enabled_)
{
  other.instance_ = VK_NULL_HANDLE;
  other.debug_messenger_ = VK_NULL_HANDLE;
  other.surface_ = VK_NULL_HANDLE;
  other.validation_enabled_ = false;
}

VulkanContext &VulkanContext::operator=(VulkanContext &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    instance_ = other.instance_;
    debug_messenger_ = other.debug_messenger_;
    surface_ = other.surface_;
    validation_enabled_ = other.validation_enabled_;

    other.instance_ = VK_NULL_HANDLE;
    other.debug_messenger_ = VK_NULL_HANDLE;
    other.surface_ = VK_NULL_HANDLE;
    other.validation_enabled_ = false;
  }

  return *this;
}

VkInstance VulkanContext::instance() const
{
  return instance_;
}

VkSurfaceKHR VulkanContext::surface() const
{
  return surface_;
}

bool VulkanContext::validation_enabled() const
{
  return validation_enabled_;
}

void VulkanContext::create_instance(
  const VulkanRendererConfig &config, std::span<const char *const> required_extensions)
{
  std::vector<const char *> enabled_extensions(required_extensions.begin(), required_extensions.end());

  if(validation_enabled_ && !contains_extension(enabled_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
  {
    enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  std::vector<const char *> enabled_layers;
  if(validation_enabled_)
  {
    enabled_layers.push_back(validation_layer_name);
  }

  const char *application_name = config.application_name ? config.application_name : "vulkan_rt";

  VkApplicationInfo application_info{};
  application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  application_info.pApplicationName = application_name;
  application_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  application_info.pEngineName = "vulkan_rt";
  application_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  application_info.apiVersion = VK_API_VERSION_1_3;

  auto debug_create_info = make_debug_messenger_create_info();

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &application_info;
  create_info.enabledExtensionCount = static_cast<std::uint32_t>(enabled_extensions.size());
  create_info.ppEnabledExtensionNames = enabled_extensions.data();
  create_info.enabledLayerCount = static_cast<std::uint32_t>(enabled_layers.size());
  create_info.ppEnabledLayerNames = enabled_layers.empty() ? nullptr : enabled_layers.data();
  create_info.pNext = validation_enabled_ ? &debug_create_info : nullptr;

  throw_if_failed(vkCreateInstance(&create_info, nullptr, &instance_), "vkCreateInstance");
}

void VulkanContext::create_debug_messenger()
{
  const auto create_debug_messenger =
    reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));

  if(!create_debug_messenger)
  {
    throw std::runtime_error("VK_EXT_debug_utils was enabled but vkCreateDebugUtilsMessengerEXT is unavailable.");
  }

  auto create_info = make_debug_messenger_create_info();
  throw_if_failed(
    create_debug_messenger(instance_, &create_info, nullptr, &debug_messenger_), "vkCreateDebugUtilsMessengerEXT");
}

void VulkanContext::create_surface(const SurfaceProvider &surface_provider)
{
  surface_ = surface_provider.create_vulkan_surface(instance_);
  if(surface_ == VK_NULL_HANDLE)
  {
    throw std::runtime_error("SurfaceProvider returned VK_NULL_HANDLE.");
  }
}

void VulkanContext::destroy()
{
  if(instance_ == VK_NULL_HANDLE)
  {
    return;
  }

  if(surface_ != VK_NULL_HANDLE)
  {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }

  if(debug_messenger_ != VK_NULL_HANDLE)
  {
    const auto destroy_debug_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
    if(destroy_debug_messenger)
    {
      destroy_debug_messenger(instance_, debug_messenger_, nullptr);
    }
    debug_messenger_ = VK_NULL_HANDLE;
  }

  vkDestroyInstance(instance_, nullptr);
  instance_ = VK_NULL_HANDLE;
  validation_enabled_ = false;
}
}
