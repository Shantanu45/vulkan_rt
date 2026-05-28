function(vulkan_rt_setup_dependencies)
  find_package(fmt CONFIG REQUIRED)
  find_package(spdlog CONFIG REQUIRED)
  find_package(Catch2 CONFIG REQUIRED)
  find_package(CLI11 CONFIG REQUIRED)
  find_package(SDL3 CONFIG REQUIRED)
  find_package(imgui CONFIG REQUIRED)
  find_package(TinyGLTF CONFIG REQUIRED)
  find_package(nlohmann_json CONFIG REQUIRED)

  # Group all third-party targets into a single IDE filter when the imported
  # targets expose enough metadata for IDEs to show them.
  foreach(_target fmt spdlog spdlog_header_only Catch2 Catch2WithMain CLI11 SDL3::SDL3 SDL3::SDL3-shared SDL3::SDL3-static imgui::imgui TinyGLTF::TinyGLTF nlohmann_json::nlohmann_json)
    if(TARGET ${_target})
      set_target_properties(${_target} PROPERTIES FOLDER "thirdparty")
    endif()
  endforeach()
endfunction()
