#include "scene/Camera.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
float length(vulkan_rt::scene::Vec3 vector)
{
  return std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}
}

TEST_CASE("camera defaults are suitable for the procedural scene")
{
  const vulkan_rt::scene::Camera camera;

  CHECK(camera.position().z == Catch::Approx(3.0F));
  CHECK(camera.vertical_fov_degrees() == Catch::Approx(45.0F));
  CHECK(camera.aspect_ratio() == Catch::Approx(16.0F / 9.0F));
  CHECK(camera.near_plane() > 0.0F);
  CHECK(camera.far_plane() > camera.near_plane());
}

TEST_CASE("camera updates aspect ratio from viewport size")
{
  vulkan_rt::scene::Camera camera;

  camera.set_viewport_size(1920, 1080);

  CHECK(camera.aspect_ratio() == Catch::Approx(16.0F / 9.0F));

  camera.set_viewport_size(1024, 1024);

  CHECK(camera.aspect_ratio() == Catch::Approx(1.0F));
}

TEST_CASE("camera clamps unsafe orientation and fov values")
{
  vulkan_rt::scene::Camera camera;

  camera.set_orientation(25.0F, 120.0F);
  camera.set_vertical_fov_degrees(200.0F);

  CHECK(camera.yaw_degrees() == Catch::Approx(25.0F));
  CHECK(camera.pitch_degrees() == Catch::Approx(89.0F));
  CHECK(camera.vertical_fov_degrees() == Catch::Approx(120.0F));
}

TEST_CASE("camera local movement changes position")
{
  vulkan_rt::scene::Camera camera;
  const auto before = camera.position();

  camera.move_local(1.0F, 2.0F, 3.0F);

  const auto after = camera.position();
  CHECK(after.x != Catch::Approx(before.x));
  CHECK(after.y != Catch::Approx(before.y));
  CHECK(after.z != Catch::Approx(before.z));
}

TEST_CASE("camera direction vectors are normalized")
{
  vulkan_rt::scene::Camera camera;
  camera.set_orientation(-35.0F, 15.0F);

  CHECK(length(camera.view_direction()) == Catch::Approx(1.0F));
  CHECK(length(camera.right_direction()) == Catch::Approx(1.0F));
  CHECK(length(camera.up_direction()) == Catch::Approx(1.0F));
}
