#include "app/UiLayer.hpp"

namespace vulkan_rt::app {
UiStats make_ui_stats(const engine::FrameStats &frame_stats, Extent framebuffer_extent)
{
  UiStats stats;
  stats.frame_time_ms = frame_stats.frame_time_ms;
  stats.fps = frame_stats.fps;
  stats.frame_index = frame_stats.frame_index;
  stats.framebuffer_extent = framebuffer_extent;
  return stats;
}

void UiLayer::begin_frame() {}

void UiLayer::draw(const UiStats &stats) { last_stats_ = stats; }

void UiLayer::end_frame() {}

const UiStats &UiLayer::last_stats() const { return last_stats_; }
}// namespace vulkan_rt::app
