#pragma once

#include "input/input.h"

#include <SDL3/SDL.h>

#include <string_view>

namespace vulkan_rt::app {
struct Extent
{
  int width = 0;
  int height = 0;
};

class Window
{
public:
  Window(std::string_view title, int width, int height);
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;
  Window(Window &&) = delete;
  Window &operator=(Window &&) = delete;

  void poll_events(vulkan_rt::input::InputSystem &input);

  [[nodiscard]] bool should_close() const;
  [[nodiscard]] bool was_resized() const;
  [[nodiscard]] Extent framebuffer_extent() const;
  [[nodiscard]] SDL_Window *native_handle() const;

  void clear_resize_flag();
  void request_close();

private:
  SDL_Window *handle_ = nullptr;
  bool should_close_ = false;
  bool resized_ = false;
};
}// namespace vulkan_rt::app
