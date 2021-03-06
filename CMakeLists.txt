cmake_minimum_required (VERSION 3.1.0)
 
project(ImageTools)

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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") #support C++14
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
