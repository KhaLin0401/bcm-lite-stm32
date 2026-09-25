cmake_minimum_required(VERSION 3.20)
project(bcm_lite LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f103/linker/stm32f103c8.ld)

# ============================================================
# TẦNG PLATFORM
# ============================================================
add_library(platform_stm32f103 INTERFACE)

target_compile_options(platform_stm32f103 INTERFACE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)

target_link_options(platform_stm32f103 INTERFACE
    -mcpu=cortex-m3
    -mthumb
    -nostartfiles
    --specs=nano.specs
    --specs=nosys.specs
    "-T" "${LINKER_SCRIPT}"
    -Wl,--gc-sections
    -Wl,--orphan-handling=warn
    -Wl,--print-memory-usage
    "-Wl,-Map=$<TARGET_PROPERTY:NAME>.map"
)

target_sources(platform_stm32f103 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f103/startup/startup_stm32f103.c
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f103/system/system_stm32f103.c
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32f103/startup/startup_stm32f103.c
    PROPERTIES COMPILE_OPTIONS "-fno-tree-loop-distribute-patterns"
)

target_compile_definitions(platform_stm32f103 INTERFACE
    STM32F103xB
)

target_include_directories(platform_stm32f103 SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/cmsis/core
    ${CMAKE_CURRENT_SOURCE_DIR}/external/cmsis/device
)

# ============================================================
# TẦNG APPLICATION
# ============================================================
add_executable(bcm app/main.c)

set_target_properties(bcm PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS ${LINKER_SCRIPT}
)

target_link_libraries(bcm PRIVATE platform_stm32f103)