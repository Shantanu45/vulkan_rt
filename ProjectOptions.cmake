include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


include(CheckCXXSourceCompiles)


macro(vulkan_rt_supports_sanitizers)
  # Emscripten doesn't support sanitizers
  if(EMSCRIPTEN)
    set(SUPPORTS_UBSAN OFF)
    set(SUPPORTS_ASAN OFF)
  elseif((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)

    message(STATUS "Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform")
    set(TEST_PROGRAM "int main() { return 0; }")

    # Check if UndefinedBehaviorSanitizer works at link time
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
    check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

    if(HAS_UBSAN_LINK_SUPPORT)
      message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
      set(SUPPORTS_UBSAN ON)
    else()
      message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
      set(SUPPORTS_UBSAN OFF)
    endif()
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if(MSVC)
    set(SUPPORTS_ASAN OFF)
  elseif((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    if (NOT WIN32)
      message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
      set(TEST_PROGRAM "int main() { return 0; }")

      # Check if AddressSanitizer works at link time
      set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
      set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
      check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

      if(HAS_ASAN_LINK_SUPPORT)
        message(STATUS "AddressSanitizer is supported at both compile and link time.")
        set(SUPPORTS_ASAN ON)
      else()
        message(WARNING "AddressSanitizer is NOT supported at link time.")
        set(SUPPORTS_ASAN OFF)
      endif()
    else()
      set(SUPPORTS_ASAN ON)
    endif()
  endif()
endmacro()

macro(vulkan_rt_setup_options)
  option(vulkan_rt_ENABLE_HARDENING "Enable hardening" ON)
  option(vulkan_rt_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    vulkan_rt_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    vulkan_rt_ENABLE_HARDENING
    OFF)

  vulkan_rt_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR vulkan_rt_PACKAGING_MAINTAINER_MODE)
    option(vulkan_rt_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(vulkan_rt_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(vulkan_rt_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(vulkan_rt_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(vulkan_rt_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(vulkan_rt_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(vulkan_rt_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(vulkan_rt_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(vulkan_rt_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(vulkan_rt_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(vulkan_rt_ENABLE_PCH "Enable precompiled headers" OFF)
    option(vulkan_rt_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(vulkan_rt_ENABLE_IPO "Enable IPO/LTO" ON)
    option(vulkan_rt_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(vulkan_rt_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(vulkan_rt_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(vulkan_rt_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(vulkan_rt_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(vulkan_rt_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(vulkan_rt_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(vulkan_rt_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(vulkan_rt_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(vulkan_rt_ENABLE_PCH "Enable precompiled headers" OFF)
    option(vulkan_rt_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      vulkan_rt_ENABLE_IPO
      vulkan_rt_WARNINGS_AS_ERRORS
      vulkan_rt_ENABLE_SANITIZER_ADDRESS
      vulkan_rt_ENABLE_SANITIZER_LEAK
      vulkan_rt_ENABLE_SANITIZER_UNDEFINED
      vulkan_rt_ENABLE_SANITIZER_THREAD
      vulkan_rt_ENABLE_SANITIZER_MEMORY
      vulkan_rt_ENABLE_UNITY_BUILD
      vulkan_rt_ENABLE_CLANG_TIDY
      vulkan_rt_ENABLE_CPPCHECK
      vulkan_rt_ENABLE_COVERAGE
      vulkan_rt_ENABLE_PCH
      vulkan_rt_ENABLE_CACHE)
  endif()

  vulkan_rt_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (vulkan_rt_ENABLE_SANITIZER_ADDRESS OR vulkan_rt_ENABLE_SANITIZER_THREAD OR vulkan_rt_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(vulkan_rt_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

endmacro()

macro(vulkan_rt_global_options)
  if(vulkan_rt_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    vulkan_rt_enable_ipo()
  endif()

  vulkan_rt_supports_sanitizers()

  if(vulkan_rt_ENABLE_HARDENING AND vulkan_rt_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR vulkan_rt_ENABLE_SANITIZER_UNDEFINED
       OR vulkan_rt_ENABLE_SANITIZER_ADDRESS
       OR vulkan_rt_ENABLE_SANITIZER_THREAD
       OR vulkan_rt_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    vulkan_rt_enable_hardening(vulkan_rt_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(vulkan_rt_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(vulkan_rt_warnings INTERFACE)
  add_library(vulkan_rt_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  vulkan_rt_set_project_warnings(
    vulkan_rt_warnings
    ${vulkan_rt_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  include(cmake/Linker.cmake)
  # Must configure each target with linker options, we're avoiding setting it globally for now

  if(NOT EMSCRIPTEN)
    include(cmake/Sanitizers.cmake)
    vulkan_rt_enable_sanitizers(
      vulkan_rt_options
      ${vulkan_rt_ENABLE_SANITIZER_ADDRESS}
      ${vulkan_rt_ENABLE_SANITIZER_LEAK}
      ${vulkan_rt_ENABLE_SANITIZER_UNDEFINED}
      ${vulkan_rt_ENABLE_SANITIZER_THREAD}
      ${vulkan_rt_ENABLE_SANITIZER_MEMORY})
  endif()

  set_target_properties(vulkan_rt_options PROPERTIES UNITY_BUILD ${vulkan_rt_ENABLE_UNITY_BUILD})

  if(vulkan_rt_ENABLE_PCH)
    target_precompile_headers(
      vulkan_rt_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(vulkan_rt_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    vulkan_rt_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(vulkan_rt_ENABLE_CLANG_TIDY)
    vulkan_rt_enable_clang_tidy(vulkan_rt_options ${vulkan_rt_WARNINGS_AS_ERRORS})
  endif()

  if(vulkan_rt_ENABLE_CPPCHECK)
    vulkan_rt_enable_cppcheck(${vulkan_rt_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(vulkan_rt_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    vulkan_rt_enable_coverage(vulkan_rt_options)
  endif()

  if(vulkan_rt_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(vulkan_rt_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(vulkan_rt_ENABLE_HARDENING AND NOT vulkan_rt_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR vulkan_rt_ENABLE_SANITIZER_UNDEFINED
       OR vulkan_rt_ENABLE_SANITIZER_ADDRESS
       OR vulkan_rt_ENABLE_SANITIZER_THREAD
       OR vulkan_rt_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    vulkan_rt_enable_hardening(vulkan_rt_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
