#include "VulkanCheck.hpp"
#include "SDL3/SDL_vulkan.h"
#include <vector>
#include <array>
#include <string>
#include "libassert/assert.hpp"
#include <set>

namespace vulkan_rt::render::vulkan
{
uint32_t instance_api_version = VK_API_VERSION_1_0;
std::unordered_map<std::string, bool> requested_instance_extensions;
std::set<std::string> enabled_instance_extension_names;

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
    LOGE("Failed to initialize Vulkan loader via volk.\n");
    return ERR_CANT_CREATE;
  }
  return OK;
}

void _fill_reqiured_instance_extensions()
{
  std::array extenstions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VULKAN_DEBUG
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif// VULKAN DEBUG
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
        LOGI("Optional extension %s not found.", requested_extension.first.c_str());
      }
    }
  }
  return OK;
}


VulkanCheckResult check_vulkan([[maybe_unused]] bool request_validation)
{
  _initializee_volk();
  _initialize_vulkan_version();
  _validate_reqired_instance_extensions();
  // TODO
  return VulkanCheckResult{};
}

}
