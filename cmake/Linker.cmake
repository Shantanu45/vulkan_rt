macro(vulkan_rt_configure_linker project_name)
  set(vulkan_rt_USER_LINKER_OPTION
    "DEFAULT"
      CACHE STRING "Linker to be used")
    set(vulkan_rt_USER_LINKER_OPTION_VALUES "DEFAULT" "SYSTEM" "LLD" "GOLD" "BFD" "MOLD" "SOLD" "APPLE_CLASSIC" "MSVC")
  set_property(CACHE vulkan_rt_USER_LINKER_OPTION PROPERTY STRINGS ${vulkan_rt_USER_LINKER_OPTION_VALUES})
  list(
    FIND
    vulkan_rt_USER_LINKER_OPTION_VALUES
    ${vulkan_rt_USER_LINKER_OPTION}
    vulkan_rt_USER_LINKER_OPTION_INDEX)

  if(${vulkan_rt_USER_LINKER_OPTION_INDEX} EQUAL -1)
    message(
      STATUS
        "Using custom linker: '${vulkan_rt_USER_LINKER_OPTION}', explicitly supported entries are ${vulkan_rt_USER_LINKER_OPTION_VALUES}")
  endif()

  set_target_properties(${project_name} PROPERTIES LINKER_TYPE "${vulkan_rt_USER_LINKER_OPTION}")
endmacro()
