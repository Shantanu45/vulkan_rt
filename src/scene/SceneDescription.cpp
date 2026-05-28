#include "scene/SceneDescription.hpp"

#include "scene/gltf_loader.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace vulkan_rt::scene
{
namespace
{
Vec3 read_vec3(const nlohmann::json &json, const char *key, Vec3 fallback)
{
  if(!json.contains(key))
  {
    return fallback;
  }

  const auto &value = json.at(key);
  if(!value.is_array() || value.size() != 3)
  {
    throw std::runtime_error(std::string{key} + " must be an array of 3 numbers.");
  }

  return Vec3{
    value.at(0).get<float>(),
    value.at(1).get<float>(),
    value.at(2).get<float>(),
  };
}

Triangle transform_triangle(Triangle triangle, Vec3 position, float scale)
{
  const auto transform_point = [position, scale](Vec3 point) {
    return Vec3{
      point.x * scale + position.x,
      point.y * scale + position.y,
      point.z * scale + position.z,
    };
  };

  triangle.v0 = transform_point(triangle.v0);
  triangle.v1 = transform_point(triangle.v1);
  triangle.v2 = transform_point(triangle.v2);
  return triangle;
}

void append_scene(
  std::vector<Material> &materials,
  std::vector<Triangle> &triangles,
  const Scene &source,
  Vec3 position,
  float scale)
{
  const auto material_offset = static_cast<std::uint32_t>(materials.size());
  materials.insert(materials.end(), source.materials().begin(), source.materials().end());

  for(auto triangle : source.triangles())
  {
    triangle.material_index += material_offset;
    triangles.push_back(transform_triangle(triangle, position, scale));
  }
}

Scene load_gltf_scene(const SceneObjectDescription &object, std::string &error_message)
{
  GltfLoader loader;
  if(!loader.load(object.path, &error_message))
  {
    return {};
  }
  return loader.make_scene();
}
}// namespace

bool load_scene_description(const std::string &path, SceneDescription &description, std::string &error_message)
{
  try
  {
    std::ifstream input{path};
    if(!input)
    {
      error_message = "Failed to open scene description: " + path;
      return false;
    }

    const auto json = nlohmann::json::parse(input);

    SceneDescription parsed;
    parsed.name = json.value("name", parsed.name);

    const auto &objects = json.at("objects");
    if(!objects.is_array())
    {
      error_message = "Scene description field 'objects' must be an array.";
      return false;
    }

    parsed.objects.reserve(objects.size());
    for(const auto &object_json : objects)
    {
      SceneObjectDescription object;
      object.type = object_json.value("type", object.type);
      object.path = object_json.value("path", object.path);
      object.position = read_vec3(object_json, "position", object.position);
      object.scale = object_json.value("scale", object.scale);
      if(object.scale <= 0.0F)
      {
        throw std::runtime_error("Scene object scale must be positive.");
      }
      parsed.objects.push_back(std::move(object));
    }

    description = std::move(parsed);
    error_message.clear();
    return true;
  }
  catch(const std::exception &exception)
  {
    error_message = exception.what();
    return false;
  }
}

Scene make_scene_from_description(const SceneDescription &description, std::string &error_message)
{
  std::vector<Material> materials;
  std::vector<Triangle> triangles;

  try
  {
    for(const auto &object : description.objects)
    {
      if(object.type == "procedural_cornell_box")
      {
        append_scene(materials, triangles, make_procedural_scene(), object.position, object.scale);
      }
      else if(object.type == "gltf")
      {
        if(object.path.empty())
        {
          error_message = "glTF scene object requires a non-empty path.";
          return {};
        }

        std::string gltf_error;
        const auto gltf_scene = load_gltf_scene(object, gltf_error);
        if(gltf_scene.empty())
        {
          error_message = gltf_error.empty() ? "Failed to load glTF scene object: " + object.path : gltf_error;
          return {};
        }
        append_scene(materials, triangles, gltf_scene, object.position, object.scale);
      }
      else
      {
        error_message = "Unsupported scene object type: " + object.type;
        return {};
      }
    }
  }
  catch(const std::exception &exception)
  {
    error_message = exception.what();
    return {};
  }

  error_message.clear();
  return Scene{std::move(materials), std::move(triangles)};
}
}// namespace vulkan_rt::scene
