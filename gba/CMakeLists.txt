cmake_minimum_required(VERSION 3.21)

# Use GBA toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/GBA-demo-framework/3ds-cmake/DevkitArmGBA.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/GBA-demo-framework/3ds-cmake/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/GBA-demo-framework/cmake)

project(gba_examples)

# ASM must be enabled to support .S files
enable_language(ASM)
# Include all the macros and tools needed for GBA development
include(ToolsGBA)
# Using some special compiler flags for GBA
include(CompilerFlagsGBA)

# Image data conversion --------------------------------------------

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
		VERBATIM
	)
	LIST(APPEND ${_listName} "${CMAKE_CURRENT_SOURCE_DIR}/data/${_outname}.c")
endmacro()

# Decoder sources --------------------------------------------

# Set directory for GBA demo framework
set(DEMO_FRAMEWORK_SRC_DIR GBA-demo-framework/src)

# Set flags on which version of decoders to use
set(DXTV_USE_ASM True)
if(DXTV_USE_ASM)
	list(APPEND DXTV_DECODER_FILES ${DEMO_FRAMEWORK_SRC_DIR}/video/dxtv_asm.s)
else()
	list(APPEND DXTV_DECODER_FILES ${DEMO_FRAMEWORK_SRC_DIR}/video/dxtv.cpp)
endif()
list(APPEND DXTV_DECODER_FILES
	${DEMO_FRAMEWORK_SRC_DIR}/image/dxt_tables.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/video/dxtv_structs.cpp
)

set(LZ4_USE_ASM True)
if(LZ4_USE_ASM)
	list(APPEND LZ4_DECODER_FILES ${DEMO_FRAMEWORK_SRC_DIR}/compression/lz4_asm.s)
else()
	list(APPEND LZ4_DECODER_FILES ${DEMO_FRAMEWORK_SRC_DIR}/compression/lz4.cpp)
endif()

set(ADPCM_USE_ASM True)
if(ADPCM_USE_ASM)
	list(APPEND ADPCM_DECODER_FILES ${DEMO_FRAMEWORK_SRC_DIR}/audio/adpcm_asm.s)
else()
	list(APPEND ADPCM_DECODER_FILES ${DEMO_FRAMEWORK_SRC_DIR}/audio/adpcm.cpp)
endif()
list(APPEND ADPCM_DECODER_FILES
	${DEMO_FRAMEWORK_SRC_DIR}/audio/adpcm_structs.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/audio/adpcm_tables.cpp
)

# Video example sources --------------------------------------------
LIST(APPEND VIDEO_SOURCE_FILES
	${DEMO_FRAMEWORK_SRC_DIR}/sprites.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/time.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/tui.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/tiles.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/audio/resampler.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/memory/dma.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/memory/memcpy.s
	${DEMO_FRAMEWORK_SRC_DIR}/memory/memory.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/memory/memset.s
	${DEMO_FRAMEWORK_SRC_DIR}/print/itoa.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/print/output.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/sys/halt.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/sys/interrupts.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/sys/intdispatcher.s
	${DEMO_FRAMEWORK_SRC_DIR}/sys/input.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/sys/memctrl.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/compression/bios.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/compression/lz77_asm.s
	${DEMO_FRAMEWORK_SRC_DIR}/media/vid2hio.cpp
	${DXTV_DECODER_FILES}
	${LZ4_DECODER_FILES}
	${ADPCM_DECODER_FILES}
	video/mediahelp.cpp
	video/mediaplayer.cpp
	video/subtitles.cpp
	video/vid2hdecoder.cpp
	video.cpp
	data/video.s
)
set_source_files_properties(data/video.s OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/data/video.bin)

# Add image source files to be converted with img2h here. Paths with spaces need escaping (\"\")
target_img2h_file(VIDEO_DATA_FILES "font_8x8" "data/font_8x8.png" "--blackwhite=0.5 --outformat=bgr555 --tiles --prune=4")
target_img2h_file(VIDEO_DATA_FILES "font_subtitles" "data/font_subtitles.png" "--blackwhite=0.5 --outformat=bgr555 --tiles --prune=4")
target_img2h_file(VIDEO_DATA_FILES "mediahelp" "data/mediahelp.png" "--paletted=16 --outformat=bgr555 --tiles --sprites=64,32 --prune=4")

# Image example sources --------------------------------------------
LIST(APPEND IMAGE_SOURCE_FILES
	${DEMO_FRAMEWORK_SRC_DIR}/tui.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/memory/dma.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/memory/memcpy.s
	${DEMO_FRAMEWORK_SRC_DIR}/memory/memset.s
	${DEMO_FRAMEWORK_SRC_DIR}/print/itoa.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/print/output.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/sys/halt.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/sys/interrupts.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/sys/intdispatcher.s
	${DEMO_FRAMEWORK_SRC_DIR}/sys/input.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/sys/memctrl.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/compression/bios.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/compression/lz77_asm.s
	${DEMO_FRAMEWORK_SRC_DIR}/image/dxt.cpp
	${DEMO_FRAMEWORK_SRC_DIR}/image/dxt_tables.cpp
	${LZ4_DECODER_FILES}
	image.cpp
)

# Add image source files to be converted with img2h here. Paths with spaces need escaping (\"\")
target_img2h_file(IMAGE_DATA_FILES "font_8x8" "data/font_8x8.png" "--blackwhite=0.5 --outformat=bgr555 --tiles --prune=4")
target_img2h_file(IMAGE_DATA_FILES "images_dxt" "../data/images/240x160/*.png" "--truecolor=rgb888 --outformat=bgr555 --dxt --lz10 --vram=true")

LIST(APPEND INCLUDE_DIRS
	${DEVKITARM}/arm-none-eabi/include
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
add_executable(video ${VIDEO_SOURCE_FILES} ${VIDEO_DATA_FILES}) # Create the elf file
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
