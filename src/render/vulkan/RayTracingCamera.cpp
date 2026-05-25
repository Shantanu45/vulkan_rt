#include "RayTracingCamera.hpp"

namespace vulkan_rt::render::vulkan
{
namespace
{
struct GpuCamera
{
  float position_fov_y[4]{};
  float forward_aspect[4]{};
  float right[4]{};
  float up[4]{};
};

static_assert(sizeof(GpuCamera) == 64);

void write_vec3(float (&target)[4], scene::Vec3 source, float w)
{
  target[0] = source.x;
  target[1] = source.y;
  target[2] = source.z;
  target[3] = w;
}

GpuCamera make_gpu_camera(const scene::Camera &camera)
{
  GpuCamera gpu_camera{};
  write_vec3(gpu_camera.position_fov_y, camera.position(), camera.vertical_fov_degrees());
  write_vec3(gpu_camera.forward_aspect, camera.view_direction(), camera.aspect_ratio());
  write_vec3(gpu_camera.right, camera.right_direction(), 0.0F);
  write_vec3(gpu_camera.up, camera.up_direction(), 0.0F);
  return gpu_camera;
}
}

RayTracingCamera::RayTracingCamera(const VulkanAllocator &allocator, const scene::Camera &camera)
  : buffer_(allocator,
      BufferCreateInfo{
        .size = sizeof(GpuCamera),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_usage = VMA_MEMORY_USAGE_AUTO,
        .alloc_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
      })
{
  update(camera);
}

void RayTracingCamera::update(const scene::Camera &camera)
{
  const auto gpu_camera = make_gpu_camera(camera);
  auto *data = static_cast<GpuCamera *>(buffer_.map());
  *data = gpu_camera;
  buffer_.flush();
  buffer_.unmap();
}

const VulkanBuffer &RayTracingCamera::buffer() const
{
  return buffer_;
}
}
