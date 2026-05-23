#pragma  once
#include <stdint.h>
#include <string>

namespace vulkan_rt::render::vulkan
{
struct VulkanCheckResult
{
  bool loader_present = false;
  bool instance_created = false;
  bool validation_layer_available = false;
  uint32_t physical_device_count = 0;
  std::string error = "";
};

VulkanCheckResult check_vulkan( bool request_validation);
}
