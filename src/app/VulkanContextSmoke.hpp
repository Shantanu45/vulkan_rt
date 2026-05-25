#pragma once

#include "app/AppConfig.hpp"

namespace vulkan_rt::app
{
int vulkan_context_smoke_test(const AppConfig &config);
int vulkan_device_smoke_test(const AppConfig &config);
int vulkan_swapchain_smoke_test(const AppConfig &config);
int vulkan_frame_smoke_test(const AppConfig &config);
int vulkan_shader_smoke_test(const AppConfig &config);
int vulkan_rt_pipeline_smoke_test(const AppConfig &config);
int vulkan_sbt_smoke_test(const AppConfig &config);
int vulkan_acceleration_structure_smoke_test(const AppConfig &config);
int vulkan_triangle_blas_smoke_test(const AppConfig &config);
int vulkan_tlas_smoke_test(const AppConfig &config);
int vulkan_buffer_smoke_test(const AppConfig &config);
int vulkan_clear_smoke_test(const AppConfig &config);
int vulkan_resize_smoke_test(const AppConfig &config);
}
