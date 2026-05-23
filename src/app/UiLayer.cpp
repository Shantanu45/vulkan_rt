#include "app/UiLayer.hpp"

namespace vulkan_rt::app
{
void UiLayer::begin_frame() {}

void UiLayer::draw(const UiStats &stats)
{
  last_stats_ = stats;
}

void UiLayer::end_frame() {}

const UiStats &UiLayer::last_stats() const
{
  return last_stats_;
}
}
