cmake_minimum_required (VERSION 3.2.0)
 
project(GBA-image-tools)

find_package(ImageMagick REQUIRED COMPONENTS Magick++)

#-------------------------------------------------------------------------------
#set up compiler flags and executable names

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

if(CMAKE_COMPILER_IS_GNUCXX)
    #set up compiler flags for GCC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17") #support C++17
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11") #support C11
endif()

#-------------------------------------------------------------------------------
#set up build directories

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

#-------------------------------------------------------------------------------
#define targets

set(HELPERS_SRC
    colorhelpers.cpp
    datahelpers.cpp
    filehelpers.cpp
    imagehelpers.cpp
    spritehelpers.cpp
)

include_directories(${ImageMagick_INCLUDE_DIRS})
add_executable(colormap555 colormap555.cpp ${HELPERS_SRC})
target_link_libraries(colormap555 ${ImageMagick_LIBRARIES})
add_executable(gimppalette555 gimppalette555.cpp)
add_executable(hex2gba hex2gba.cpp)
target_link_libraries(hex2gba ${ImageMagick_LIBRARIES})
add_executable(img2h img2h.cpp ${HELPERS_SRC})
target_link_libraries(img2h ${ImageMagick_LIBRARIES} stdc++fs pthread)

#-------------------------------------------------------------------------------
#define install files for CPack

install(FILES
    "colormap555"
    "gimppalette555"
    "hex2gba"
    "img2h"
    "colormap555.png"
    "GBA.gpl"
    "README.md"
    "LICENSE"
    DESTINATION .
)

# Tell CPack to create a zip file with our project name
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}")
# Tell CPack not to put everything inside an enclosing directory.
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
# Apparently this should be always on but isn't for backwards compatibility.
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)
