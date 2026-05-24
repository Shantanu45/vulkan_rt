#include "scene/Camera.hpp"

#include <algorithm>
#include <cmath>

namespace vulkan_rt::scene
{
namespace
{
constexpr float pi = 3.14159265358979323846F;
constexpr Vec3 world_up{0.0F, 1.0F, 0.0F};

float degrees_to_radians(float degrees)
{
  return degrees * pi / 180.0F;
}

Vec3 add(Vec3 lhs, Vec3 rhs)
{
  return Vec3{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

Vec3 scale(Vec3 vector, float scalar)
{
  return Vec3{vector.x * scalar, vector.y * scalar, vector.z * scalar};
}

float dot(Vec3 lhs, Vec3 rhs)
{
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vec3 cross(Vec3 lhs, Vec3 rhs)
{
  return Vec3{
    lhs.y * rhs.z - lhs.z * rhs.y,
    lhs.z * rhs.x - lhs.x * rhs.z,
    lhs.x * rhs.y - lhs.y * rhs.x,
  };
}

Vec3 normalize(Vec3 vector)
{
  const float length = std::sqrt(dot(vector, vector));
  if(length <= 0.0F)
  {
    return Vec3{};
  }
  return scale(vector, 1.0F / length);
}
}

Camera::Camera() = default;

void Camera::set_aspect_ratio(float aspect_ratio)
{
  if(aspect_ratio > 0.0F)
  {
    aspect_ratio_ = aspect_ratio;
  }
}

void Camera::set_viewport_size(int width, int height)
{
  if(width > 0 && height > 0)
  {
    set_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
  }
}

void Camera::set_orientation(float yaw_degrees, float pitch_degrees)
{
  yaw_degrees_ = yaw_degrees;
  pitch_degrees_ = std::clamp(pitch_degrees, -89.0F, 89.0F);
}

void Camera::set_vertical_fov_degrees(float vertical_fov_degrees)
{
  vertical_fov_degrees_ = std::clamp(vertical_fov_degrees, 1.0F, 120.0F);
}

void Camera::move_local(float forward, float right, float up)
{
  position_ = add(position_, scale(view_direction(), forward));
  position_ = add(position_, scale(right_direction(), right));
  position_ = add(position_, scale(world_up, up));
}

Vec3 Camera::position() const
{
  return position_;
}

float Camera::yaw_degrees() const
{
  return yaw_degrees_;
}

float Camera::pitch_degrees() const
{
  return pitch_degrees_;
}

float Camera::vertical_fov_degrees() const
{
  return vertical_fov_degrees_;
}

float Camera::aspect_ratio() const
{
  return aspect_ratio_;
}

float Camera::near_plane() const
{
  return near_plane_;
}

float Camera::far_plane() const
{
  return far_plane_;
}

Vec3 Camera::view_direction() const
{
  const float yaw = degrees_to_radians(yaw_degrees_);
  const float pitch = degrees_to_radians(pitch_degrees_);

  return normalize(Vec3{
    std::cos(yaw) * std::cos(pitch),
    std::sin(pitch),
    std::sin(yaw) * std::cos(pitch),
  });
}

Vec3 Camera::right_direction() const
{
  return normalize(cross(view_direction(), world_up));
}

Vec3 Camera::up_direction() const
{
  return normalize(cross(right_direction(), view_direction()));
}
}
