#include "app/AppConfig.hpp"
#include "engine/Engine.hpp"
#include "engine/EngineConfig.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

namespace
{
class SpyRenderer final : public vulkan_rt::render::Renderer
{
public:
  void render(
    const vulkan_rt::render::RenderFrameInfo &frame_info,
    const vulkan_rt::scene::Scene &scene,
    const vulkan_rt::scene::Camera &camera) override
  {
    static_cast<void>(scene);
    static_cast<void>(camera);
    reset_accumulation_frames.push_back(frame_info.reset_accumulation);
  }

  void resize(int width, int height) override
  {
    static_cast<void>(width);
    static_cast<void>(height);
  }

  void wait_idle() override {}

  std::vector<bool> reset_accumulation_frames;
};
}

TEST_CASE("engine config is derived from app config")
{
  vulkan_rt::app::AppConfig app_config;
  app_config.validation = true;
  app_config.gpu_index = 2;
  app_config.scene = "procedural";

  const auto engine_config = vulkan_rt::engine::make_engine_config(app_config);

  CHECK(engine_config.validation);
  CHECK(engine_config.gpu_index == 2);
  CHECK(engine_config.scene == "procedural");
}

TEST_CASE("engine update records frame timing")
{
  vulkan_rt::engine::Engine engine{{}};

  engine.update(0.25);

  CHECK(engine.frame_stats().frame_time_ms == Catch::Approx(250.0));
  CHECK(engine.frame_stats().fps == Catch::Approx(4.0));
}

TEST_CASE("engine render increments frame index")
{
  vulkan_rt::engine::Engine engine{{}};

  CHECK(engine.frame_stats().frame_index == 0);
  CHECK(engine.frame_stats().accumulated_sample_count == 0);

  engine.render();
  engine.render();

  CHECK(engine.frame_stats().frame_index == 2);
  CHECK(engine.frame_stats().accumulated_sample_count == 2);
}

TEST_CASE("engine owns the configured procedural scene")
{
  vulkan_rt::engine::Engine engine{{}};

  CHECK_FALSE(engine.scene().empty());
  CHECK(engine.scene().materials().size() == 6);
  CHECK(engine.scene().triangles().size() == 36);
}

TEST_CASE("engine updates camera aspect ratio on resize")
{
  vulkan_rt::engine::Engine engine{{}};

  engine.resize(800, 600);

  CHECK(engine.camera().aspect_ratio() == Catch::Approx(4.0F / 3.0F));
}

TEST_CASE("engine moves camera from camera control input")
{
  vulkan_rt::engine::Engine engine{{}};
  const auto before = engine.camera().position();

  const bool changed = engine.update_camera(
    vulkan_rt::engine::CameraControlInput{
      .move_forward = true,
    },
    1.0);

  const auto after = engine.camera().position();
  CHECK(changed);
  CHECK(after.z != Catch::Approx(before.z));
}

TEST_CASE("engine uses configured camera movement speed")
{
  vulkan_rt::engine::Engine engine{{}};
  vulkan_rt::engine::CameraSettings settings = engine.camera_settings();
  settings.move_speed = 6.0F;
  engine.set_camera_settings(settings);

  const auto before = engine.camera().position();
  const bool changed = engine.update_camera(
    vulkan_rt::engine::CameraControlInput{
      .move_forward = true,
    },
    0.5);

  const auto after = engine.camera().position();
  CHECK(changed);
  CHECK(before.z - after.z == Catch::Approx(3.0F));
}

TEST_CASE("engine rotates camera from mouse delta when rotation is active")
{
  vulkan_rt::engine::Engine engine{{}};

  const bool changed = engine.update_camera(
    vulkan_rt::engine::CameraControlInput{
      .rotate = true,
      .mouse_delta_x = 20.0F,
      .mouse_delta_y = -10.0F,
    },
    1.0);

  CHECK(changed);
  CHECK(engine.camera().yaw_degrees() == Catch::Approx(-88.0F));
  CHECK(engine.camera().pitch_degrees() == Catch::Approx(1.0F));
}

TEST_CASE("engine applies camera fov settings")
{
  vulkan_rt::engine::Engine engine{{}};
  vulkan_rt::engine::CameraSettings settings = engine.camera_settings();
  settings.vertical_fov_degrees = 70.0F;

  engine.set_camera_settings(settings);

  CHECK(engine.camera().vertical_fov_degrees() == Catch::Approx(70.0F));
  CHECK(engine.camera_settings().vertical_fov_degrees == Catch::Approx(70.0F));
}

TEST_CASE("engine requests accumulation reset for startup and camera changes")
{
  vulkan_rt::engine::Engine engine{{}};
  auto renderer = std::make_unique<SpyRenderer>();
  auto *spy = renderer.get();
  engine.set_renderer(std::move(renderer));

  engine.render();
  engine.render();

  REQUIRE(spy->reset_accumulation_frames.size() == 2);
  CHECK(spy->reset_accumulation_frames[0]);
  CHECK_FALSE(spy->reset_accumulation_frames[1]);

  engine.update_camera(
    vulkan_rt::engine::CameraControlInput{
      .move_forward = true,
    },
    1.0);
  engine.render();

  REQUIRE(spy->reset_accumulation_frames.size() == 3);
  CHECK(spy->reset_accumulation_frames[2]);
}

TEST_CASE("engine requests accumulation reset for fov changes")
{
  vulkan_rt::engine::Engine engine{{}};
  auto renderer = std::make_unique<SpyRenderer>();
  auto *spy = renderer.get();
  engine.set_renderer(std::move(renderer));

  engine.render();
  engine.render();

  vulkan_rt::engine::CameraSettings settings = engine.camera_settings();
  settings.vertical_fov_degrees = 80.0F;
  engine.set_camera_settings(settings);
  engine.render();

  REQUIRE(spy->reset_accumulation_frames.size() == 3);
  CHECK_FALSE(spy->reset_accumulation_frames[1]);
  CHECK(spy->reset_accumulation_frames[2]);
  CHECK(engine.frame_stats().accumulated_sample_count == 1);
}
