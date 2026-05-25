#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload
{
  vec3 radiance;
  uint hit;
  vec3 hit_position;
  float _pad0;
  vec3 normal;
  float _pad1;
  vec3 albedo;
  float _pad2;
  vec3 emission;
  float _pad3;
};

layout(location = 0) rayPayloadInEXT RayPayload payload;

struct Material
{
  vec4 albedo;
  vec4 emission;
};

layout(set = 0, binding = 2, std430) readonly buffer MaterialIndices
{
  uint material_indices[];
};

layout(set = 0, binding = 3, std430) readonly buffer Materials
{
  Material materials[];
};

layout(set = 0, binding = 7, std430) readonly buffer Vertices
{
  float vertices[];
};

void main()
{
  const uint material_index = material_indices[gl_PrimitiveID];
  const Material material = materials[material_index];
  const uint vertex_offset = gl_PrimitiveID * 9u;
  const vec3 v0 = vec3(vertices[vertex_offset + 0u], vertices[vertex_offset + 1u], vertices[vertex_offset + 2u]);
  const vec3 v1 = vec3(vertices[vertex_offset + 3u], vertices[vertex_offset + 4u], vertices[vertex_offset + 5u]);
  const vec3 v2 = vec3(vertices[vertex_offset + 6u], vertices[vertex_offset + 7u], vertices[vertex_offset + 8u]);
  const vec3 geometric_normal = normalize(cross(v1 - v0, v2 - v0));

  payload.hit = 1u;
  payload.radiance = vec3(0.0);
  payload.hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  payload.normal = faceforward(geometric_normal, gl_WorldRayDirectionEXT, geometric_normal);
  payload.albedo = material.albedo.rgb;
  payload.emission = material.emission.rgb;
}
