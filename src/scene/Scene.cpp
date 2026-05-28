#include "scene/Scene.hpp"

#include <utility>

namespace vulkan_rt::scene {
namespace {
  constexpr Material make_diffuse(Vec3 albedo)
  { return Material{ .albedo = albedo, .emission = {}, .type = MaterialType::Diffuse, .roughness = 1.0F }; }

  constexpr Material make_metal(Vec3 albedo, float roughness)
  { return Material{ .albedo = albedo, .emission = {}, .type = MaterialType::Metal, .roughness = roughness }; }

  constexpr Material make_dielectric(Vec3 tint, float roughness, float ior)
  { return Material{ .albedo = tint, .emission = {}, .type = MaterialType::Dielectric, .roughness = roughness, .ior = ior }; }

  constexpr Material make_light(Vec3 emission)
  {
    return Material{
      .albedo = { 1.0F, 1.0F, 1.0F }, .emission = emission, .type = MaterialType::Emissive, .roughness = 0.0F };
  }

  void add_quad(std::vector<Triangle> &triangles, Vec3 a, Vec3 b, Vec3 c, Vec3 d, std::uint32_t material_index)
  {
    triangles.push_back(Triangle{ .v0 = a, .v1 = b, .v2 = c, .material_index = material_index });
    triangles.push_back(Triangle{ .v0 = a, .v1 = c, .v2 = d, .material_index = material_index });
  }

  void add_box(std::vector<Triangle> &triangles, Vec3 min, Vec3 max, std::uint32_t material_index)
  {
    add_quad(triangles, { min.x, min.y, min.z }, { max.x, min.y, min.z }, { max.x, min.y, max.z }, { min.x, min.y, max.z }, material_index);
    add_quad(triangles, { min.x, max.y, min.z }, { min.x, max.y, max.z }, { max.x, max.y, max.z }, { max.x, max.y, min.z }, material_index);
    add_quad(triangles, { min.x, min.y, min.z }, { min.x, max.y, min.z }, { max.x, max.y, min.z }, { max.x, min.y, min.z }, material_index);
    add_quad(triangles, { min.x, min.y, max.z }, { max.x, min.y, max.z }, { max.x, max.y, max.z }, { min.x, max.y, max.z }, material_index);
    add_quad(triangles, { min.x, min.y, min.z }, { min.x, min.y, max.z }, { min.x, max.y, max.z }, { min.x, max.y, min.z }, material_index);
    add_quad(triangles, { max.x, min.y, min.z }, { max.x, max.y, min.z }, { max.x, max.y, max.z }, { max.x, min.y, max.z }, material_index);
  }
}// namespace

Scene::Scene(std::vector<Material> materials, std::vector<Triangle> triangles)
  : materials_(std::move(materials)), triangles_(std::move(triangles))
{}

std::span<const Material> Scene::materials() const { return std::span<const Material>{ materials_ }; }

std::span<const Triangle> Scene::triangles() const { return std::span<const Triangle>{ triangles_ }; }

bool Scene::empty() const { return materials_.empty() || triangles_.empty(); }

Scene make_procedural_scene()
{
  std::vector<Material> materials{
    make_diffuse(Vec3{ 0.75F, 0.75F, 0.75F }),
    make_diffuse(Vec3{ 0.75F, 0.10F, 0.10F }),
    make_diffuse(Vec3{ 0.10F, 0.55F, 0.20F }),
    make_light(Vec3{ 8.0F, 7.5F, 6.5F }),
    make_metal(Vec3{ 0.86F, 0.78F, 0.62F }, 0.12F),
    make_dielectric(Vec3{ 0.92F, 0.97F, 1.0F }, 0.02F, 1.5F),
  };

  constexpr std::uint32_t white = 0;
  constexpr std::uint32_t red = 1;
  constexpr std::uint32_t green = 2;
  constexpr std::uint32_t light = 3;
  constexpr std::uint32_t metal = 4;
  constexpr std::uint32_t dielectric = 5;

  std::vector<Triangle> triangles;
  triangles.reserve(36);

  add_quad(
    triangles, { -1.0F, -1.0F, -1.0F }, { 1.0F, -1.0F, -1.0F }, { 1.0F, -1.0F, 1.0F }, { -1.0F, -1.0F, 1.0F }, white);
  add_quad(
    triangles, { -1.0F, 1.0F, -1.0F }, { -1.0F, 1.0F, 1.0F }, { 1.0F, 1.0F, 1.0F }, { 1.0F, 1.0F, -1.0F }, white);
  add_quad(
    triangles, { -1.0F, -1.0F, -1.0F }, { -1.0F, 1.0F, -1.0F }, { 1.0F, 1.0F, -1.0F }, { 1.0F, -1.0F, -1.0F }, white);
  add_quad(
    triangles, { -1.0F, -1.0F, 1.0F }, { -1.0F, 1.0F, 1.0F }, { -1.0F, 1.0F, -1.0F }, { -1.0F, -1.0F, -1.0F }, red);
  add_quad(
    triangles, { 1.0F, -1.0F, -1.0F }, { 1.0F, 1.0F, -1.0F }, { 1.0F, 1.0F, 1.0F }, { 1.0F, -1.0F, 1.0F }, green);
  add_quad(triangles,
    { -0.25F, 0.90F, -0.25F },
    { -0.25F, 0.90F, 0.25F },
    { 0.25F, 0.90F, 0.25F },
    { 0.25F, 0.90F, -0.25F },
    light);
  add_box(triangles, { -0.65F, -1.0F, -0.35F }, { -0.20F, -0.30F, 0.25F }, metal);
  add_box(triangles, { 0.25F, -1.0F, -0.10F }, { 0.65F, -0.55F, 0.35F }, dielectric);

  return Scene{ std::move(materials), std::move(triangles) };
}
}// namespace vulkan_rt::scene
