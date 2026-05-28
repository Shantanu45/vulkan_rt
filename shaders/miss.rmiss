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

void main()
{
  payload.hit = 0u;
  payload.radiance = vec3(0.04, 0.06, 0.10);
  payload.hit_position = vec3(0.0);
  payload.front_face = 1u;
  payload.normal = vec3(0.0, 1.0, 0.0);
  payload.roughness = 1.0;
  payload.albedo = vec3(0.0);
  payload.material_type = 0u;
  payload.emission = vec3(0.0);
  payload.ior = 1.5;
}
