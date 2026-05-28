#pragma once

#include "app/Window.hpp"
#include "engine/Engine.hpp"
#include "engine/FrameStats.hpp"
#include "render/Renderer.hpp"

#include <SDL3/SDL.h>

#include <cstdint>

namespace vulkan_rt::app {
struct UiStats
{
  double frame_time_ms = 0.0;
  double fps = 0.0;
  uint64_t frame_index = 0;
  uint64_t accumulated_sample_count = 0;
  Extent framebuffer_extent = {};
};

struct UiActions
{
  bool renderer_settings_changed = false;
  bool camera_settings_changed = false;
  bool reset_accumulation_requested = false;
};

[[nodiscard]] UiStats make_ui_stats(const engine::FrameStats &frame_stats, Extent framebuffer_extent);

class UiLayer
{
public:
  UiLayer();
  ~UiLayer();

  UiLayer(const UiLayer &) = delete;
  UiLayer &operator=(const UiLayer &) = delete;
  UiLayer(UiLayer &&) = delete;
  UiLayer &operator=(UiLayer &&) = delete;

  void initialize(SDL_Window *window);
  void handle_event(const SDL_Event &event);
  void begin_frame();
  UiActions draw(
    const UiStats &stats, render::RendererSettings &renderer_settings, engine::CameraSettings &camera_settings);
  void end_frame();

  [[nodiscard]] const UiStats &last_stats() const;
  [[nodiscard]] bool wants_keyboard() const;
  [[nodiscard]] bool wants_mouse() const;

private:
  UiStats last_stats_ = {};
  bool initialized_ = false;
};
}// namespace vulkan_rt::app
