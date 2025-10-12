########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND unilink_COMPONENT_NAMES unilink::unilink)
list(REMOVE_DUPLICATES unilink_COMPONENT_NAMES)
if(DEFINED unilink_FIND_DEPENDENCY_NAMES)
  list(APPEND unilink_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES unilink_FIND_DEPENDENCY_NAMES)
else()
  set(unilink_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(unilink_PACKAGE_FOLDER_RELEASE "/home/adam/.conan2/p/b/unili93617bb4a7865/p")
set(unilink_BUILD_MODULES_PATHS_RELEASE )


set(unilink_INCLUDE_DIRS_RELEASE "${unilink_PACKAGE_FOLDER_RELEASE}/include")
set(unilink_RES_DIRS_RELEASE )
set(unilink_DEFINITIONS_RELEASE "-DUNILINK_ENABLE_CONFIG=1"
			"-DUNILINK_ENABLE_MEMORY_TRACKING=1")
set(unilink_SHARED_LINK_FLAGS_RELEASE )
set(unilink_EXE_LINK_FLAGS_RELEASE )
set(unilink_OBJECTS_RELEASE )
set(unilink_COMPILE_DEFINITIONS_RELEASE "UNILINK_ENABLE_CONFIG=1"
			"UNILINK_ENABLE_MEMORY_TRACKING=1")
set(unilink_COMPILE_OPTIONS_C_RELEASE )
set(unilink_COMPILE_OPTIONS_CXX_RELEASE -std=c++17)
set(unilink_LIB_DIRS_RELEASE "${unilink_PACKAGE_FOLDER_RELEASE}/lib")
set(unilink_BIN_DIRS_RELEASE "${unilink_PACKAGE_FOLDER_RELEASE}/bin")
set(unilink_LIBRARY_TYPE_RELEASE SHARED)
set(unilink_IS_HOST_WINDOWS_RELEASE 0)
set(unilink_LIBS_RELEASE unilink)
set(unilink_SYSTEM_LIBS_RELEASE pthread)
set(unilink_FRAMEWORK_DIRS_RELEASE )
set(unilink_FRAMEWORKS_RELEASE )
set(unilink_BUILD_DIRS_RELEASE )
set(unilink_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(unilink_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${unilink_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${unilink_COMPILE_OPTIONS_C_RELEASE}>")
set(unilink_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${unilink_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${unilink_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${unilink_EXE_LINK_FLAGS_RELEASE}>")


set(unilink_COMPONENTS_RELEASE unilink::unilink)
########### COMPONENT unilink::unilink VARIABLES ############################################

set(unilink_unilink_unilink_INCLUDE_DIRS_RELEASE "${unilink_PACKAGE_FOLDER_RELEASE}/include")
set(unilink_unilink_unilink_LIB_DIRS_RELEASE "${unilink_PACKAGE_FOLDER_RELEASE}/lib")
set(unilink_unilink_unilink_BIN_DIRS_RELEASE "${unilink_PACKAGE_FOLDER_RELEASE}/bin")
set(unilink_unilink_unilink_LIBRARY_TYPE_RELEASE SHARED)
set(unilink_unilink_unilink_IS_HOST_WINDOWS_RELEASE 0)
set(unilink_unilink_unilink_RES_DIRS_RELEASE )
set(unilink_unilink_unilink_DEFINITIONS_RELEASE "-DUNILINK_ENABLE_CONFIG=1"
			"-DUNILINK_ENABLE_MEMORY_TRACKING=1")
set(unilink_unilink_unilink_OBJECTS_RELEASE )
set(unilink_unilink_unilink_COMPILE_DEFINITIONS_RELEASE "UNILINK_ENABLE_CONFIG=1"
			"UNILINK_ENABLE_MEMORY_TRACKING=1")
set(unilink_unilink_unilink_COMPILE_OPTIONS_C_RELEASE "")
set(unilink_unilink_unilink_COMPILE_OPTIONS_CXX_RELEASE "-std=c++17")
set(unilink_unilink_unilink_LIBS_RELEASE unilink)
set(unilink_unilink_unilink_SYSTEM_LIBS_RELEASE pthread)
set(unilink_unilink_unilink_FRAMEWORK_DIRS_RELEASE )
set(unilink_unilink_unilink_FRAMEWORKS_RELEASE )
set(unilink_unilink_unilink_DEPENDENCIES_RELEASE )
set(unilink_unilink_unilink_SHARED_LINK_FLAGS_RELEASE )
set(unilink_unilink_unilink_EXE_LINK_FLAGS_RELEASE )
set(unilink_unilink_unilink_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(unilink_unilink_unilink_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${unilink_unilink_unilink_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${unilink_unilink_unilink_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${unilink_unilink_unilink_EXE_LINK_FLAGS_RELEASE}>
)
set(unilink_unilink_unilink_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${unilink_unilink_unilink_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${unilink_unilink_unilink_COMPILE_OPTIONS_C_RELEASE}>")