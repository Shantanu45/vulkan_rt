#include "render/NullRenderer.hpp"
#include "render/Renderer.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("null renderer accepts renderer lifecycle calls")
{
  vulkan_rt::render::NullRenderer renderer;

  CHECK_NOTHROW(renderer.resize(1280, 720));
  CHECK_NOTHROW(renderer.render(vulkan_rt::render::RenderFrameInfo{
    .frame_index = 42,
    .frame_time_ms = 16.6,
    .fps = 60.0,
  }));
  CHECK_NOTHROW(renderer.wait_idle());
}
