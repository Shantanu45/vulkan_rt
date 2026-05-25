#include "render/vulkan/RayTracingCameraData.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("ray tracing camera data packs camera state for the shader")
{
  vulkan_rt::scene::Camera camera;
  camera.set_viewport_size(1600, 900);
  camera.set_orientation(-45.0F, 10.0F);
  camera.set_vertical_fov_degrees(60.0F);
  camera.move_local(1.0F, 2.0F, 3.0F);

  const auto data = vulkan_rt::render::vulkan::make_ray_tracing_camera_data(camera);
  const auto position = camera.position();
  const auto forward = camera.view_direction();
  const auto right = camera.right_direction();
  const auto up = camera.up_direction();

  CHECK(data.position_fov_y[0] == Catch::Approx(position.x));
  CHECK(data.position_fov_y[1] == Catch::Approx(position.y));
  CHECK(data.position_fov_y[2] == Catch::Approx(position.z));
  CHECK(data.position_fov_y[3] == Catch::Approx(60.0F));

  CHECK(data.forward_aspect[0] == Catch::Approx(forward.x));
  CHECK(data.forward_aspect[1] == Catch::Approx(forward.y));
  CHECK(data.forward_aspect[2] == Catch::Approx(forward.z));
  CHECK(data.forward_aspect[3] == Catch::Approx(16.0F / 9.0F));

  CHECK(data.right[0] == Catch::Approx(right.x));
  CHECK(data.right[1] == Catch::Approx(right.y));
  CHECK(data.right[2] == Catch::Approx(right.z));
  CHECK(data.right[3] == Catch::Approx(0.0F));

  CHECK(data.up[0] == Catch::Approx(up.x));
  CHECK(data.up[1] == Catch::Approx(up.y));
  CHECK(data.up[2] == Catch::Approx(up.z));
  CHECK(data.up[3] == Catch::Approx(0.0F));
}

TEST_CASE("ray tracing camera data keeps shader-facing layout stable")
{
  CHECK(sizeof(vulkan_rt::render::vulkan::RayTracingCameraData) == 64);
}
