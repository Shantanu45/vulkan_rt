#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace vulkan_rt::scene {
struct Vec3
{
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

enum class MaterialType : std::uint32_t
{
  Diffuse = 0,
  Metal = 1,
  Dielectric = 2,
  Emissive = 3,
};

struct Material
{
  Vec3 albedo{ 1.0F, 1.0F, 1.0F };
  Vec3 emission{};
  MaterialType type = MaterialType::Diffuse;
  float roughness = 1.0F;
  float ior = 1.5F;
};

struct Triangle
{
  Vec3 v0{};
  Vec3 v1{};
  Vec3 v2{};
  Vec3 n0{};
  Vec3 n1{};
  Vec3 n2{};
  std::uint32_t material_index = 0;
};

class Scene
{
public:
  Scene() = default;
  Scene(std::vector<Material> materials, std::vector<Triangle> triangles);

  [[nodiscard]] std::span<const Material> materials() const;
  [[nodiscard]] std::span<const Triangle> triangles() const;
  [[nodiscard]] bool empty() const;

private:
  std::vector<Material> materials_;
  std::vector<Triangle> triangles_;
};

[[nodiscard]] Scene make_procedural_scene();
}// namespace vulkan_rt::scene
