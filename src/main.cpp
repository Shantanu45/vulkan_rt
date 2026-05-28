#include "app/AppConfig.hpp"
#include "app/Application.hpp"
#include "app/VulkanContextSmoke.hpp"
#include "render/vulkan/VulkanCheck.hpp"

#include "util/logger.h"
#include <fmt/format.h>
#include <internal_use_only/config.hpp>
#include <exception>
#include <memory>
#include <utility>

int main(int argc, char **argv)
{
  auto logger = std::make_unique<::Util::StdSpdLogger>();
  ::Util::set_logger_iface(logger.get());

  try
  {
    auto config = vulkan_rt::app::parse_app_config(argc, argv);
    ::Util::set_debug_logging_enabled(config.verbose);

    LOGD("Git SHA: {}", vulkan_rt::cmake::git_sha);
    if (config.dry_run_config) {
      LOGI("{} {} config: {}x{}, validation={}, gpu={}, scene={}, app_smoke={}",
        vulkan_rt::cmake::project_name,
        vulkan_rt::cmake::project_version,
        config.width,
        config.height,
        config.validation,
        config.gpu_index,
        config.scene,
        config.app_smoke);
      return 0;
    }

  if (config.check_vulkan) {
    auto result = vulkan_rt::render::vulkan::check_vulkan(config.validation);
    LOGI("\n loader: {},\n instance creation: {},\n validation: {},\n physical device count: {}\n",
      result.loader_present,
      result.instance_created,
      result.validation_layer_available,
      result.physical_device_count);
    if (!result.error.empty()) {
      LOGE("Vulkan check failed: {}", result.error);
      return 1;
    }
    return 0;
  }

  if (config.vulkan_context_smoke) {
    return vulkan_rt::app::vulkan_context_smoke_test(config);
  }

  if (config.vulkan_device_smoke) {
    return vulkan_rt::app::vulkan_device_smoke_test(config);
  }

  if (config.vulkan_swapchain_smoke) {
    return vulkan_rt::app::vulkan_swapchain_smoke_test(config);
  }

  if (config.vulkan_frame_smoke) {
    return vulkan_rt::app::vulkan_frame_smoke_test(config);
  }

  if (config.vulkan_shader_smoke) {
    return vulkan_rt::app::vulkan_shader_smoke_test(config);
  }

  if (config.vulkan_rt_pipeline_smoke) {
    return vulkan_rt::app::vulkan_rt_pipeline_smoke_test(config);
  }

  if (config.vulkan_sbt_smoke) {
    return vulkan_rt::app::vulkan_sbt_smoke_test(config);
  }

  if (config.vulkan_rt_descriptor_smoke) {
    return vulkan_rt::app::vulkan_rt_descriptor_smoke_test(config);
  }

  if (config.vulkan_trace_smoke) {
    return vulkan_rt::app::vulkan_trace_smoke_test(config);
  }

  if (config.vulkan_acceleration_structure_smoke) {
    return vulkan_rt::app::vulkan_acceleration_structure_smoke_test(config);
  }

  if (config.vulkan_triangle_blas_smoke) {
    return vulkan_rt::app::vulkan_triangle_blas_smoke_test(config);
  }

  if (config.vulkan_tlas_smoke) {
    return vulkan_rt::app::vulkan_tlas_smoke_test(config);
  }

  if (config.vulkan_buffer_smoke) {
    return vulkan_rt::app::vulkan_buffer_smoke_test(config);
  }

  if (config.vulkan_clear_smoke) {
    return vulkan_rt::app::vulkan_clear_smoke_test(config);
  }

  if (config.vulkan_resize_smoke) {
    return vulkan_rt::app::vulkan_resize_smoke_test(config);
  }

  if (config.vulkan_renderer_smoke) {
    return vulkan_rt::app::vulkan_renderer_smoke_test(config);
  }

    const bool app_smoke = config.app_smoke;
    vulkan_rt::app::Application application{ std::move(config) };
    if (app_smoke) { return application.smoke_test(); }
    return application.run();
  }
  catch(const std::exception &exception)
  {
    LOGE("Fatal error: {}", exception.what());
    return 1;
  }
}
