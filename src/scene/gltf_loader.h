#pragma once

#include "scene/Scene.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tinygltf
{
class Model;
struct Node;
struct Primitive;
}

namespace vulkan_rt::scene
{
struct GltfVertex
{
  Vec3 position{};
  Vec3 normal{0.0F, 1.0F, 0.0F};
};

struct GltfPrimitive
{
  std::vector<GltfVertex> vertices;
  std::vector<std::uint32_t> indices;
  std::uint32_t material_index = 0;
};

struct GltfMesh
{
  std::string name;
  std::vector<GltfPrimitive> primitives;
};

struct GltfMaterial
{
  std::string name;
  Material material;
};

struct GltfNode
{
  std::string name;
  int mesh_index = -1;
  int parent_index = -1;
  std::vector<int> children;
  float local_transform[16]{};
};

struct GltfSceneInfo
{
  std::string name;
  std::vector<int> root_nodes;
};

struct GltfAsset
{
  std::vector<GltfMesh> meshes;
  std::vector<GltfMaterial> materials;
  std::vector<GltfNode> nodes;
  std::vector<GltfSceneInfo> scenes;
  int default_scene = 0;
};

class GltfLoader
{
public:
  [[nodiscard]] bool load(const std::string &path, std::string *error_message = nullptr);
  [[nodiscard]] const GltfAsset &asset() const;
  [[nodiscard]] Scene make_scene() const;

private:
  [[nodiscard]] GltfPrimitive extract_primitive(const tinygltf::Model &model, const tinygltf::Primitive &primitive) const;
  [[nodiscard]] GltfMaterial extract_material(const tinygltf::Model &model, int material_index) const;
  [[nodiscard]] GltfNode extract_node(const tinygltf::Node &node) const;
  void append_node_triangles(std::vector<Triangle> &triangles, int node_index, const float world_transform[16]) const;

  GltfAsset asset_;
};
}// namespace vulkan_rt::scene
