add_executable(${PROJECT_NAME} main.cpp
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE vulkan_rt::vulkan_rt_options
          vulkan_rt::vulkan_rt_warnings
          CLI11::CLI11
          fmt::fmt
          spdlog::spdlog)


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
