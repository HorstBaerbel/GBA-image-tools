cmake_minimum_required(VERSION 3.21)

#-------------------------------------------------------------------------------
# Add required libraries
 
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET
    sdl2
)

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2") # for profiling

#-------------------------------------------------------------------------------
# Define sources

list(APPEND GENCOLORMAPS_SRC
    gencolormaps.cpp
    color/colorformat.cpp
    color/colorhelpers.cpp
    color/lchf.cpp
    color/xrgb1555.cpp
    color/rgb565.cpp
    color/rgb888.cpp
    color/xrgb8888.cpp
    color/ycgcorf.cpp
    color/conversions.cpp
    io/imageio.cpp
    ${LIBPLUM_INCLUDE_DIR}/libplum.c
)

list(APPEND HEX2GBA_SRC
    hex2gba.cpp
    color/xrgb8888.cpp
)

file(GLOB_RECURSE HELPERS_SRC CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*/*.cpp ${PROJECT_SOURCE_DIR}/src/*/*.h)
list(APPEND HELPERS_SRC
    ${LIBPLUM_INCLUDE_DIR}/libplum.c
)

list(APPEND IMG2H_SRC
    img2h.cpp
    ${HELPERS_SRC}
)

list(APPEND VID2H_SRC
    vid2h.cpp
    ${HELPERS_SRC}
)

list(APPEND VID2HPLAY_SRC
    vid2hplay.cpp
    ${HELPERS_SRC}
)

#-------------------------------------------------------------------------------
# Define targets

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/cxxopts
    ${PROJECT_SOURCE_DIR}/eigen
    ${PROJECT_SOURCE_DIR}/glob
    ${PROJECT_SOURCE_DIR}/src
    ${LIBPLUM_INCLUDE_DIR}
)

add_executable(gencolormaps ${GENCOLORMAPS_SRC})
target_link_libraries(gencolormaps OpenMP::OpenMP_CXX)
add_dependencies(gencolormaps BuildLibplumBasefiles)

add_executable(hex2gba ${HEX2GBA_SRC})
target_link_libraries(hex2gba)

add_executable(img2h ${IMG2H_SRC})
target_link_libraries(img2h PkgConfig::LIBAV OpenMP::OpenMP_CXX PkgConfig::SDL2 stdc++fs pthread)
add_dependencies(img2h BuildLibplumBasefiles)

add_executable(vid2h ${VID2H_SRC})
target_link_libraries(vid2h PkgConfig::LIBAV OpenMP::OpenMP_CXX PkgConfig::SDL2 stdc++fs pthread)
add_dependencies(vid2h BuildLibplumBasefiles)

add_executable(vid2hplay ${VID2HPLAY_SRC})
target_link_libraries(vid2hplay PkgConfig::LIBAV OpenMP::OpenMP_CXX PkgConfig::SDL2 stdc++fs pthread)
add_dependencies(vid2hplay BuildLibplumBasefiles)

#-------------------------------------------------------------------------------
# Define install files for CPack

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/gencolormaps"
    "${CMAKE_CURRENT_BINARY_DIR}/hex2gba"
    "${CMAKE_CURRENT_BINARY_DIR}/img2h"
    "${CMAKE_CURRENT_BINARY_DIR}/vid2h"
    "${CMAKE_CURRENT_BINARY_DIR}/vid2hplay"
    "${PROJECT_SOURCE_DIR}/colormap555.png"
    "${PROJECT_SOURCE_DIR}/colormap565.png"
    "${PROJECT_SOURCE_DIR}/README.md"
    "${PROJECT_SOURCE_DIR}/img2h.md"
    "${PROJECT_SOURCE_DIR}/vid2h.md"
    "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION .
)

# Tell CPack to create a zip file with our project name
set(CPACK_GENERATOR "ZIP")
# Tell CPack not to put everything inside an enclosing directory.
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
# Apparently this should be always on but isn't for backwards compatibility.
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)
