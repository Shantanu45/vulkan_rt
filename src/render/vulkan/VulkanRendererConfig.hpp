#pragma once

struct VulkanRendererConfig
{
  bool validation = false;
  const char *application_name = "vulkan_rt";
  int gpu_index = -1;
};
