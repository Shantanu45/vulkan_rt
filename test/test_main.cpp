#include <catch2/catch_test_macros.hpp>
#include <internal_use_only/config.hpp>
#include <vulkan_rt/sample_library.hpp>

TEST_CASE("configured project metadata is available")
{
  CHECK(vulkan_rt::cmake::project_name.size() > 0);
  CHECK(vulkan_rt::cmake::project_version.size() > 0);
}

TEST_CASE("sample constexpr code works")
{
  STATIC_REQUIRE(factorial_constexpr(0) == 1);
  STATIC_REQUIRE(factorial_constexpr(5) == 120);
}
