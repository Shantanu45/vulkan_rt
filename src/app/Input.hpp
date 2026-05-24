#pragma once

#include <SDL3/SDL.h>

namespace vulkan_rt::app {
class Input
{
public:
  void begin_frame();
  void handle_event(const SDL_Event &event);

  [[nodiscard]] bool quit_requested() const;
  [[nodiscard]] bool escape_pressed() const;
  [[nodiscard]] bool right_mouse_down() const;
  [[nodiscard]] float mouse_delta_x() const;
  [[nodiscard]] float mouse_delta_y() const;

private:
  bool quit_requested_ = false;
  bool escape_pressed_ = false;
  bool right_mouse_down_ = false;
  float mouse_delta_x_ = 0.0F;
  float mouse_delta_y_ = 0.0F;
};
}// namespace vulkan_rt::app
