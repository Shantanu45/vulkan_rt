#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "scene/gltf_loader.h"

#include <tiny_gltf.h>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace vulkan_rt::scene
{
namespace
{
constexpr float identity_matrix[16]{
  1.0F, 0.0F, 0.0F, 0.0F,
  0.0F, 1.0F, 0.0F, 0.0F,
  0.0F, 0.0F, 1.0F, 0.0F,
  0.0F, 0.0F, 0.0F, 1.0F,
};

float matrix_at(const float matrix[16], int column, int row)
{
  return matrix[column * 4 + row];
}

void set_matrix_at(float matrix[16], int column, int row, float value)
{
  matrix[column * 4 + row] = value;
}

void copy_identity(float matrix[16])
{
  std::memcpy(matrix, identity_matrix, sizeof(identity_matrix));
}

void multiply_matrix(const float lhs[16], const float rhs[16], float out[16])
{
  float result[16]{};
  for(int column = 0; column < 4; ++column)
  {
    for(int row = 0; row < 4; ++row)
    {
      float value = 0.0F;
      for(int k = 0; k < 4; ++k)
      {
        value += matrix_at(lhs, k, row) * matrix_at(rhs, column, k);
      }
      set_matrix_at(result, column, row, value);
    }
  }
  std::memcpy(out, result, sizeof(result));
}

Vec3 transform_point(const float matrix[16], Vec3 point)
{
  return Vec3{
    matrix_at(matrix, 0, 0) * point.x + matrix_at(matrix, 1, 0) * point.y + matrix_at(matrix, 2, 0) * point.z +
      matrix_at(matrix, 3, 0),
    matrix_at(matrix, 0, 1) * point.x + matrix_at(matrix, 1, 1) * point.y + matrix_at(matrix, 2, 1) * point.z +
      matrix_at(matrix, 3, 1),
    matrix_at(matrix, 0, 2) * point.x + matrix_at(matrix, 1, 2) * point.y + matrix_at(matrix, 2, 2) * point.z +
      matrix_at(matrix, 3, 2),
  };
}

Vec3 transform_vector(const float matrix[16], Vec3 vector)
{
  return Vec3{
    matrix_at(matrix, 0, 0) * vector.x + matrix_at(matrix, 1, 0) * vector.y + matrix_at(matrix, 2, 0) * vector.z,
    matrix_at(matrix, 0, 1) * vector.x + matrix_at(matrix, 1, 1) * vector.y + matrix_at(matrix, 2, 1) * vector.z,
    matrix_at(matrix, 0, 2) * vector.x + matrix_at(matrix, 1, 2) * vector.y + matrix_at(matrix, 2, 2) * vector.z,
  };
}

Vec3 subtract(Vec3 lhs, Vec3 rhs)
{
  return Vec3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

Vec3 cross(Vec3 lhs, Vec3 rhs)
{
  return Vec3{
    lhs.y * rhs.z - lhs.z * rhs.y,
    lhs.z * rhs.x - lhs.x * rhs.z,
    lhs.x * rhs.y - lhs.y * rhs.x,
  };
}

float length(Vec3 value)
{
  return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

Vec3 normalize_or(Vec3 value, Vec3 fallback)
{
  const float len = length(value);
  if(len <= 0.0F)
  {
    return fallback;
  }
  return Vec3{value.x / len, value.y / len, value.z / len};
}

Material default_material()
{
  return Material{.albedo = {0.8F, 0.8F, 0.8F}, .type = MaterialType::Diffuse, .roughness = 1.0F};
}

const tinygltf::Accessor *find_accessor(const tinygltf::Model &model, const tinygltf::Primitive &primitive, const char *name)
{
  const auto found = primitive.attributes.find(name);
  if(found == primitive.attributes.end())
  {
    return nullptr;
  }
  return &model.accessors[found->second];
}

const std::uint8_t *accessor_data(const tinygltf::Model &model, const tinygltf::Accessor &accessor, std::size_t &stride)
{
  const auto &view = model.bufferViews[accessor.bufferView];
  const auto &buffer = model.buffers[view.buffer];
  stride = accessor.ByteStride(view);
  return buffer.data.data() + view.byteOffset + accessor.byteOffset;
}

Vec3 read_vec3_attribute(const tinygltf::Model &model, const tinygltf::Accessor &accessor, std::size_t index)
{
  if(accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC3)
  {
    throw std::runtime_error("glTF attribute must be FLOAT VEC3.");
  }

  std::size_t stride = 0;
  const auto *data = accessor_data(model, accessor, stride);
  const auto *values = reinterpret_cast<const float *>(data + index * stride);
  return Vec3{values[0], values[1], values[2]};
}

std::uint32_t read_index(const tinygltf::Model &model, const tinygltf::Accessor &accessor, std::size_t index)
{
  const auto &view = model.bufferViews[accessor.bufferView];
  const auto &buffer = model.buffers[view.buffer];
  const auto *data = buffer.data.data() + view.byteOffset + accessor.byteOffset;

  switch(accessor.componentType)
  {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
      return data[index];
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
      return reinterpret_cast<const std::uint16_t *>(data)[index];
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
      return reinterpret_cast<const std::uint32_t *>(data)[index];
    default:
      throw std::runtime_error("Unsupported glTF index component type.");
  }
}

void make_translation(float matrix[16], const std::vector<double> &translation)
{
  copy_identity(matrix);
  if(translation.size() == 3)
  {
    set_matrix_at(matrix, 3, 0, static_cast<float>(translation[0]));
    set_matrix_at(matrix, 3, 1, static_cast<float>(translation[1]));
    set_matrix_at(matrix, 3, 2, static_cast<float>(translation[2]));
  }
}

void make_scale(float matrix[16], const std::vector<double> &scale)
{
  copy_identity(matrix);
  if(scale.size() == 3)
  {
    set_matrix_at(matrix, 0, 0, static_cast<float>(scale[0]));
    set_matrix_at(matrix, 1, 1, static_cast<float>(scale[1]));
    set_matrix_at(matrix, 2, 2, static_cast<float>(scale[2]));
  }
}

void make_rotation(float matrix[16], const std::vector<double> &rotation)
{
  copy_identity(matrix);
  if(rotation.size() != 4)
  {
    return;
  }

  const float x = static_cast<float>(rotation[0]);
  const float y = static_cast<float>(rotation[1]);
  const float z = static_cast<float>(rotation[2]);
  const float w = static_cast<float>(rotation[3]);

  set_matrix_at(matrix, 0, 0, 1.0F - 2.0F * y * y - 2.0F * z * z);
  set_matrix_at(matrix, 0, 1, 2.0F * x * y + 2.0F * w * z);
  set_matrix_at(matrix, 0, 2, 2.0F * x * z - 2.0F * w * y);
  set_matrix_at(matrix, 1, 0, 2.0F * x * y - 2.0F * w * z);
  set_matrix_at(matrix, 1, 1, 1.0F - 2.0F * x * x - 2.0F * z * z);
  set_matrix_at(matrix, 1, 2, 2.0F * y * z + 2.0F * w * x);
  set_matrix_at(matrix, 2, 0, 2.0F * x * z + 2.0F * w * y);
  set_matrix_at(matrix, 2, 1, 2.0F * y * z - 2.0F * w * x);
  set_matrix_at(matrix, 2, 2, 1.0F - 2.0F * x * x - 2.0F * y * y);
}

bool has_emission(Vec3 emission)
{
  return emission.x > 0.0F || emission.y > 0.0F || emission.z > 0.0F;
}
}// namespace

bool GltfLoader::load(const std::string &path, std::string *error_message)
{
  tinygltf::TinyGLTF loader;
  tinygltf::Model model;
  std::string error;
  std::string warning;

  const auto extension = std::filesystem::path{path}.extension().string();
  const bool is_binary = extension == ".glb" || extension == ".GLB";
  const bool loaded = is_binary ? loader.LoadBinaryFromFile(&model, &error, &warning, path)
                                : loader.LoadASCIIFromFile(&model, &error, &warning, path);
  if(!loaded)
  {
    if(error_message != nullptr)
    {
      *error_message = error.empty() ? "Failed to load glTF file." : error;
    }
    return false;
  }

  try
  {
    asset_ = {};
    asset_.materials.reserve(model.materials.size() + 1);
    asset_.materials.push_back(GltfMaterial{.name = "default", .material = default_material()});
    for(int material_index = 0; material_index < static_cast<int>(model.materials.size()); ++material_index)
    {
      asset_.materials.push_back(extract_material(model, material_index));
    }

    asset_.meshes.reserve(model.meshes.size());
    for(const auto &mesh_source : model.meshes)
    {
      GltfMesh mesh;
      mesh.name = mesh_source.name;
      mesh.primitives.reserve(mesh_source.primitives.size());
      for(const auto &primitive : mesh_source.primitives)
      {
        if(primitive.mode == TINYGLTF_MODE_TRIANGLES)
        {
          mesh.primitives.push_back(extract_primitive(model, primitive));
        }
      }
      asset_.meshes.push_back(std::move(mesh));
    }

    asset_.nodes.reserve(model.nodes.size());
    for(const auto &node_source : model.nodes)
    {
      asset_.nodes.push_back(extract_node(node_source));
    }
    for(int node_index = 0; node_index < static_cast<int>(asset_.nodes.size()); ++node_index)
    {
      for(const int child : asset_.nodes[node_index].children)
      {
        if(child >= 0 && child < static_cast<int>(asset_.nodes.size()))
        {
          asset_.nodes[child].parent_index = node_index;
        }
      }
    }

    asset_.scenes.reserve(model.scenes.size());
    for(const auto &scene_source : model.scenes)
    {
      asset_.scenes.push_back(GltfSceneInfo{.name = scene_source.name, .root_nodes = scene_source.nodes});
    }
    asset_.default_scene = model.defaultScene >= 0 ? model.defaultScene : 0;
    if(asset_.scenes.empty() && !asset_.nodes.empty())
    {
      asset_.scenes.push_back(GltfSceneInfo{.name = "default", .root_nodes = {0}});
      asset_.default_scene = 0;
    }
  }
  catch(const std::exception &exception)
  {
    asset_ = {};
    if(error_message != nullptr)
    {
      *error_message = exception.what();
    }
    return false;
  }

  if(error_message != nullptr)
  {
    *error_message = warning;
  }
  return true;
}

const GltfAsset &GltfLoader::asset() const
{
  return asset_;
}

Scene GltfLoader::make_scene() const
{
  std::vector<Material> materials;
  materials.reserve(asset_.materials.size());
  for(const auto &material : asset_.materials)
  {
    materials.push_back(material.material);
  }

  std::vector<Triangle> triangles;
  if(asset_.scenes.empty())
  {
    return Scene{std::move(materials), std::move(triangles)};
  }

  const auto scene_index = std::clamp(asset_.default_scene, 0, static_cast<int>(asset_.scenes.size()) - 1);
  for(const int root_node : asset_.scenes[scene_index].root_nodes)
  {
    append_node_triangles(triangles, root_node, identity_matrix);
  }

  return Scene{std::move(materials), std::move(triangles)};
}

GltfPrimitive GltfLoader::extract_primitive(const tinygltf::Model &model, const tinygltf::Primitive &primitive) const
{
  const auto *positions = find_accessor(model, primitive, "POSITION");
  if(positions == nullptr)
  {
    throw std::runtime_error("glTF primitive is missing POSITION.");
  }

  const auto *normals = find_accessor(model, primitive, "NORMAL");

  GltfPrimitive result;
  result.material_index = primitive.material >= 0 ? static_cast<std::uint32_t>(primitive.material + 1) : 0;
  result.vertices.resize(positions->count);

  for(std::size_t index = 0; index < positions->count; ++index)
  {
    result.vertices[index].position = read_vec3_attribute(model, *positions, index);
    if(normals != nullptr)
    {
      result.vertices[index].normal = normalize_or(read_vec3_attribute(model, *normals, index), Vec3{0.0F, 1.0F, 0.0F});
    }
  }

  if(primitive.indices >= 0)
  {
    const auto &indices = model.accessors[primitive.indices];
    result.indices.resize(indices.count);
    for(std::size_t index = 0; index < indices.count; ++index)
    {
      result.indices[index] = read_index(model, indices, index);
    }
  }
  else
  {
    result.indices.resize(result.vertices.size());
    for(std::size_t index = 0; index < result.vertices.size(); ++index)
    {
      result.indices[index] = static_cast<std::uint32_t>(index);
    }
  }

  return result;
}

GltfMaterial GltfLoader::extract_material(const tinygltf::Model &model, int material_index) const
{
  const auto &source = model.materials[material_index];
  const auto &pbr = source.pbrMetallicRoughness;

  Material material = default_material();
  material.albedo = Vec3{
    static_cast<float>(pbr.baseColorFactor[0]),
    static_cast<float>(pbr.baseColorFactor[1]),
    static_cast<float>(pbr.baseColorFactor[2]),
  };
  material.roughness = static_cast<float>(pbr.roughnessFactor);
  material.emission = Vec3{
    static_cast<float>(source.emissiveFactor[0]),
    static_cast<float>(source.emissiveFactor[1]),
    static_cast<float>(source.emissiveFactor[2]),
  };

  if(has_emission(material.emission))
  {
    material.type = MaterialType::Emissive;
  }
  else if(pbr.metallicFactor > 0.5)
  {
    material.type = MaterialType::Metal;
  }
  else
  {
    material.type = MaterialType::Diffuse;
  }

  return GltfMaterial{.name = source.name, .material = material};
}

GltfNode GltfLoader::extract_node(const tinygltf::Node &node) const
{
  GltfNode result;
  result.name = node.name;
  result.mesh_index = node.mesh;
  result.children = node.children;
  copy_identity(result.local_transform);

  if(node.matrix.size() == 16)
  {
    for(std::size_t index = 0; index < 16; ++index)
    {
      result.local_transform[index] = static_cast<float>(node.matrix[index]);
    }
    return result;
  }

  float translation[16]{};
  float rotation[16]{};
  float scale[16]{};
  float translation_rotation[16]{};
  make_translation(translation, node.translation);
  make_rotation(rotation, node.rotation);
  make_scale(scale, node.scale);
  multiply_matrix(translation, rotation, translation_rotation);
  multiply_matrix(translation_rotation, scale, result.local_transform);
  return result;
}

void GltfLoader::append_node_triangles(std::vector<Triangle> &triangles, int node_index, const float world_transform[16]) const
{
  if(node_index < 0 || node_index >= static_cast<int>(asset_.nodes.size()))
  {
    return;
  }

  const auto &node = asset_.nodes[node_index];
  float node_world[16]{};
  multiply_matrix(world_transform, node.local_transform, node_world);

  if(node.mesh_index >= 0 && node.mesh_index < static_cast<int>(asset_.meshes.size()))
  {
    const auto &mesh = asset_.meshes[node.mesh_index];
    for(const auto &primitive : mesh.primitives)
    {
      for(std::size_t index = 0; index + 2 < primitive.indices.size(); index += 3)
      {
        const auto i0 = primitive.indices[index + 0];
        const auto i1 = primitive.indices[index + 1];
        const auto i2 = primitive.indices[index + 2];
        if(i0 >= primitive.vertices.size() || i1 >= primitive.vertices.size() || i2 >= primitive.vertices.size())
        {
          continue;
        }

        const auto &v0 = primitive.vertices[i0];
        const auto &v1 = primitive.vertices[i1];
        const auto &v2 = primitive.vertices[i2];
        const auto p0 = transform_point(node_world, v0.position);
        const auto p1 = transform_point(node_world, v1.position);
        const auto p2 = transform_point(node_world, v2.position);
        const auto geometric_normal = normalize_or(cross(subtract(p1, p0), subtract(p2, p0)), Vec3{0.0F, 1.0F, 0.0F});

        triangles.push_back(Triangle{
          .v0 = p0,
          .v1 = p1,
          .v2 = p2,
          .n0 = normalize_or(transform_vector(node_world, v0.normal), geometric_normal),
          .n1 = normalize_or(transform_vector(node_world, v1.normal), geometric_normal),
          .n2 = normalize_or(transform_vector(node_world, v2.normal), geometric_normal),
          .material_index = primitive.material_index,
        });
      }
    }
  }

  for(const int child : node.children)
  {
    append_node_triangles(triangles, child, node_world);
  }
}
}// namespace vulkan_rt::scene
