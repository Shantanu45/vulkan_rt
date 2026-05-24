#include "scene/Scene.hpp"

#include <utility>

namespace vulkan_rt::scene {
namespace {
  constexpr Material make_diffuse(Vec3 albedo)
  { return Material{ .albedo = albedo, .emission = {}, .roughness = 1.0F }; }

  constexpr Material make_light(Vec3 emission)
  { return Material{ .albedo = { 1.0F, 1.0F, 1.0F }, .emission = emission, .roughness = 0.0F }; }

  void add_quad(std::vector<Triangle> &triangles, Vec3 a, Vec3 b, Vec3 c, Vec3 d, std::uint32_t material_index)
  {
    triangles.push_back(Triangle{ .v0 = a, .v1 = b, .v2 = c, .material_index = material_index });
    triangles.push_back(Triangle{ .v0 = a, .v1 = c, .v2 = d, .material_index = material_index });
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
  };

  constexpr std::uint32_t white = 0;
  constexpr std::uint32_t red = 1;
  constexpr std::uint32_t green = 2;
  constexpr std::uint32_t light = 3;

  std::vector<Triangle> triangles;
  triangles.reserve(12);

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
    { -0.25F, 0.99F, -0.25F },
    { -0.25F, 0.99F, 0.25F },
    { 0.25F, 0.99F, 0.25F },
    { 0.25F, 0.99F, -0.25F },
    light);

  return Scene{ std::move(materials), std::move(triangles) };
}
}// namespace vulkan_rt::scene
