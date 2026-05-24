#include "app/Input.hpp"

namespace vulkan_rt::app {
void Input::begin_frame()
{
  escape_pressed_ = false;
  mouse_delta_x_ = 0.0F;
  mouse_delta_y_ = 0.0F;
}

void Input::handle_event(const SDL_Event &event)
{
  switch (event.type) {
  case SDL_EVENT_QUIT:
    quit_requested_ = true;
    break;
  case SDL_EVENT_KEY_DOWN:
    if (!event.key.repeat && event.key.scancode == SDL_SCANCODE_ESCAPE) { escape_pressed_ = true; }
    break;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (event.button.button == SDL_BUTTON_RIGHT) { right_mouse_down_ = true; }
    break;
  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (event.button.button == SDL_BUTTON_RIGHT) { right_mouse_down_ = false; }
    break;
  case SDL_EVENT_MOUSE_MOTION:
    mouse_delta_x_ += event.motion.xrel;
    mouse_delta_y_ += event.motion.yrel;
    break;
  default:
    break;
  }
}

bool Input::quit_requested() const { return quit_requested_; }

bool Input::escape_pressed() const { return escape_pressed_; }

bool Input::right_mouse_down() const { return right_mouse_down_; }

float Input::mouse_delta_x() const { return mouse_delta_x_; }

float Input::mouse_delta_y() const { return mouse_delta_y_; }
}// namespace vulkan_rt::app
