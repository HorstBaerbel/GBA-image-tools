cmake_minimum_required(VERSION 3.18)

#-------------------------------------------------------------------------------
# Add required libraries
 
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMAGICK REQUIRED IMPORTED_TARGET
    Magick++
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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

#-------------------------------------------------------------------------------
# Create a library target for the Catch2 test framework

set(CATCH_CONFIG_PREFIX_ALL ON)
add_subdirectory(Catch2)

#-------------------------------------------------------------------------------
# Define targets

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/color/xrgb1555.cpp
)

set(TESTS_SRC
    test_xrgb1555.cpp
)

set(TARGET_NAME unit_tests)

add_executable(${TARGET_NAME} ${SRC} ${TESTS_SRC})
target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain pthread)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${TARGET_NAME})
