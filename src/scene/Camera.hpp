#pragma once

#include "scene/Scene.hpp"

namespace vulkan_rt::scene
{
class Camera
{
public:
  Camera();

  void set_aspect_ratio(float aspect_ratio);
  void set_viewport_size(int width, int height);
  void set_orientation(float yaw_degrees, float pitch_degrees);
  void set_vertical_fov_degrees(float vertical_fov_degrees);
  void move_local(float forward, float right, float up);

  [[nodiscard]] Vec3 position() const;
  [[nodiscard]] float yaw_degrees() const;
  [[nodiscard]] float pitch_degrees() const;
  [[nodiscard]] float vertical_fov_degrees() const;
  [[nodiscard]] float aspect_ratio() const;
  [[nodiscard]] float near_plane() const;
  [[nodiscard]] float far_plane() const;

  [[nodiscard]] Vec3 view_direction() const;
  [[nodiscard]] Vec3 right_direction() const;
  [[nodiscard]] Vec3 up_direction() const;

private:
  Vec3 position_{0.0F, 0.0F, 3.0F};
  float yaw_degrees_ = -90.0F;
  float pitch_degrees_ = 0.0F;
  float vertical_fov_degrees_ = 45.0F;
  float aspect_ratio_ = 16.0F / 9.0F;
  float near_plane_ = 0.01F;
  float far_plane_ = 1000.0F;
};
}
