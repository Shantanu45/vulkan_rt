#include "scene/Scene.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

namespace
{
bool in_range(float value, float min_value, float max_value)
{
  return value >= min_value && value <= max_value;
}

bool color_in_range(vulkan_rt::scene::Vec3 color, float min_value, float max_value)
{
  return in_range(color.x, min_value, max_value) && in_range(color.y, min_value, max_value) &&
         in_range(color.z, min_value, max_value);
}
}

TEST_CASE("procedural scene contains renderable geometry")
{
  const auto scene = vulkan_rt::scene::make_procedural_scene();

  CHECK_FALSE(scene.empty());
  CHECK(scene.materials().size() == 4);
  CHECK(scene.triangles().size() == 12);
}

TEST_CASE("procedural scene triangles reference valid materials")
{
  const auto scene = vulkan_rt::scene::make_procedural_scene();

  for(const auto &triangle : scene.triangles())
  {
    CHECK(triangle.material_index < scene.materials().size());
  }
}

TEST_CASE("procedural scene materials have sane values")
{
  const auto scene = vulkan_rt::scene::make_procedural_scene();

  const auto has_emissive_material =
    std::any_of(scene.materials().begin(), scene.materials().end(), [](const auto &material) {
      return material.emission.x > 0.0F || material.emission.y > 0.0F || material.emission.z > 0.0F;
    });

  CHECK(has_emissive_material);

  for(const auto &material : scene.materials())
  {
    CHECK(color_in_range(material.albedo, 0.0F, 1.0F));
    CHECK(material.roughness >= 0.0F);
    CHECK(material.roughness <= 1.0F);
    CHECK(material.emission.x >= 0.0F);
    CHECK(material.emission.y >= 0.0F);
    CHECK(material.emission.z >= 0.0F);
  }
}
