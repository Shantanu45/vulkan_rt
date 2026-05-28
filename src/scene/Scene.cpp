#include "scene/Scene.hpp"

#include <cmath>
#include <utility>

namespace vulkan_rt::scene {
namespace {
  constexpr float pi = 3.14159265358979323846F;

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

  constexpr Vec3 subtract(Vec3 lhs, Vec3 rhs)
  { return Vec3{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z }; }

  constexpr Vec3 scale(Vec3 value, float scalar)
  { return Vec3{ value.x * scalar, value.y * scalar, value.z * scalar }; }

  constexpr Vec3 cross(Vec3 lhs, Vec3 rhs)
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

  Vec3 normalize(Vec3 value)
  {
    const float len = length(value);
    if(len <= 0.0F)
    {
      return Vec3{ 0.0F, 1.0F, 0.0F };
    }
    return scale(value, 1.0F / len);
  }

  void add_quad(std::vector<Triangle> &triangles, Vec3 a, Vec3 b, Vec3 c, Vec3 d, std::uint32_t material_index)
  {
    const Vec3 normal = normalize(cross(subtract(b, a), subtract(c, a)));
    triangles.push_back(Triangle{ .v0 = a, .v1 = b, .v2 = c, .n0 = normal, .n1 = normal, .n2 = normal, .material_index = material_index });
    triangles.push_back(Triangle{ .v0 = a, .v1 = c, .v2 = d, .n0 = normal, .n1 = normal, .n2 = normal, .material_index = material_index });
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

  Vec3 sphere_point(Vec3 center, float radius, float theta, float phi)
  {
    return Vec3{
      center.x + radius * std::sin(theta) * std::cos(phi),
      center.y + radius * std::cos(theta),
      center.z + radius * std::sin(theta) * std::sin(phi),
    };
  }

  void add_smooth_triangle(std::vector<Triangle> &triangles, Vec3 center, Vec3 a, Vec3 b, Vec3 c, std::uint32_t material_index)
  {
    triangles.push_back(Triangle{
      .v0 = a,
      .v1 = b,
      .v2 = c,
      .n0 = normalize(subtract(a, center)),
      .n1 = normalize(subtract(b, center)),
      .n2 = normalize(subtract(c, center)),
      .material_index = material_index,
    });
  }

  void add_uv_sphere(
    std::vector<Triangle> &triangles, Vec3 center, float radius, std::uint32_t rings, std::uint32_t segments, std::uint32_t material_index)
  {
    for(std::uint32_t ring = 0; ring < rings; ++ring)
    {
      const float theta0 = pi * static_cast<float>(ring) / static_cast<float>(rings);
      const float theta1 = pi * static_cast<float>(ring + 1) / static_cast<float>(rings);

      for(std::uint32_t segment = 0; segment < segments; ++segment)
      {
        const float phi0 = 2.0F * pi * static_cast<float>(segment) / static_cast<float>(segments);
        const float phi1 = 2.0F * pi * static_cast<float>(segment + 1) / static_cast<float>(segments);

        const Vec3 p00 = sphere_point(center, radius, theta0, phi0);
        const Vec3 p01 = sphere_point(center, radius, theta0, phi1);
        const Vec3 p10 = sphere_point(center, radius, theta1, phi0);
        const Vec3 p11 = sphere_point(center, radius, theta1, phi1);

        if(ring == 0)
        {
          add_smooth_triangle(triangles, center, p00, p10, p11, material_index);
        }
        else if(ring + 1 == rings)
        {
          add_smooth_triangle(triangles, center, p00, p10, p01, material_index);
        }
        else
        {
          add_smooth_triangle(triangles, center, p00, p10, p11, material_index);
          add_smooth_triangle(triangles, center, p00, p11, p01, material_index);
        }
      }
    }
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
    make_metal(Vec3{ 1.0F, 0.78F, 0.34F }, 0.38F),
    make_dielectric(Vec3{ 0.92F, 0.97F, 1.0F }, 0.02F, 1.5F),
  };

  constexpr std::uint32_t white = 0;
  constexpr std::uint32_t red = 1;
  constexpr std::uint32_t green = 2;
  constexpr std::uint32_t light = 3;
  constexpr std::uint32_t metal = 4;
  constexpr std::uint32_t dielectric = 5;

  std::vector<Triangle> triangles;
  triangles.reserve(1080);

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
  add_box(triangles, { -0.25F, -1.0F, -0.70F }, { 0.25F, -0.78F, -0.20F }, white);
  add_uv_sphere(triangles, { -0.42F, -0.66F, 0.05F }, 0.34F, 12, 24, metal);
  add_uv_sphere(triangles, { 0.43F, -0.68F, 0.18F }, 0.32F, 12, 24, dielectric);

  return Scene{ std::move(materials), std::move(triangles) };
}
}// namespace vulkan_rt::scene
