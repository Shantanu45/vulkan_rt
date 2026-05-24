#include "VulkanCheck.hpp"
#include "SDL3/SDL_vulkan.h"
#include <vector>
#include <array>
#include <string>
#include "libassert/assert.hpp"
#include <set>

#define VULKAN_DEBUG

namespace vulkan_rt::render::vulkan
{
struct Vendor
{
  constexpr static uint32_t VENDOR_UNKNOWN = 0x0;
  constexpr static uint32_t VENDOR_AMD = 0x1002;
  constexpr static uint32_t VENDOR_IMGTEC = 0x1010;
  constexpr static uint32_t VENDOR_APPLE = 0x106B;
  constexpr static uint32_t VENDOR_NVIDIA = 0x10DE;
  constexpr static uint32_t VENDOR_ARM = 0x13B5;
  constexpr static uint32_t VENDOR_MICROSOFT = 0x1414;
  constexpr static uint32_t VENDOR_QUALCOMM = 0x5143;
  constexpr static uint32_t VENDOR_INTEL = 0x8086;
};

enum DeviceType {
  DEVICE_TYPE_OTHER = 0x0,
  DEVICE_TYPE_INTEGRATED_GPU = 0x1,
  DEVICE_TYPE_DISCRETE_GPU = 0x2,
  DEVICE_TYPE_VIRTUAL_GPU = 0x3,
  DEVICE_TYPE_CPU = 0x4,
  DEVICE_TYPE_MAX = 0x5
};

struct Device
{
  std::string name = "Unknown";
  uint32_t vendor = Vendor::VENDOR_UNKNOWN;
  DeviceType type = DEVICE_TYPE_OTHER;
};

struct DeviceQueueFamilies
{
  std::vector<VkQueueFamilyProperties> properties;
};
uint32_t instance_api_version = VK_API_VERSION_1_0;
std::unordered_map<std::string, bool> requested_instance_extensions;
std::set<std::string> enabled_instance_extension_names;

std::vector<Device> driver_devices;
std::vector<VkPhysicalDevice> physical_devices;
VkInstance instance = VK_NULL_HANDLE;
std::vector<DeviceQueueFamilies> device_queue_families;

VulkanCheckResult check_result{};

Error _initialize_vulkan_version()
{
  // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkApplicationInfo.html#_description
  // For Vulkan 1.0 vkEnumerateInstanceVersion is not available, including not in the loader we compile against on
  // Android.
  typedef VkResult(VKAPI_PTR * _vkEnumerateInstanceVersion)(uint32_t *);
  _vkEnumerateInstanceVersion func =
    (_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion");
  if (func != nullptr) {
    uint32_t api_version;
    VkResult res = func(&api_version);
    if (res == VK_SUCCESS) {
      instance_api_version = api_version;
    } else {
      // According to the documentation this shouldn't fail with anything except a memory allocation error
      // in which case we're in deep trouble anyway.
      ERR_FAIL_V(ERR_CANT_CREATE);
    }
  } else {
    LOGI("vkEnumerateInstanceVersion not available, assuming Vulkan 1.0.");
    instance_api_version = VK_API_VERSION_1_0;
  }

  return OK;
}

Error _initializee_volk()
{
  if (volkInitialize() != VK_SUCCESS) {
    LOGE("Failed to initialize Vulkan loader via volk.");
    return ERR_CANT_CREATE;
  }
  check_result.loader_present = true;
  return OK;
}

void _fill_reqiured_instance_extensions()
{
  std::array extenstions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,     // This extension allows us to use the properties2 features to query additional device capabilities.
    VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,        // This extension allows us to use colorspaces other than SRGB.
#ifdef VULKAN_DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif  // VULKAN DEBUG
  };
  for (auto& extenstion: extenstions) {
      requested_instance_extensions[extenstion] = true;
  }
}

Error _validate_reqired_instance_extensions()
{
  _fill_reqiured_instance_extensions();

  DEBUG_ASSERT(!requested_instance_extensions.empty());
  uint32_t instance_extension_count = 0;
  VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);
  ERR_FAIL_COND_V(result != VK_SUCCESS && result != VK_INCOMPLETE, ERR_CANT_CREATE);
  ERR_FAIL_COND_V_MSG(instance_extension_count == 0, ERR_CANT_CREATE, "No instance extensions were found.");

  std::vector<VkExtensionProperties> instance_extensions;
  instance_extensions.resize(instance_extension_count);
  result = vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.data());
  if (result != VK_SUCCESS && result != VK_INCOMPLETE) { ERR_FAIL_V(ERR_CANT_CREATE); }

  for (const auto& instance_extension: instance_extensions)
  {
    if (requested_instance_extensions.contains(instance_extension.extensionName)) { 
        enabled_instance_extension_names.insert(instance_extension.extensionName);
      }
  }

  // Now check our requested extensions.
  for (std::pair<std::string, bool> requested_extension : requested_instance_extensions) {
    if (!enabled_instance_extension_names.contains(requested_extension.first)) {
      if (requested_extension.second) {
        std::string msg = "Required extension " + requested_extension.first + " not found.";
        ERR_FAIL_V_MSG(ERR_BUG, msg.c_str());
      } else {
        LOGI("Optional extension {} not found.", requested_extension.first);
      }
    }
  }
  return OK;
}

Error _create_vulkan_instance(const VkInstanceCreateInfo *p_create_info,
  VkInstance *r_instance)
{

  VkResult err = vkCreateInstance(p_create_info, nullptr, r_instance);
  ERR_FAIL_COND_V_MSG(err == VK_ERROR_INCOMPATIBLE_DRIVER,
    ERR_CANT_CREATE,
    "Cannot find a compatible Vulkan installable client driver (ICD).\n\n"
    "vkCreateInstance Failure");
  ERR_FAIL_COND_V_MSG(err == VK_ERROR_EXTENSION_NOT_PRESENT,
    ERR_CANT_CREATE,
    "Cannot find a specified extension library.\n"
    "Make sure your layers path is set appropriately.\n"
    "vkCreateInstance Failure");
  ERR_FAIL_COND_V_MSG(err,
    ERR_CANT_CREATE,
    "vkCreateInstance failed.\n\n"
    "Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
    "Please look at the Getting Started guide for additional information.\n"
    "vkCreateInstance Failure");

  return OK;
}

Error _find_validation_layers(std::vector<const char *> &r_layer_names)
{
  r_layer_names.clear();

  uint32_t instance_layer_count = 0;
  VkResult err = vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
  ERR_FAIL_COND_V(err != VK_SUCCESS, ERR_CANT_CREATE);
  if (instance_layer_count > 0) {
    std::vector<VkLayerProperties> layer_properties;
    layer_properties.resize(instance_layer_count);
    err = vkEnumerateInstanceLayerProperties(&instance_layer_count, layer_properties.data());
    ERR_FAIL_COND_V(err != VK_SUCCESS, ERR_CANT_CREATE);

    // Preferred set of validation layers.
    const std::initializer_list<const char *> preferred = { "VK_LAYER_KHRONOS_validation" };

    // Alternative (deprecated, removed in SDK 1.1.126.0) set of validation layers.
    const std::initializer_list<const char *> lunarg = { "VK_LAYER_LUNARG_standard_validation" };

    // Alternative (deprecated, removed in SDK 1.1.121.1) set of validation layers.
    const std::initializer_list<const char *> google = { "VK_LAYER_GOOGLE_threading",
      "VK_LAYER_LUNARG_parameter_validation",
      "VK_LAYER_LUNARG_object_tracker",
      "VK_LAYER_LUNARG_core_validation",
      "VK_LAYER_GOOGLE_unique_objects" };

    // Verify all the layers of the list are present.
    for (const std::initializer_list<const char *> &list : { preferred, lunarg, google }) {
      bool layers_found = false;
      for (const char *layer_name : list) {
        layers_found = false;

        for (const VkLayerProperties &properties : layer_properties) {
          if (!strcmp(properties.layerName, layer_name)) {
            layers_found = true;
            break;
          }
        }

        if (!layers_found) { break; }
      }

      if (layers_found) {
        r_layer_names.reserve(list.size());
        for (const char *layer_name : list) { r_layer_names.push_back(layer_name); }

        break;
      }
    }
  }

  return OK;
}

Error _initialize_vulkan_instance()
{
  [[maybe_unused]] Error err;
  std::vector<const char *> enabled_extension_names;
  enabled_extension_names.reserve(enabled_instance_extension_names.size());
  for (const std::string &extension_name : enabled_instance_extension_names) {
    enabled_extension_names.push_back(extension_name.data());
  }

  std::vector<const char *> enabled_layer_names;
  
  #ifdef VULKAN_DEBUG
    err = _find_validation_layers(enabled_layer_names);
  if (err == OK) {
    check_result.validation_layer_available = true;
  } else {
    ERR_FAIL_COND_V(err != OK, err);
  }
  #endif //VULKAN_DEBUG

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    instance_info.pApplicationInfo = nullptr;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extension_names.size());
    instance_info.ppEnabledExtensionNames = enabled_extension_names.data();
    instance_info.enabledLayerCount = static_cast<uint32_t>(enabled_layer_names.size());
    instance_info.ppEnabledLayerNames = enabled_layer_names.data();

    // This is info for a temp callback to use during CreateInstance. After the instance is created, we use the
    // instance-based function to register the final callback.
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {};
    VkDebugReportCallbackCreateInfoEXT debug_report_callback_create_info = {};

    if (instance == VK_NULL_HANDLE) {
      if (_create_vulkan_instance(&instance_info, &instance) != OK) return FAILED;
    }

	volkLoadInstance(instance);
    check_result.instance_created = true;
    return OK;
}

Error _check_devices()
{
  uint32_t physical_device_count = 0;
  VkResult err = vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
  ERR_FAIL_COND_V(err != VK_SUCCESS, ERR_CANT_CREATE);
  ERR_FAIL_COND_V_MSG(physical_device_count == 0,
    ERR_CANT_CREATE,
    "vkEnumeratePhysicalDevices reported zero accessible devices.\n\nDo you have a compatible Vulkan installable "
    "client driver (ICD) installed?\nvkEnumeratePhysicalDevices Failure.");

  driver_devices.resize(physical_device_count);
  physical_devices.resize(physical_device_count);
  device_queue_families.resize(physical_device_count);
  err = vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());
  ERR_FAIL_COND_V(err != VK_SUCCESS, ERR_CANT_CREATE);

  // Fill the list of driver devices with the properties from the physical devices.
  for (uint32_t i = 0; i < physical_devices.size(); i++) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_devices[i], &props);

    Device &driver_device = driver_devices[i];
    driver_device.name = std::string(props.deviceName);
    driver_device.vendor = props.vendorID;
    driver_device.type = DeviceType(props.deviceType);

    // TODO:
    //_check_driver_workarounds(props, driver_device);

    uint32_t queue_family_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_properties_count, nullptr);

    if (queue_family_properties_count > 0) {
      device_queue_families[i].properties.resize(queue_family_properties_count);
      vkGetPhysicalDeviceQueueFamilyProperties(
        physical_devices[i], &queue_family_properties_count, device_queue_families[i].properties.data());
    }
  }
  check_result.physical_device_count = physical_devices.size();
  return OK;
}

VulkanCheckResult check_vulkan([[maybe_unused]] bool request_validation)
{
  _initializee_volk();
  _initialize_vulkan_version();
  _validate_reqired_instance_extensions();
  _initialize_vulkan_instance();
  _check_devices();
  // TODO
  return check_result;
}

}
