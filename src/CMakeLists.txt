set(util
    util/error_list.cpp
    util/error_list.h
    util/error_macros.cpp
    util/error_macros.h
    util/hash.h
    util/hash_func.h
    util/logger.cpp
    util/logger.h
    util/renderdoc_helpers.h
    util/rid.h
    util/rid_owner.h
    util/string_helpers.cpp
    util/string_helpers.h
    util/timer.cpp
    util/timer.h
    util/_mutex.h
    util/typedefs.h
)

source_group("util" FILES ${util})

set(app
  main.cpp
  app/AppConfig.cpp
  app/AppConfig.hpp
  app/Application.cpp
  app/Application.hpp
  app/Input.cpp
  app/Input.hpp
  app/SdlSurfaceProvider.cpp
  app/SdlSurfaceProvider.hpp
  app/UiLayer.cpp
  app/UiLayer.hpp
  app/VulkanContextSmoke.cpp
  app/VulkanContextSmoke.hpp
  app/Window.cpp
  app/Window.hpp
)

source_group("app" FILES ${app})

set(engine
    engine/Engine.cpp
    engine/Engine.hpp
    engine/EngineConfig.cpp
    engine/EngineConfig.hpp
    engine/FrameStats.hpp
)

source_group("engine" FILES ${engine})

set(render
    render/NullRenderer.cpp
    render/NullRenderer.hpp
    render/Renderer.hpp
)

source_group("render" FILES ${render})

set(vulkan
    render/vulkan/AccelerationStructure.hpp
    render/vulkan/AccelerationStructure.cpp
    render/vulkan/OneShotCommandBuffer.hpp
    render/vulkan/OneShotCommandBuffer.cpp
    render/vulkan/VulkanCheck.hpp
    render/vulkan/VulkanCheck.cpp
    render/vulkan/VulkanRendererConfig.hpp
    render/vulkan/VulkanRenderer.hpp
    render/vulkan/VulkanRenderer.cpp
    render/vulkan/RayTracingCamera.hpp
    render/vulkan/RayTracingCamera.cpp
    render/vulkan/RayTracingCameraData.hpp
    render/vulkan/RayTracingScene.hpp
    render/vulkan/RayTracingScene.cpp
    render/vulkan/RayTracingPipeline.hpp
    render/vulkan/RayTracingPipeline.cpp
    render/vulkan/RayTracingDescriptorSet.hpp
    render/vulkan/RayTracingDescriptorSet.cpp
    render/vulkan/ShaderBindingTable.hpp
    render/vulkan/ShaderBindingTable.cpp
    render/vulkan/ShaderModule.hpp
    render/vulkan/ShaderModule.cpp
    render/vulkan/VulkanAllocator.hpp
    render/vulkan/VulkanAllocator.cpp
    render/vulkan/VulkanBuffer.hpp
    render/vulkan/VulkanBuffer.cpp
    render/vulkan/VulkanImage.hpp
    render/vulkan/VulkanImage.cpp
    render/vulkan/VulkanContext.hpp
    render/vulkan/VulkanContext.cpp
    render/vulkan/VulkanDevice.hpp
    render/vulkan/VulkanDevice.cpp
    render/vulkan/VulkanSwapchain.hpp
    render/vulkan/VulkanSwapchain.cpp
    render/vulkan/VulkanFrameResources.hpp
    render/vulkan/VulkanFrameResources.cpp
    render/vulkan/SurfaceProvider.hpp
)

source_group("render/vulkan" FILES ${vulkan})

set(scene
    scene/Camera.cpp
    scene/Camera.hpp
    scene/Scene.cpp
    scene/Scene.hpp
)

source_group("scene" FILES ${scene})

find_package(Vulkan REQUIRED COMPONENTS glslc)

set(vulkan_rt_shader_sources
    "${CMAKE_SOURCE_DIR}/shaders/raygen.rgen"
    "${CMAKE_SOURCE_DIR}/shaders/miss.rmiss"
    "${CMAKE_SOURCE_DIR}/shaders/closesthit.rchit"
)

source_group("shaders" FILES ${vulkan_rt_shader_sources})

set(vulkan_rt_compiled_shaders)

function(vulkan_rt_compile_shader shader_source shader_stage)
  get_filename_component(shader_filename "${shader_source}" NAME)
  set(shader_output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_filename}.spv")

  add_custom_command(
    OUTPUT "${shader_output}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMAND Vulkan::glslc
            --target-env=vulkan1.3
            "-fshader-stage=${shader_stage}"
            "${shader_source}"
            -o "${shader_output}"
    DEPENDS "${shader_source}"
    COMMENT "Compiling ${shader_filename} to SPIR-V"
    VERBATIM)

  set(vulkan_rt_compiled_shaders
      ${vulkan_rt_compiled_shaders}
      "${shader_output}"
      PARENT_SCOPE)
endfunction()

vulkan_rt_compile_shader("${CMAKE_SOURCE_DIR}/shaders/raygen.rgen" rgen)
vulkan_rt_compile_shader("${CMAKE_SOURCE_DIR}/shaders/miss.rmiss" rmiss)
vulkan_rt_compile_shader("${CMAKE_SOURCE_DIR}/shaders/closesthit.rchit" rchit)

add_custom_target(vulkan_rt_shaders
  DEPENDS ${vulkan_rt_compiled_shaders}
  SOURCES ${vulkan_rt_shader_sources})
set_target_properties(vulkan_rt_shaders PROPERTIES FOLDER "assets")

add_executable(${PROJECT_NAME}
${app}
${engine}
${render}
${vulkan}
${scene}
${util}
${vulkan_rt_shader_sources}
)

add_dependencies(${PROJECT_NAME} vulkan_rt_shaders)

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
find_package(libassert REQUIRED)
find_package(sdl3 REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE vulkan_rt::vulkan_rt_options
          vulkan_rt::vulkan_rt_warnings
          CLI11::CLI11
          fmt::fmt
          spdlog::spdlog
          Vulkan::Headers
          volk::volk
          libassert::assert
          SDL3::SDL3
          GPUOpen::VulkanMemoryAllocator
          ${vulkan_rt_sdl_target})

target_compile_definitions(${PROJECT_NAME} PRIVATE
  VMA_STATIC_VULKAN_FUNCTIONS=0
  VMA_DYNAMIC_VULKAN_FUNCTIONS=1
)


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
