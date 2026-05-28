#pragma once

#include "scene/Scene.hpp"

#include <string>
#include <vector>

namespace vulkan_rt::scene
{
struct SceneObjectDescription
{
  std::string type = "procedural_cornell_box";
  std::string path;
  Vec3 position{};
  float scale = 1.0F;
};

struct SceneDescription
{
  std::string name = "Untitled";
  std::vector<SceneObjectDescription> objects;
};

[[nodiscard]] bool load_scene_description(
  const std::string &path, SceneDescription &description, std::string &error_message);
[[nodiscard]] Scene make_scene_from_description(const SceneDescription &description, std::string &error_message);
}// namespace vulkan_rt::scene
