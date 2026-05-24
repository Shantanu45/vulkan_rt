#pragma once

#include "app/Window.hpp"
#include "engine/FrameStats.hpp"

#include <cstdint>

namespace vulkan_rt::app {
struct UiStats
{
  double frame_time_ms = 0.0;
  double fps = 0.0;
  uint64_t frame_index = 0;
  Extent framebuffer_extent = {};
};

[[nodiscard]] UiStats make_ui_stats(const engine::FrameStats &frame_stats, Extent framebuffer_extent);

class UiLayer
{
public:
  void begin_frame();
  void draw(const UiStats &stats);
  void end_frame();

  [[nodiscard]] const UiStats &last_stats() const;

private:
  UiStats last_stats_ = {};
};
}// namespace vulkan_rt::app
