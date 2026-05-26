#include "app/Application.hpp"

#include "app/SdlSurfaceProvider.hpp"
#include "render/vulkan/VulkanRenderer.hpp"
#include "render/vulkan/VulkanRendererConfig.hpp"
#include "util/logger.h"
#include <SDL3/SDL.h>
#include <fmt/format.h>
#include <internal_use_only/config.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace vulkan_rt::app {
namespace
{
engine::CameraControlInput make_camera_control_input(
  const vulkan_rt::input::InputSystem &input, bool allow_keyboard, bool allow_mouse)
{
  return engine::CameraControlInput{
    .move_forward = allow_keyboard && input.is_held(vulkan_rt::input::Key::W),
    .move_backward = allow_keyboard && input.is_held(vulkan_rt::input::Key::S),
    .move_left = allow_keyboard && input.is_held(vulkan_rt::input::Key::A),
    .move_right = allow_keyboard && input.is_held(vulkan_rt::input::Key::D),
    .move_up = allow_keyboard && input.is_held(vulkan_rt::input::Key::E),
    .move_down = allow_keyboard && input.is_held(vulkan_rt::input::Key::Q),
    .fast_move = allow_keyboard && input.is_held(vulkan_rt::input::Key::LeftShift),
    .rotate = allow_mouse && input.is_mouse_held(vulkan_rt::input::MouseButton::Right),
    .mouse_delta_x = allow_mouse ? input.get_mouse_delta().x : 0.0F,
    .mouse_delta_y = allow_mouse ? input.get_mouse_delta().y : 0.0F,
  };
}
}

SdlRuntime::SdlRuntime()
{
  if (!SDL_Init(SDL_INIT_VIDEO)) { throw std::runtime_error(std::string{ "SDL_Init failed: " } + SDL_GetError()); }
}

SdlRuntime::~SdlRuntime() { SDL_Quit(); }

Application::Application(AppConfig config)
  : config_(std::move(config)), sdl_runtime_(),
    window_(fmt::format("{} {}", vulkan_rt::cmake::project_name, vulkan_rt::cmake::project_version),
      config_.width,
      config_.height),
    ui_(), engine_(engine::make_engine_config(config_)), previous_frame_time_(Clock::now())
{
  LOGI("Created SDL3 application window: {}x{}", config_.width, config_.height);
  ui_.initialize(window_.native_handle());
  const auto extent = window_.framebuffer_extent();
  SdlSurfaceProvider surface_provider{window_.native_handle()};
  const std::string application_name{vulkan_rt::cmake::project_name};
  render::vulkan::VulkanRendererConfig vulkan_config{
    .validation = config_.validation,
    .application_name = application_name.c_str(),
    .gpu_index = config_.gpu_index,
  };
  engine_.set_renderer(std::make_unique<render::vulkan::VulkanRenderer>(
    vulkan_config,
    surface_provider,
    render::vulkan::SwapchainExtent{
      .width = static_cast<std::uint32_t>(extent.width),
      .height = static_cast<std::uint32_t>(extent.height),
    }));
  engine_.resize(extent.width, extent.height);
}

Application::~Application() = default;

int Application::run()
{
  while (!window_.should_close()) {
    const auto now = Clock::now();
    const std::chrono::duration<double> delta = now - previous_frame_time_;
    previous_frame_time_ = now;

    tick_once(delta.count());

    SDL_Delay(1);
  }

  return 0;
}

int Application::smoke_test()
{
  const auto now = Clock::now();
  const std::chrono::duration<double> delta = now - previous_frame_time_;
  previous_frame_time_ = now;

  tick_once(delta.count());

  const auto extent = window_.framebuffer_extent();
  LOGI("Application smoke test passed: framebuffer {}x{}", extent.width, extent.height);
  return 0;
}

void Application::tick_once(double delta_seconds)
{
  window_.poll_events(input_, ui_);

  const auto extent = window_.framebuffer_extent();
  if(extent.width <= 0 || extent.height <= 0)
  {
    return;
  }

  if (window_.was_resized()) {
    LOGD("Window resized: {}x{}", extent.width, extent.height);
    engine_.resize(extent.width, extent.height);
    window_.clear_resize_flag();
  }

  auto renderer_settings = engine_.renderer_settings();
  ui_.begin_frame();
  const auto ui_actions = ui_.draw(make_ui_stats(engine_.frame_stats(), extent), renderer_settings);
  ui_.end_frame();
  if(ui_actions.renderer_settings_changed)
  {
    engine_.set_renderer_settings(renderer_settings);
  }
  if(ui_actions.reset_accumulation_requested)
  {
    engine_.request_accumulation_reset();
  }

  engine_.update_camera(make_camera_control_input(input_, !ui_.wants_keyboard(), !ui_.wants_mouse()), delta_seconds);
  engine_.update(delta_seconds);
  engine_.render();
}
}// namespace vulkan_rt::app
