#Báo cho CMake biết đây là cross-compile cho hệ thống không có OS
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Xử lý "cái bẫy" CMake tự thử biên dịch
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Chỉ định đường dẫn tới bộ công cụ arm-none-eabi
# (Giả định rằng thư mục bin của arm-none-eabi đã được thêm vào PATH của máy tính)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
# Ngăn CMake lấy nhằm thư viện của HOST thực hiện build chương trình
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)