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
	GBA-demo-framework/src/time.cpp
	GBA-demo-framework/src/tui.cpp
	GBA-demo-framework/src/memory/dma.cpp
	GBA-demo-framework/src/memory/memcpy.s
	GBA-demo-framework/src/memory/memset.s
	GBA-demo-framework/src/print/itoa.cpp
	GBA-demo-framework/src/print/output.cpp
	GBA-demo-framework/src/sys/interrupts.cpp
	GBA-demo-framework/src/sys/intdispatcher.s
	GBA-demo-framework/src/sys/input.cpp
	GBA-demo-framework/src/compression/bios.cpp
	GBA-demo-framework/src/compression/lz4.cpp
	GBA-demo-framework/src/compression/lz4_asm.s
	GBA-demo-framework/src/compression/lz77_asm.s
	GBA-demo-framework/src/compression/adpcm.cpp
	GBA-demo-framework/src/compression/adpcm_asm.s
	../src/if/adpcm_structs.cpp
	../src/if/adpcm_tables.cpp
	../src/if/dxt_tables.cpp
	../src/if/dxtv_structs.cpp
	video/dxtv.cpp
	video/dxtv_asm.s
	video/vid2hdecoder.cpp
	video/vid2hio.cpp
	video/videoplayer.cpp
	video.cpp
	data/font_8x8.c
	data/video.s
)
set_source_files_properties(data/video.s OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/data/video.bin)

# Image example sources --------------------------------------------
LIST(APPEND IMAGE_SOURCE_FILES
	GBA-demo-framework/src/tui.cpp
	GBA-demo-framework/src/memory/dma.cpp
	GBA-demo-framework/src/memory/memcpy.s
	GBA-demo-framework/src/memory/memset.s
	GBA-demo-framework/src/print/itoa.cpp
	GBA-demo-framework/src/print/output.cpp
	GBA-demo-framework/src/sys/interrupts.cpp
	GBA-demo-framework/src/sys/intdispatcher.s
	GBA-demo-framework/src/sys/input.cpp
	GBA-demo-framework/src/compression/bios.cpp
	GBA-demo-framework/src/compression/lz4.cpp
	GBA-demo-framework/src/compression/lz4_asm.s
	GBA-demo-framework/src/compression/lz77_asm.s
	../src/if/dxt_tables.cpp
	image/dxt.cpp
	image.cpp
	data/font_8x8.c
)

# Add image source files to be converted with img2h here. Paths with spaces need escaping (\"\")
# These files will be added to IMAGE_DATA_FILES
target_img2h_file(IMAGE_DATA_FILES "images_dxt" "\"../data/images/240x160/*.png\"" "--truecolor=rgb888 --outformat=bgr555 --dxt --lz4 --vram=true")

LIST(APPEND INCLUDE_DIRS
	${DEVKITARM}/arm-none-eabi/include
	${CMAKE_CURRENT_SOURCE_DIR}/../src/if # Include directory for common code from the GBA image tools
	${CMAKE_CURRENT_SOURCE_DIR}/GBA-demo-framework/src
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/data
	${CMAKE_CURRENT_SOURCE_DIR}/video
)

LIST(APPEND LIB_DIRS
)

LIST(APPEND LIBS
)

# Video example
add_executable(video ${VIDEO_SOURCE_FILES}) # Create the elf file
add_gba_executables(video "GBAIT_VIDEO" "GITV" "HB" 1) # Generate the .gba from the elf
add_map_file(video)
add_sections_file(video)
target_include_directories(video PRIVATE ${INCLUDE_DIRS})
target_link_directories(video PRIVATE  ${LIB_DIRS})
target_link_libraries(video ${LIBS}) # Link the application and the demo framework

# Image example
add_executable(image ${IMAGE_SOURCE_FILES} ${IMAGE_DATA_FILES}) # Create the elf file
add_gba_executables(image "GBAIT_IMAGE" "GITI" "HB" 1) # Generate the .gba from the elf
add_map_file(image)
add_sections_file(image)
target_include_directories(image PRIVATE ${INCLUDE_DIRS})
target_link_directories(image PRIVATE ${LIB_DIRS})
target_link_libraries(image ${LIBS}) # Link the application and the demo framework
