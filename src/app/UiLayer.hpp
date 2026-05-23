#pragma once

#include "app/Window.hpp"

namespace vulkan_rt::app
{
struct UiStats
{
  double frame_time_ms = 0.0;
  double fps = 0.0;
  Extent framebuffer_extent = {};
};

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
}
