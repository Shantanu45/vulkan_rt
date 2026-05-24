/*****************************************************************//**
 * \file   VulkanCheck.hpp
 * \brief  checks:
 * - Vulkan loader present
 * - instance can be created
 * - validation layer availability
 * - physical devices found
 * 
 * \author Shantanu Kumar
 * \date   May 2026
 *********************************************************************/
#pragma  once
#include <stdint.h>
#include <string>

#include "volk.h"
#include "util/error_macros.h"

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

Error _initialize_vulkan_version();

Error _initializee_volk();
}
