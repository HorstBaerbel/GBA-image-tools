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
pkg_check_modules(TBB REQUIRED IMPORTED_TARGET
    tbb
)

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
# Create a library target for the Catch header-only test framework

add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE test/)

#-------------------------------------------------------------------------------
#define targets

set(TESTS_SRC
)

set(TARGET_NAME unit_tests)
 
add_executable(${TARGET_NAME} main.cpp ${TESTS_SRC})
target_link_libraries(${TARGET_NAME} PRIVATE Catch PkgConfig::LIBMAGICK PkgConfig::LIBAV PkgConfig::TBB stdc++fs pthread)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/Catch2/single_include)

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} -o report.xml -r junit)
