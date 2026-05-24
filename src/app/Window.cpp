#include "app/Window.hpp"

#include "app/Input.hpp"

#include <stdexcept>
#include <string>

namespace vulkan_rt::app {
Window::Window(std::string_view title, int width, int height)
{
  handle_ = SDL_CreateWindow(std::string{ title }.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (handle_ == nullptr) { throw std::runtime_error(std::string{ "SDL_CreateWindow failed: " } + SDL_GetError()); }
}

Window::~Window()
{
  if (handle_ != nullptr) { SDL_DestroyWindow(handle_); }
}

void Window::poll_events(Input &input)
{
  input.begin_frame();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    input.handle_event(event);

    switch (event.type) {
    case SDL_EVENT_QUIT:
      should_close_ = true;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      if (event.window.windowID == SDL_GetWindowID(handle_)) { resized_ = true; }
      break;
    default:
      break;
    }
  }

  if (input.quit_requested() || input.escape_pressed()) { should_close_ = true; }
}

bool Window::should_close() const { return should_close_; }

bool Window::was_resized() const { return resized_; }

Extent Window::framebuffer_extent() const
{
  Extent extent;
  if (!SDL_GetWindowSizeInPixels(handle_, &extent.width, &extent.height)) {
    throw std::runtime_error(std::string{ "SDL_GetWindowSizeInPixels failed: " } + SDL_GetError());
  }
  return extent;
}

SDL_Window *Window::native_handle() const { return handle_; }

void Window::clear_resize_flag() { resized_ = false; }

void Window::request_close() { should_close_ = true; }
}// namespace vulkan_rt::app
