#include "app/Application.hpp"

#include <SDL3/SDL.h>
#include <fmt/format.h>
#include <internal_use_only/config.hpp>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace vulkan_rt::app
{
SdlRuntime::SdlRuntime()
{
  if(!SDL_Init(SDL_INIT_VIDEO)) { throw std::runtime_error(std::string{"SDL_Init failed: "} + SDL_GetError()); }
}

SdlRuntime::~SdlRuntime()
{
  SDL_Quit();
}

Application::Application(AppConfig config)
  : config_(std::move(config))
  , sdl_runtime_()
  , window_(fmt::format("{} {}", vulkan_rt::cmake::project_name, vulkan_rt::cmake::project_version),
      config_.width,
      config_.height)
  , engine_(engine::make_engine_config(config_))
  , previous_frame_time_(Clock::now())
{
  spdlog::info("Created SDL3 application window: {}x{}", config_.width, config_.height);
}

Application::~Application() = default;

int Application::run()
{
  while(!window_.should_close())
  {
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
  spdlog::info("Application smoke test passed: framebuffer {}x{}", extent.width, extent.height);
  return 0;
}

void Application::tick_once(double delta_seconds)
{
  window_.poll_events(input_);

  engine_.update(input_, delta_seconds);
  engine_.render();

  ui_.begin_frame();
  ui_.draw(make_ui_stats(engine_.frame_stats(), window_.framebuffer_extent()));
  ui_.end_frame();

  if(window_.was_resized())
  {
    const auto extent = window_.framebuffer_extent();
    spdlog::debug("Window resized: {}x{}", extent.width, extent.height);
    window_.clear_resize_flag();
  }
}
}
