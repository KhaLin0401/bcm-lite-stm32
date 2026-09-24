# 1. Nâng phiên bản tối thiểu lên 3.20
cmake_minimum_required(VERSION 3.20)
project(STM32_BareMetal_App LANGUAGES C)

# Chuẩn C11, tắt extension
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ==============================================================================
# TẦNG PLATFORM (Phần cứng)
# ==============================================================================
# Tạo một pseudo-target (target ảo) đại diện cho nền tảng phần cứng
add_library(platform_stm32f103 INTERFACE)

# Gắn cờ CPU, tối ưu section và zero-warning vào platform
target_compile_options(platform_stm32f103 INTERFACE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections  # Chia function vào các section riêng để linker dọn rác
    -fdata-sections      # Chia biến toàn cục vào các section riêng
    -Wall
    -Wextra
    -Werror              # Bắt buộc sửa warning, không cho phép lơ đi
    -Wpedantic           # Bắt lỗi vi phạm chuẩn C11 thuần túy
)

# ==============================================================================
# CẤU HÌNH LINKER & RUNTIME CHO TẦNG PLATFORM
# ==============================================================================
target_link_options(platform_stm32f103 INTERFACE
    # --- 1. Nhóm cờ Linker ---
    "-Wl,--gc-sections"
    "-Wl,--print-memory-usage"
    
    # Sinh file map động theo tên của Target (Generator Expression)
    "-Wl,-Map=$<TARGET_PROPERTY:NAME>.map"

    # --- 2. Nhóm cờ Runtime (C Standard Library) ---
    "--specs=nano.specs"
    "--specs=nosys.specs"
)

# Macro dòng chip
target_compile_definitions(platform_stm32f103 INTERFACE 
    STM32F103xB
)

# Include path phần cứng
target_include_directories(platform_stm32f103 INTERFACE
    external/cmsis/core
    external/cmsis/device
)

# ==============================================================================
# TẦNG APPLICATION (Phần mềm)
# ==============================================================================
# Đặt tên target thuần túy là "bcm" (hoặc tên ứng dụng của bạn)
add_executable(bcm app/main.c)

# Yêu cầu CMake gắn thêm đuôi .elf khi sinh file vật lý (đầu ra sẽ là bcm.elf)
set_target_properties(bcm PROPERTIES SUFFIX ".elf")

# Các lệnh sau này thao tác với target cực kỳ gọn gàng
target_link_libraries(bcm PRIVATE platform_stm32f103)