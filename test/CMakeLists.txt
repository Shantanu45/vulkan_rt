include(Catch)

# Provide a simple smoke test to make sure that the CLI works and can display a --help message
add_test(NAME cli.has_help COMMAND ${PROJECT_NAME} --help)

# Provide a test to verify that the version being reported from the application
# matches the version given to CMake. This will be important once you package
# your program. Real world shows that this is the kind of simple mistake that is easy
# to make, but also easy to test for.
add_test(NAME cli.version_matches COMMAND ${PROJECT_NAME} --version)
set_tests_properties(cli.version_matches PROPERTIES PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

add_test(
  NAME cli.dry_run_config
  COMMAND ${PROJECT_NAME}
          --dry-run-config
          --width 1600
          --height 900
          --validation
          --gpu 0)
set_tests_properties(cli.dry_run_config PROPERTIES PASS_REGULAR_EXPRESSION "1600x900")

option(VULKAN_RT_ENABLE_VULKAN_CONTEXT_TESTS "Enable Vulkan context integration tests" OFF)

if(VULKAN_RT_ENABLE_VULKAN_CONTEXT_TESTS)
  add_test(
    NAME vulkan.context_smoke
    COMMAND ${PROJECT_NAME} --vulkan-context-smoke --width 64 --height 64)
  add_test(
    NAME vulkan.device_smoke
    COMMAND ${PROJECT_NAME} --vulkan-device-smoke --width 64 --height 64)
  add_test(
    NAME vulkan.swapchain_smoke
    COMMAND ${PROJECT_NAME} --vulkan-swapchain-smoke --width 64 --height 64)
  add_test(
    NAME vulkan.frame_smoke
    COMMAND ${PROJECT_NAME} --vulkan-frame-smoke --width 64 --height 64)
  add_test(
    NAME vulkan.shader_smoke
    COMMAND ${PROJECT_NAME} --vulkan-shader-smoke --width 64 --height 64)
  add_test(
    NAME vulkan.clear_smoke
    COMMAND ${PROJECT_NAME} --vulkan-clear-smoke --width 64 --height 64)
  add_test(
    NAME vulkan.resize_smoke
    COMMAND ${PROJECT_NAME} --vulkan-resize-smoke --width 64 --height 64)
endif()

add_executable(tests
    camera_tests.cpp
    engine_tests.cpp
    render_tests.cpp
    scene_tests.cpp
    test_main.cpp
)

target_link_libraries(
  tests
  PRIVATE vulkan_rt::vulkan_rt_warnings
          vulkan_rt::vulkan_rt_options
          Catch2::Catch2WithMain
          fmt::fmt
          spdlog::spdlog)

target_sources(tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src/engine/Engine.cpp"
    "${CMAKE_SOURCE_DIR}/src/engine/EngineConfig.cpp"
    "${CMAKE_SOURCE_DIR}/src/render/NullRenderer.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/Camera.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/Scene.cpp"
    "${CMAKE_SOURCE_DIR}/src/util/logger.cpp"
)

vulkan_rt_configure_linker(tests)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(
    TARGET tests
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:tests> $<TARGET_FILE_DIR:tests>
    COMMAND_EXPAND_LISTS)
endif()

target_include_directories(tests PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/configured_files/include"
    "${CMAKE_SOURCE_DIR}/src"
)

catch_discover_tests(tests)
