cmake_minimum_required (VERSION 3.21)
project(GBA-image-tools)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

#-------------------------------------------------------------------------------
# Create combined source files for the libplum image library

set(LIBPLUM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libplum/build)
add_custom_target(BuildLibplumBasefiles
    make basefiles
    BYPRODUCTS
        ${LIBPLUM_INCLUDE_DIR}/libplum.h
        ${LIBPLUM_INCLUDE_DIR}/libplum.c
    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}/libplum
)

add_subdirectory(src)
add_subdirectory(test)