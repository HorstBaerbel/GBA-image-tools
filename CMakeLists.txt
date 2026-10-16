cmake_minimum_required (VERSION 3.21)
project(GBA-image-tools)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

#-------------------------------------------------------------------------------
# Create combined source files for the libplum image library

set(LIBPLUM_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/libplum/build)
add_custom_target(BuildLibplumBasefiles
    make basefiles
    BYPRODUCTS
        ${LIBPLUM_INCLUDE_DIR}/libplum.h
        ${LIBPLUM_INCLUDE_DIR}/libplum.c
    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}/libplum
)

#-------------------------------------------------------------------------------
# Synchronize shared definitions into the GBA-demo-framework submodule.
# The submodule is used by other projects too, so it must keep its own copy of
# the shared definitions. The files below are pure data-format definitions that
# must be byte-for-byte identical on both sides (no platform-specific code),
# so the authoritative copies from src/media/, src/audio_codec/ and
# src/image_codec/ are copied into the submodule whenever
# the source file has changed.
#
# sync_shared_file(<src_file> <dst_file>)
#   Creates a custom target "Sync<base>" that copies <src_file> to <dst_file>
#   when the source has changed, and records it in SHARED_SYNC_TARGETS.
#   Anchor dependencies (e.g. on the vid2h tool) are applied after the
#   subdirectories have been added, since those targets do not exist yet.
function(sync_shared_file src_file dst_file)
    get_filename_component(src_base ${src_file} NAME)
    get_filename_component(src_dir ${src_file} DIRECTORY)
    get_filename_component(src_dir ${src_dir} NAME)
    set(sync_target "Sync_${src_dir}_${src_base}")
    add_custom_command(
        OUTPUT ${dst_file}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${src_file} ${dst_file}
        DEPENDS ${src_file}
        COMMENT "Synchronizing ${src_file} into GBA-demo-framework ${dst_file}"
        VERBATIM
    )
    add_custom_target(${sync_target} DEPENDS ${dst_file})
    list(APPEND SHARED_SYNC_TARGETS ${sync_target})
    set(SHARED_SYNC_TARGETS ${SHARED_SYNC_TARGETS} PARENT_SCOPE)
endfunction()

set(SUBMODULE_SRC_DIR ${PROJECT_SOURCE_DIR}/gba/GBA-demo-framework/src)

# Files that must stay in sync between the host tools and the GBA submodule
sync_shared_file(${PROJECT_SOURCE_DIR}/src/media/vid2h_structs.h         ${SUBMODULE_SRC_DIR}/media/vid2h_structs.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/media/mediatypes.h            ${SUBMODULE_SRC_DIR}/media/mediatypes.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/audio/processingtype.h        ${SUBMODULE_SRC_DIR}/audio/processingtype.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/audio_codec/adpcm_tables.h    ${SUBMODULE_SRC_DIR}/audio/adpcm_tables.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/audio_codec/adpcm_tables.cpp  ${SUBMODULE_SRC_DIR}/audio/adpcm_tables.cpp)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/audio_codec/adpcm_structs.h   ${SUBMODULE_SRC_DIR}/audio/adpcm_structs.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/audio_codec/adpcm_structs.cpp ${SUBMODULE_SRC_DIR}/audio/adpcm_structs.cpp)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/image/processingtype.h        ${SUBMODULE_SRC_DIR}/image/processingtype.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/image_codec/dxt_tables.h      ${SUBMODULE_SRC_DIR}/image/dxt_tables.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/image_codec/dxt_tables.cpp    ${SUBMODULE_SRC_DIR}/image/dxt_tables.cpp)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/video_codec/dxtv_constants.h  ${SUBMODULE_SRC_DIR}/video/dxtv_constants.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/video_codec/dxtv_structs.h    ${SUBMODULE_SRC_DIR}/video/dxtv_structs.h)
sync_shared_file(${PROJECT_SOURCE_DIR}/src/video_codec/dxtv_structs.cpp  ${SUBMODULE_SRC_DIR}/video/dxtv_structs.cpp)

add_subdirectory(adpcm-xq)
add_subdirectory(src)
add_subdirectory(test)

# Build the sync step whenever the vid2h tool (which defines the format) is built
foreach(sync_target ${SHARED_SYNC_TARGETS})
    add_dependencies(vid2h ${sync_target})
endforeach()