set(app
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

source_group("app" FILES ${app})

set(vulkan
    render/vulkan/VulkanCheck.hpp
    render/vulkan/VulkanCheck.cpp
)

source_group("render/vulkan" FILES ${vulkan})


source_group("app" FILES ${app})

add_executable(${PROJECT_NAME}
${app}
${vulkan}
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

find_package(volk REQUIRED)
find_package(VulkanHeaders REQUIRED)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE vulkan_rt::vulkan_rt_options
          vulkan_rt::vulkan_rt_warnings
          CLI11::CLI11
          fmt::fmt
          spdlog::spdlog
          vulkan-headers::vulkan-headers
          volk::volk
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
