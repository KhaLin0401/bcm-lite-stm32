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
    -g3
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

# ==============================================================================
# POST-BUILD ACTIONS (Sinh file nạp)
# ==============================================================================
add_custom_command(TARGET bcm POST_BUILD
    # 1. Sinh file .hex (dùng cho các tool nạp ST-Link Utility, STM32CubeProgrammer)
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:bcm> $<TARGET_FILE_DIR:bcm>/$<TARGET_FILE_BASE_NAME:bcm>.hex
    
    # 2. Sinh file .bin (dùng để nạp qua bootloader, OTA hoặc J-Link)
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:bcm> $<TARGET_FILE_DIR:bcm>/$<TARGET_FILE_BASE_NAME:bcm>.bin
    
    # 3. In ra dung lượng file (Kinh điển của GCC)
    COMMAND arm-none-eabi-size ${CMAKE_SIZE} $<TARGET_FILE:bcm>
    
    COMMENT "Generating file .hex and .bin..."
)

# ==============================================================================
# FLASH TARGET (Nạp firmware qua pyOCD)
# ==============================================================================
add_custom_target(flash
    # Lệnh gọi pyOCD để nạp file .elf
    COMMAND pyocd flash $<TARGET_FILE:bcm> -t stm32f103c8
    
    # Rất quan trọng: Bắt buộc phải build target 'bcm' thành công trước khi nạp
    DEPENDS bcm
    
    # Hiển thị thông báo khi bắt đầu chạy
    COMMENT "Flashing firmware to STM32 by pyOCD..."
    
    # Cho phép pyOCD chiếm quyền điều khiển terminal để in thanh tiến trình (progress bar)
    USES_TERMINAL 
)