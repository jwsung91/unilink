# Unilink library target construction and shared target utilities.

function(unilink_configure_library_target target)
  target_link_libraries(${target} PUBLIC unilink_dependencies)
  target_compile_features(${target} PUBLIC cxx_std_20)
  target_include_directories(
    ${target} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                     $<INSTALL_INTERFACE:include>
  )
endfunction()

function(unilink_configure_shared_target target output_name)
  set_target_properties(
    ${target}
    PROPERTIES VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION_MAJOR}
               OUTPUT_NAME "${output_name}"
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN YES
               WINDOWS_EXPORT_ALL_SYMBOLS OFF
  )
  target_compile_definitions(
    ${target}
    PUBLIC UNILINK_BUILD_SHARED
    PRIVATE UNILINK_BUILDING_LIBRARY
  )
  unilink_configure_library_target(${target})
endfunction()

function(unilink_configure_static_target target output_name)
  set_target_properties(
    ${target}
    PROPERTIES OUTPUT_NAME "${output_name}"
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN YES
  )
  target_compile_definitions(${target} PUBLIC UNILINK_BUILD_STATIC)
  unilink_configure_library_target(${target})
endfunction()

function(unilink_add_library_targets)
  set(_unilink_library_targets)
  set(_unilink_export_header_target)
  set(_unilink_shared_library_target)

  if(UNILINK_BUILD_SHARED AND UNILINK_BUILD_STATIC)
    add_library(unilink_shared SHARED ${UNILINK_SOURCES} ${UNILINK_HEADERS})
    add_library(unilink_static STATIC ${UNILINK_SOURCES} ${UNILINK_HEADERS})

    unilink_configure_shared_target(unilink_shared "unilink")
    unilink_configure_static_target(unilink_static "unilink_static")

    add_library(unilink INTERFACE)
    target_link_libraries(
      unilink
      INTERFACE $<$<TARGET_EXISTS:unilink_shared>:unilink_shared>
                $<$<NOT:$<TARGET_EXISTS:unilink_shared>>:unilink_static>
                unilink_dependencies
    )

    list(APPEND _unilink_library_targets unilink unilink_shared unilink_static)
    set(_unilink_export_header_target unilink_shared)
    set(_unilink_shared_library_target unilink_shared)

  elseif(UNILINK_BUILD_SHARED)
    add_library(unilink SHARED ${UNILINK_SOURCES} ${UNILINK_HEADERS})
    unilink_configure_shared_target(unilink "unilink")

    list(APPEND _unilink_library_targets unilink)
    set(_unilink_export_header_target unilink)
    set(_unilink_shared_library_target unilink)

  elseif(UNILINK_BUILD_STATIC)
    add_library(unilink STATIC ${UNILINK_SOURCES} ${UNILINK_HEADERS})
    unilink_configure_static_target(unilink "unilink")

    list(APPEND _unilink_library_targets unilink)
    set(_unilink_export_header_target unilink)

  else()
    message(
      FATAL_ERROR "At least one library type (shared or static) must be enabled"
    )
  endif()

  if(TARGET unilink_shared)
    add_library(unilink::unilink_shared ALIAS unilink_shared)
  endif()
  if(TARGET unilink_static)
    add_library(unilink::unilink_static ALIAS unilink_static)
  endif()
  if(TARGET unilink)
    add_library(unilink::unilink ALIAS unilink)
  endif()

  set(UNILINK_LIBRARY_TARGETS
      ${_unilink_library_targets}
      PARENT_SCOPE
  )
  set(UNILINK_EXPORT_HEADER_TARGET
      ${_unilink_export_header_target}
      PARENT_SCOPE
  )
  set(UNILINK_SHARED_LIBRARY_TARGET
      ${_unilink_shared_library_target}
      PARENT_SCOPE
  )
endfunction()

function(unilink_configure_executable target)
  if(WIN32
     AND UNILINK_BUILD_SHARED
     AND UNILINK_SHARED_LIBRARY_TARGET
  )
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${UNILINK_SHARED_LIBRARY_TARGET}>
        $<TARGET_FILE_DIR:${target}>
      COMMENT
        "Copying ${UNILINK_SHARED_LIBRARY_TARGET} runtime to ${target} output directory"
    )
  endif()
endfunction()
