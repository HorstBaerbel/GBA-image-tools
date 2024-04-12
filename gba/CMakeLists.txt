cmake_minimum_required(VERSION 3.21)

# Use GBA toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/3ds-cmake/DevkitArmGBA.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/3ds-cmake/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(video)

# ASM must be enabled to support .S files
enable_language(ASM)
# Include all the macros and tools needed for GBA development
include(ToolsGBA)
# Using some special compiler flags for GBA
include(CompilerFlagsGBA)

# List all the source files in our directory
LIST(APPEND TARGET_SOURCE_FILES
	tui.cpp
	video.cpp
	memory/dma.cpp
	memory/memcpy.s
	memory/memset.s
	print/itoa.cpp
	print/output.cpp
	sys/decompress.cpp
	video/lz77.s
	video/codec_dxtg.cpp
	video/codec_dxtv.cpp
	video/videodecoder.cpp
	video/videoreader.cpp
	video/videoplayer.cpp
	data/font_8x8.c
	data/video.s
)

LIST(APPEND TARGET_INCLUDE_DIRS
	${DEVKITPRO}/libgba/include
	${DEVKITARM}/arm-none-eabi/include
	${CMAKE_CURRENT_SOURCE_DIR}/../src
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/memory
	${CMAKE_CURRENT_SOURCE_DIR}/video
	${CMAKE_CURRENT_SOURCE_DIR}/print
	${CMAKE_CURRENT_SOURCE_DIR}/sys
)

LIST(APPEND TARGET_LIB_DIRS
	${DEVKITPRO}/libgba/lib
)

LIST(APPEND TARGET_LIBS
	gba
)

link_directories(${TARGET_LIB_DIRS})
include_directories(${TARGET_INCLUDE_DIRS})
add_executable(${PROJECT_NAME}.elf ${TARGET_SOURCE_FILES}) # Create the elf file
add_gba_executable(${PROJECT_NAME}.elf) # Generate the .gba from the elf
target_link_libraries(${PROJECT_NAME}.elf ${TARGET_LIBS}) # Link the application and the demo framework
