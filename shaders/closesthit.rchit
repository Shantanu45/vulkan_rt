#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 payload;

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

void main()
{
  const uint material_index = material_indices[gl_PrimitiveID];
  const Material material = materials[material_index];
  payload = material.albedo.rgb + material.emission.rgb * 0.05;
}
