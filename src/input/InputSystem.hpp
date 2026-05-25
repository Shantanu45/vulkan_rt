#pragma once

#include "SDL3/SDL.h"

#include <cstddef>
#include <cstdint>

namespace vulkan_rt::input
{
struct Vec2
{
  float x = 0.0F;
  float y = 0.0F;
};

enum class Key : std::uint16_t
{
  Unknown,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Return,
  LeftCtrl,
  LeftAlt,
  LeftShift,
  Space,
  Escape,
  Left,
  Right,
  Up,
  Down,
  _1,
  _2,
  _3,
  _4,
  _5,
  _6,
  _7,
  _8,
  _9,
  _0,
  Count
};

enum KeyState : std::uint8_t
{
  KEY_NONE = 0,
  KEY_DOWN = 1 << 0,
  KEY_PRESSED = 1 << 1,
  KEY_RELEASED = 1 << 2,
};

enum class MouseButton
{
  Left,
  Middle,
  Right,
  Count
};

enum MouseState : std::uint8_t
{
  MOUSE_NONE = 0,
  MOUSE_DOWN = 1 << 0,
  MOUSE_PRESSED = 1 << 1,
  MOUSE_RELEASED = 1 << 2,
};

class InputSystemInterface
{
public:
  virtual ~InputSystemInterface() = default;
  virtual void on_sdl_event(const SDL_Event &event) = 0;

  [[nodiscard]] virtual bool is_held(Key key) const = 0;
  [[nodiscard]] virtual bool just_pressed(Key key) const = 0;
  [[nodiscard]] virtual bool just_released(Key key) const = 0;

  [[nodiscard]] virtual bool is_mouse_held(MouseButton button) const = 0;
  [[nodiscard]] virtual bool just_mouse_pressed(MouseButton button) const = 0;
  [[nodiscard]] virtual bool just_mouse_released(MouseButton button) const = 0;
  [[nodiscard]] virtual Vec2 get_mouse_position() const = 0;
  [[nodiscard]] virtual Vec2 get_mouse_delta() const = 0;

  virtual void update() = 0;
};

class InputSystem : public InputSystemInterface
{
public:
  InputSystem();

  void on_sdl_event(const SDL_Event &event) override;

  [[nodiscard]] bool is_held(Key key) const override;
  [[nodiscard]] bool just_pressed(Key key) const override;
  [[nodiscard]] bool just_released(Key key) const override;
  [[nodiscard]] bool is_mouse_held(MouseButton button) const override;
  [[nodiscard]] bool just_mouse_pressed(MouseButton button) const override;
  [[nodiscard]] bool just_mouse_released(MouseButton button) const override;
  [[nodiscard]] Vec2 get_mouse_position() const override;
  [[nodiscard]] Vec2 get_mouse_delta() const override;

  void update() override;

private:
  Key sdl_to_key_[SDL_SCANCODE_COUNT]{};
  std::uint8_t keys_[static_cast<std::size_t>(Key::Count)]{};
  std::uint8_t mouse_[static_cast<std::size_t>(MouseButton::Count)]{};
  Vec2 mouse_position_{};
  Vec2 mouse_delta_{};
  Vec2 mouse_delta_accum_{};
  Vec2 last_mouse_position_{};
};
}
