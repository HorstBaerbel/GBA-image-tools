cmake_minimum_required(VERSION 3.16)

#-------------------------------------------------------------------------------
# Add required libraries
 
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMAGICK REQUIRED IMPORTED_TARGET
    Magick++
)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET
    sdl2
)

find_package(OpenMP REQUIRED)

#-------------------------------------------------------------------------------
# Set up compiler flags

# Add defines so Magick++ does not throw warnings
add_definitions(-DMAGICKCORE_HDRI_ENABLE=0)
add_definitions(-DMAGICKCORE_QUANTUM_DEPTH=16)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") #multi-processor compilation
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") #multi-processor compilation
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

#-------------------------------------------------------------------------------
# Define targets

set(HELPERS_SRC
    color_rgb.cpp
    color_ycgco.cpp
    colorhelpers.cpp
    codec_dxt.cpp
    codec_dxtv.cpp
    codec_gvid.cpp
    codec_lzss.cpp
    codec_rle.cpp
    compresshelpers.cpp
    datahelpers.cpp
    filehelpers.cpp
    imagehelpers.cpp
    imageio.cpp
    imagestructs.cpp
    imageprocessing.cpp
    processingoptions.cpp
    spritehelpers.cpp
    statistics.cpp
    statistics_window.cpp
    ui/gui_sdl.cpp
)

include_directories(${ImageMagick_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/eigen ../)
add_executable(colormap555 colormap555.cpp colorhelpers.cpp)
target_link_libraries(colormap555 PkgConfig::LIBMAGICK OpenMP::OpenMP_CXX)
add_executable(gimppalette555 gimppalette555.cpp)
add_executable(hex2gba hex2gba.cpp)
target_link_libraries(hex2gba PkgConfig::LIBMAGICK)
add_executable(img2h img2h.cpp ${HELPERS_SRC})
target_link_libraries(img2h PkgConfig::LIBMAGICK OpenMP::OpenMP_CXX PkgConfig::SDL2 stdc++fs pthread)
add_executable(vid2h vid2h.cpp ${HELPERS_SRC} videoreader.cpp)
target_link_libraries(vid2h PkgConfig::LIBMAGICK PkgConfig::LIBAV OpenMP::OpenMP_CXX PkgConfig::SDL2 stdc++fs pthread)

#-------------------------------------------------------------------------------
# Define install files for CPack

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/colormap555"
    "${CMAKE_CURRENT_BINARY_DIR}/gimppalette555"
    "${CMAKE_CURRENT_BINARY_DIR}/hex2gba"
    "${CMAKE_CURRENT_BINARY_DIR}/img2h"
    "${CMAKE_CURRENT_BINARY_DIR}/vid2h"
    "${PROJECT_SOURCE_DIR}/colormap555.png"
    "${PROJECT_SOURCE_DIR}/GBA.gpl"
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