#pragma once

#include <cstdint>

namespace vulkan_rt::engine
{
struct FrameStats
{
  double frame_time_ms = 0.0;
  double fps = 0.0;
  uint64_t frame_index = 0;
};
}
