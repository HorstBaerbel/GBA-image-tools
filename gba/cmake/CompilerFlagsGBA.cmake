# Using some special compiler flags for GBA:
# Link-time optimization: -flto
# Assembly output: -save-temps / or -save-temps=obj (subdirectories) (delete build directory first!)
# Minimum library includes: -ffreestanding -nostartfiles -nostdlib -nodefaultlibs
# Analyze memory usage: -Wl,--print-memory-usage
# Analyze stack usage: -fstack-usage
# See also: https://github.com/PeterMcKinnis/WorstCaseStack for stack analysis
# Change stack usage: -Wl,--stack,NR_OF_BYTES_OF_STACK
# More debug info: -g3
# More debug info when using GDB: -ggdb3
# Replace compiler memory allocation with our own: -Wl,--wrap=malloc,--wrap=free,--wrap=alloc,--wrap=calloc
# Output .map file: -Wl,-Map=${CMAKE_PROJECT_NAME}.map
# Possibly useful: -fno-jump-tables -fno-toplevel-reorder
# Remove unused functions:
#   Add to compiler flags: -fdata-sections -ffunction-sections
#   Add to linker flags (works without the above, but less effective): --gc-sections,--strip-all
#   In theory adding -flto to both compiler and linker flags should work, but GCC reports section conflicts due to an old bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41091
# Removed (does not seem to be needed): -mlong-calls
# See also: https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html
set(ARCH "-march=armv4t -mthumb -mthumb-interwork -Wl,--wrap=malloc,--wrap=free,--wrap=alloc,--wrap=calloc,--print-memory-usage,--gc-sections")
set(COMPILERFLAGS "-save-temps -Wall -Wextra -Wpedantic -mcpu=arm7tdmi -mtune=arm7tdmi -fomit-frame-pointer -ffast-math -no-pie -fno-stack-protector -fdata-sections -ffunction-sections")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wa,--warn -x assembler-with-cpp ${ARCH}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH} ${COMPILERFLAGS} -std=c11")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Og -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3") # -Wl,--strip-all")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH} ${COMPILERFLAGS} -std=c++17 -fconserve-space -fno-threadsafe-statics -fno-rtti -fno-exceptions")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Og -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3") # -Wl,--strip-all")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ARCH}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} --specs=gba.specs")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} --specs=gba.specs") # -s")

# message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
# message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
# message("CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")
# message("CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
# message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
# message("CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
# message("CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
# message("CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
