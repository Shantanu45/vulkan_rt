#include <catch2/catch_test_macros.hpp>
#include <internal_use_only/config.hpp>

TEST_CASE("configured project metadata is available")
{
  CHECK(vulkan_rt::cmake::project_name.size() > 0);
  CHECK(vulkan_rt::cmake::project_version.size() > 0);
}

