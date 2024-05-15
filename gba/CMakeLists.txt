cmake_minimum_required(VERSION 3.21)

# Use GBA toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/3ds-cmake/DevkitArmGBA.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/3ds-cmake/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

project(gba_examples)

# ASM must be enabled to support .S files
enable_language(ASM)
# Include all the macros and tools needed for GBA development
include(ToolsGBA)
# Using some special compiler flags for GBA
include(CompilerFlagsGBA)

set(IMG2H_PATH "${CMAKE_CURRENT_LIST_DIR}/../build/src/img2h")

macro(target_img2h_file _listName _outname _infiles _options)
	# split options and arguments up
	separate_arguments(_option_list NATIVE_COMMAND ${_options})
	separate_arguments(_infile_list NATIVE_COMMAND ${_infiles})
	# check if the input file is a wildcard
	file(GLOB _globbed_infiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${_infile_list})
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/data/${_outname}.h" "${CMAKE_CURRENT_SOURCE_DIR}/data/${_outname}.c"
		COMMAND "${IMG2H_PATH}" ${_option_list} ${_infile_list} data/${_outname}
		DEPENDS ${_globbed_infiles}
		WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
		COMMAND_EXPAND_LISTS
	)
	LIST(APPEND ${_listName} "${CMAKE_CURRENT_SOURCE_DIR}/data/${_outname}.c")
endmacro()

# Video example sources --------------------------------------------
LIST(APPEND VIDEO_SOURCE_FILES
	tui.cpp
	video.cpp
	memory/dma.cpp
	memory/memcpy.s
	memory/memset.s
	print/itoa.cpp
	print/output.cpp
	sys/decompress.cpp
	compression/lz77.s
	video/codec_dxtv.cpp
	video/videodecoder.cpp
	video/videoreader.cpp
	video/videoplayer.cpp
	data/font_8x8.c
	data/video.s
)
set_source_files_properties(data/video.s OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/data/video.bin)

# Image example sources --------------------------------------------
LIST(APPEND IMAGE_SOURCE_FILES
	tui.cpp
	image.cpp
	memory/dma.cpp
	memory/memcpy.s
	memory/memset.s
	print/itoa.cpp
	print/output.cpp
	sys/decompress.cpp
	compression/lz77.s
	image/dxt.cpp
	data/font_8x8.c
)

# Add image source files to be converted with img2h here. Paths with spaces need escaping (\"\")
# These files will be added to IMAGE_DATA_FILES
target_img2h_file(IMAGE_DATA_FILES "images_dxt" "\"../data/images/240x160/*.png\"" "--truecolor=rgb888 --outformat=rgb555 --dxt --lz10 --vram=true")

LIST(APPEND TARGET_INCLUDE_DIRS
	${DEVKITPRO}/libgba/include
	${DEVKITARM}/arm-none-eabi/include
	${CMAKE_CURRENT_SOURCE_DIR}/../src
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/data
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

# Video example
add_executable(video.elf ${VIDEO_SOURCE_FILES}) # Create the elf file
add_gba_executable(video.elf) # Generate the .gba from the elf
target_link_libraries(video.elf ${TARGET_LIBS}) # Link the application and the demo framework

# Image example
add_executable(image.elf ${IMAGE_SOURCE_FILES} ${IMAGE_DATA_FILES}) # Create the elf file
add_gba_executable(image.elf) # Generate the .gba from the elf
target_link_libraries(image.elf ${TARGET_LIBS}) # Link the application and the demo framework
