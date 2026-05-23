add_executable(${PROJECT_NAME}
  main.cpp
  app/AppConfig.cpp
  app/AppConfig.hpp
  app/Application.cpp
  app/Application.hpp
  app/Input.cpp
  app/Input.hpp
  app/UiLayer.cpp
  app/UiLayer.hpp
  app/Window.cpp
  app/Window.hpp
)

if(TARGET SDL3::SDL3)
  set(vulkan_rt_sdl_target SDL3::SDL3)
elseif(TARGET SDL3::SDL3-shared)
  set(vulkan_rt_sdl_target SDL3::SDL3-shared)
elseif(TARGET SDL3::SDL3-static)
  set(vulkan_rt_sdl_target SDL3::SDL3-static)
else()
  message(FATAL_ERROR "SDL3 package was found, but no supported SDL3 CMake target is available.")
endif()

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE vulkan_rt::vulkan_rt_options
          vulkan_rt::vulkan_rt_warnings
          CLI11::CLI11
          fmt::fmt
          spdlog::spdlog
          ${vulkan_rt_sdl_target})


vulkan_rt_configure_linker(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_BINARY_DIR}/configured_files/include"
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/src")

# Apply WASM-specific configuration
if(EMSCRIPTEN)
  vulkan_rt_configure_wasm_target(${PROJECT_NAME}
    TITLE "${PROJECT_NAME}"
    DESCRIPTION "${PROJECT_DESCRIPTION}"
  )
endif()
