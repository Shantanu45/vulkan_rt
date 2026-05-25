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

void main()
{
  payload.hit = 0u;
  payload.radiance = vec3(0.04, 0.06, 0.10);
  payload.hit_position = vec3(0.0);
  payload.normal = vec3(0.0, 1.0, 0.0);
  payload.albedo = vec3(0.0);
  payload.emission = vec3(0.0);
}
