#version 460
#extension GL_EXT_ray_tracing : require

struct RayPayload
{
  vec3 radiance;
  uint hit;
  vec3 hit_position;
  uint front_face;
  vec3 normal;
  float roughness;
  vec3 albedo;
  uint material_type;
  vec3 emission;
  float ior;
};

layout(location = 0) rayPayloadInEXT RayPayload payload;

struct Material
{
  vec4 albedo;
  vec4 emission;
  vec4 params;
};

layout(set = 0, binding = 2, std430) readonly buffer MaterialIndices
{
  uint material_indices[];
};

layout(set = 0, binding = 3, std430) readonly buffer Materials
{
  Material materials[];
};

struct Vertex
{
  vec4 position;
  vec4 normal;
};

layout(set = 0, binding = 7, std430) readonly buffer Vertices
{
  Vertex vertices[];
};

hitAttributeEXT vec2 hit_attributes;

void main()
{
  const uint material_index = material_indices[gl_PrimitiveID];
  const Material material = materials[material_index];
  const uint vertex_offset = gl_PrimitiveID * 3u;
  const Vertex vertex0 = vertices[vertex_offset + 0u];
  const Vertex vertex1 = vertices[vertex_offset + 1u];
  const Vertex vertex2 = vertices[vertex_offset + 2u];
  const vec3 v0 = vertex0.position.xyz;
  const vec3 v1 = vertex1.position.xyz;
  const vec3 v2 = vertex2.position.xyz;
  const vec3 geometric_normal = normalize(cross(v1 - v0, v2 - v0));
  const vec3 barycentrics = vec3(1.0 - hit_attributes.x - hit_attributes.y, hit_attributes.x, hit_attributes.y);
  const vec3 smooth_normal =
    normalize(vertex0.normal.xyz * barycentrics.x + vertex1.normal.xyz * barycentrics.y + vertex2.normal.xyz * barycentrics.z);
  const bool is_front_face = dot(smooth_normal, gl_WorldRayDirectionEXT) < 0.0;

  payload.hit = 1u;
  payload.radiance = vec3(0.0);
  payload.hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  payload.front_face = is_front_face ? 1u : 0u;
  payload.normal = faceforward(smooth_normal, gl_WorldRayDirectionEXT, smooth_normal);
  payload.albedo = material.albedo.rgb;
  payload.roughness = material.albedo.a;
  payload.material_type = uint(material.emission.a + 0.5);
  payload.emission = material.emission.rgb;
  payload.ior = material.params.x;
}
