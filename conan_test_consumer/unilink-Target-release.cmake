# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(unilink_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(unilink_FRAMEWORKS_FOUND_RELEASE "${unilink_FRAMEWORKS_RELEASE}" "${unilink_FRAMEWORK_DIRS_RELEASE}")

set(unilink_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET unilink_DEPS_TARGET)
    add_library(unilink_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET unilink_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${unilink_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${unilink_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### unilink_DEPS_TARGET to all of them
conan_package_library_targets("${unilink_LIBS_RELEASE}"    # libraries
                              "${unilink_LIB_DIRS_RELEASE}" # package_libdir
                              "${unilink_BIN_DIRS_RELEASE}" # package_bindir
                              "${unilink_LIBRARY_TYPE_RELEASE}"
                              "${unilink_IS_HOST_WINDOWS_RELEASE}"
                              unilink_DEPS_TARGET
                              unilink_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "unilink"    # package_name
                              "${unilink_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${unilink_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT unilink::unilink #############

        set(unilink_unilink_unilink_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(unilink_unilink_unilink_FRAMEWORKS_FOUND_RELEASE "${unilink_unilink_unilink_FRAMEWORKS_RELEASE}" "${unilink_unilink_unilink_FRAMEWORK_DIRS_RELEASE}")

        set(unilink_unilink_unilink_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET unilink_unilink_unilink_DEPS_TARGET)
            add_library(unilink_unilink_unilink_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET unilink_unilink_unilink_DEPS_TARGET
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_DEPENDENCIES_RELEASE}>
                     )

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'unilink_unilink_unilink_DEPS_TARGET' to all of them
        conan_package_library_targets("${unilink_unilink_unilink_LIBS_RELEASE}"
                              "${unilink_unilink_unilink_LIB_DIRS_RELEASE}"
                              "${unilink_unilink_unilink_BIN_DIRS_RELEASE}" # package_bindir
                              "${unilink_unilink_unilink_LIBRARY_TYPE_RELEASE}"
                              "${unilink_unilink_unilink_IS_HOST_WINDOWS_RELEASE}"
                              unilink_unilink_unilink_DEPS_TARGET
                              unilink_unilink_unilink_LIBRARIES_TARGETS
                              "_RELEASE"
                              "unilink_unilink_unilink"
                              "${unilink_unilink_unilink_NO_SONAME_MODE_RELEASE}")


        ########## TARGET PROPERTIES #####################################
        set_property(TARGET unilink::unilink
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_LIBRARIES_TARGETS}>
                     )

        if("${unilink_unilink_unilink_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET unilink::unilink
                         APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                         unilink_unilink_unilink_DEPS_TARGET)
        endif()

        set_property(TARGET unilink::unilink APPEND PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_LINKER_FLAGS_RELEASE}>)
        set_property(TARGET unilink::unilink APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_INCLUDE_DIRS_RELEASE}>)
        set_property(TARGET unilink::unilink APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_LIB_DIRS_RELEASE}>)
        set_property(TARGET unilink::unilink APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_COMPILE_DEFINITIONS_RELEASE}>)
        set_property(TARGET unilink::unilink APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${unilink_unilink_unilink_COMPILE_OPTIONS_RELEASE}>)


    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET unilink::unilink APPEND PROPERTY INTERFACE_LINK_LIBRARIES unilink::unilink)

########## For the modules (FindXXX)
set(unilink_LIBRARIES_RELEASE unilink::unilink)
