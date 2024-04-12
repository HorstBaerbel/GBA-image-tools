cmake_minimum_required(VERSION 3.21)

#-------------------------------------------------------------------------------
# Add required libraries
 
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMAGICK REQUIRED IMPORTED_TARGET
    Magick++
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
set(CMAKE_C_STANDARD 11)

#-------------------------------------------------------------------------------
# Create a library target for the Catch2 test framework

set(CATCH_CONFIG_PREFIX_ALL ON CACHE BOOL "Prefix all Catch2 macros with CATCH_")
add_subdirectory(Catch2)

#-------------------------------------------------------------------------------
# Define targets

set(TARGET_SRC
    ${PROJECT_SOURCE_DIR}/src/color/conversions.cpp
    ${PROJECT_SOURCE_DIR}/src/color/colorformat.cpp
    ${PROJECT_SOURCE_DIR}/src/color/grayf.cpp
    ${PROJECT_SOURCE_DIR}/src/color/lchf.cpp
    ${PROJECT_SOURCE_DIR}/src/color/rgb565.cpp
    ${PROJECT_SOURCE_DIR}/src/color/rgbf.cpp
    ${PROJECT_SOURCE_DIR}/src/color/xrgb1555.cpp
    ${PROJECT_SOURCE_DIR}/src/color/xrgb8888.cpp
    ${PROJECT_SOURCE_DIR}/src/color/ycgcorf.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/datahelpers.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/imagehelpers.cpp
    ${PROJECT_SOURCE_DIR}/src/processing/spritehelpers.cpp
)

set(TARGET_INCLUDES
    ${PROJECT_SOURCE_DIR}/src
)

file(GLOB TESTS_SRC CONFIGURE_DEPENDS "*.cpp")

# Add defines so Magick++ does not throw warnings
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/io/imageio.cpp PROPERTIES COMPILE_DEFINTIONS "MAGICKCORE_HDRI_ENABLE=0 MAGICKCORE_QUANTUM_DEPTH=16")

set(TARGET_NAME unit_tests)

add_executable(${TARGET_NAME} ${TARGET_SRC} ${TESTS_SRC})
target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain pthread)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${TARGET_INCLUDES} ${PROJECT_SOURCE_DIR}/eigen ${CMAKE_CURRENT_SOURCE_DIR}/Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${TARGET_NAME})
