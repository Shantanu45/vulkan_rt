#pragma once

#include "app/AppConfig.hpp"

namespace vulkan_rt::app
{
int vulkan_context_smoke_test(const AppConfig &config);
int vulkan_device_smoke_test(const AppConfig &config);
int vulkan_swapchain_smoke_test(const AppConfig &config);
}
