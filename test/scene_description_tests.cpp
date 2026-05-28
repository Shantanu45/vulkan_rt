#include "scene/SceneDescription.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("scene description builds procedural scene")
{
  vulkan_rt::scene::SceneDescription description;
  description.name = "test";
  description.objects.push_back(vulkan_rt::scene::SceneObjectDescription{
    .type = "procedural_cornell_box",
  });

  std::string error;
  const auto scene = vulkan_rt::scene::make_scene_from_description(description, error);

  CHECK(error.empty());
  CHECK_FALSE(scene.empty());
  CHECK(scene.materials().size() == 6);
  CHECK(scene.triangles().size() == 1080);
}

TEST_CASE("scene description applies object transform")
{
  vulkan_rt::scene::SceneDescription description;
  description.objects.push_back(vulkan_rt::scene::SceneObjectDescription{
    .type = "procedural_cornell_box",
    .position = {10.0F, 0.0F, 0.0F},
    .scale = 2.0F,
  });

  std::string error;
  const auto scene = vulkan_rt::scene::make_scene_from_description(description, error);

  REQUIRE_FALSE(scene.empty());
  CHECK(scene.triangles().front().v0.x == 8.0F);
}
