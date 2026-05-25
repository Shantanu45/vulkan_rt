#pragma once

#include "scene/Camera.hpp"

namespace vulkan_rt::render::vulkan
{
struct RayTracingCameraData
{
  float position_fov_y[4]{};
  float forward_aspect[4]{};
  float right[4]{};
  float up[4]{};
};

static_assert(sizeof(RayTracingCameraData) == 64);

inline void write_ray_tracing_camera_vec3(float (&target)[4], scene::Vec3 source, float w)
{
  target[0] = source.x;
  target[1] = source.y;
  target[2] = source.z;
  target[3] = w;
}

inline RayTracingCameraData make_ray_tracing_camera_data(const scene::Camera &camera)
{
  RayTracingCameraData gpu_camera{};
  write_ray_tracing_camera_vec3(gpu_camera.position_fov_y, camera.position(), camera.vertical_fov_degrees());
  write_ray_tracing_camera_vec3(gpu_camera.forward_aspect, camera.view_direction(), camera.aspect_ratio());
  write_ray_tracing_camera_vec3(gpu_camera.right, camera.right_direction(), 0.0F);
  write_ray_tracing_camera_vec3(gpu_camera.up, camera.up_direction(), 0.0F);
  return gpu_camera;
}
}
