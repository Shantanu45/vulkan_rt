#pragma once

#include "app/AppConfig.hpp"
#include "app/Input.hpp"
#include "app/UiLayer.hpp"
#include "app/Window.hpp"
#include "engine/Engine.hpp"

#include <chrono>

namespace vulkan_rt::app {
class SdlRuntime
{
public:
  SdlRuntime();
  ~SdlRuntime();

  SdlRuntime(const SdlRuntime &) = delete;
  SdlRuntime &operator=(const SdlRuntime &) = delete;
  SdlRuntime(SdlRuntime &&) = delete;
  SdlRuntime &operator=(SdlRuntime &&) = delete;
};

class Application
{
public:
  explicit Application(AppConfig config);
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;
  Application(Application &&) = delete;
  Application &operator=(Application &&) = delete;

  int run();
  int smoke_test();

private:
  using Clock = std::chrono::steady_clock;

  void tick_once(double delta_seconds);

  AppConfig config_;
  SdlRuntime sdl_runtime_;
  Window window_;
  Input input_;
  engine::Engine engine_;
  UiLayer ui_;
  Clock::time_point previous_frame_time_;
};
}// namespace vulkan_rt::app
