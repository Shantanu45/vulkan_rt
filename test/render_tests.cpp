#include "render/NullRenderer.hpp"
#include "render/Renderer.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("null renderer accepts renderer lifecycle calls")
{
  vulkan_rt::render::NullRenderer renderer;
  const vulkan_rt::scene::Camera camera;
  const auto scene = vulkan_rt::scene::make_procedural_scene();

  CHECK_NOTHROW(renderer.resize(1280, 720));
  CHECK_NOTHROW(renderer.render(vulkan_rt::render::RenderFrameInfo{
                                   .frame_index = 42,
                                   .frame_time_ms = 16.6,
                                   .fps = 60.0,
                                   .reset_accumulation = true,
                                 },
    scene,
    camera));
  CHECK_NOTHROW(renderer.wait_idle());
}
