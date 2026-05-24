#include "app/AppConfig.hpp"
#include "engine/Engine.hpp"
#include "engine/EngineConfig.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

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

  engine.render();
  engine.render();

  CHECK(engine.frame_stats().frame_index == 2);
}

TEST_CASE("engine owns the configured procedural scene")
{
  vulkan_rt::engine::Engine engine{{}};

  CHECK_FALSE(engine.scene().empty());
  CHECK(engine.scene().materials().size() == 4);
  CHECK(engine.scene().triangles().size() == 12);
}
