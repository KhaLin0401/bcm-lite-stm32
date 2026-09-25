#include <stdint.h>

/**
 * @brief Cấu hình hệ thống trước khi gọi hàm main().
 *        Thường dùng để cấu hình Clock (Flash latency, PLL, hệ số chia AHB/APB).
 */
void SystemInit(void) {
    /* (Cấu hình thanh ghi RCC mặc định hoặc từ hệ thống sinh mã sẽ nằm ở đây) */
    
    /* Tạm thời để trống, vi điều khiển sẽ chạy ở xung nhịp mặc định (HSI 8MHz) */
}