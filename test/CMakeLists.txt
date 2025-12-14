cmake_minimum_required(VERSION 3.21)

#-------------------------------------------------------------------------------
# Add required libraries
 
find_package(PkgConfig REQUIRED)

find_package(OpenMP REQUIRED)

#-------------------------------------------------------------------------------
# Set up compiler flags

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") #multi-processor compilation
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") #multi-processor compilation
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-enum-enum-conversion") # disable warnings for Eigen enums

#-------------------------------------------------------------------------------
# Create a library target for the Catch2 test framework

set(CATCH_CONFIG_PREFIX_ALL ON CACHE BOOL "Prefix all Catch2 macros with CATCH_")
add_subdirectory(Catch2)

#-------------------------------------------------------------------------------
# Define targets

set(TARGET_SRC
    ${PROJECT_SOURCE_DIR}/src/image_codec/dxt.cpp
    ${PROJECT_SOURCE_DIR}/src/if/dxt_tables.cpp
    ${PROJECT_SOURCE_DIR}/src/if/dxtv_structs.cpp
    ${PROJECT_SOURCE_DIR}/src/video_codec/dxtv.cpp
    ${PROJECT_SOURCE_DIR}/src/color/conversions.cpp
    ${PROJECT_SOURCE_DIR}/src/color/colorformat.cpp
    ${PROJECT_SOURCE_DIR}/src/color/colorhelpers.cpp
    ${PROJECT_SOURCE_DIR}/src/color/grayf.cpp
    ${PROJECT_SOURCE_DIR}/src/color/lchf.cpp
    ${PROJECT_SOURCE_DIR}/src/color/rgb565.cpp
    ${PROJECT_SOURCE_DIR}/src/color/rgb888.cpp
    ${PROJECT_SOURCE_DIR}/src/color/rgbf.cpp
    ${PROJECT_SOURCE_DIR}/src/color/xrgb1555.cpp
    ${PROJECT_SOURCE_DIR}/src/color/xrgb8888.cpp
    ${PROJECT_SOURCE_DIR}/src/color/ycgcorf.cpp
    ${PROJECT_SOURCE_DIR}/src/compression/lz4.cpp
    ${PROJECT_SOURCE_DIR}/src/compression/lzss.cpp
    ${PROJECT_SOURCE_DIR}/src/compression/rans.cpp
    ${PROJECT_SOURCE_DIR}/src/image/imageio.cpp
    ${PROJECT_SOURCE_DIR}/src/image/datatype.cpp
    ${PROJECT_SOURCE_DIR}/src/image/imagehelpers.cpp
    ${PROJECT_SOURCE_DIR}/src/image/spritehelpers.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/datahelpers.cpp
    ${PROJECT_SOURCE_DIR}/src/statistics/statistics.cpp
    ${LIBPLUM_INCLUDE_DIR}/libplum.c
)

set(TARGET_INCLUDES
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/codec
)

file(GLOB TESTS_SRC CONFIGURE_DEPENDS "*.cpp")

set(TARGET_NAME unit_tests)

add_executable(${TARGET_NAME}
    ${TARGET_SRC}
    ${TESTS_SRC}
)
target_link_libraries(${TARGET_NAME}
    PRIVATE
        OpenMP::OpenMP_CXX
        Catch2::Catch2WithMain
        pthread
)
target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TARGET_INCLUDES}
        ${PROJECT_SOURCE_DIR}/eigen
        ${CMAKE_CURRENT_SOURCE_DIR}/Catch2
        ${LIBPLUM_INCLUDE_DIR}
)
add_dependencies(${TARGET_NAME} BuildLibplumBasefiles)

list(APPEND CMAKE_MODULE_PATH
    ${catch2_SOURCE_DIR}/extras
)
include(CTest)
include(Catch)
catch_discover_tests(${TARGET_NAME})
