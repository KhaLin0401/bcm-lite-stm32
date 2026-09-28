#include <stdint.h>
#include <stm32f1xx.h>


uint32_t g_initialized = 0xDEADBEEFu;
uint32_t g_zeroed;


int main(void)
{
    //Bật clock cho PORT C bằng cách set thanh ghi APB2ENR
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // Xóa sạch 4 bit cấu hình hiện tại của Pin 13 (đề phòng giá trị mặc định lúc reset)
    GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
    
    // Ghi giá trị 10 (binary) vào vị trí MODE13
    GPIOC->CRH |= GPIO_CRH_MODE13_1;

    /* 3. Vòng lặp nhấp nháy LED (Busy-wait) */
    while (1) {
        // Đảo trạng thái chân PC13 thông qua thanh ghi Output Data Register (ODR)
        GPIOC->ODR ^= GPIO_ODR_ODR13;

        // Cập nhật giá trị hai biến toàn cục để tránh bị compiler tối ưu hóa đi mất (nếu bật -O2)
        // Khi cắm Debugger, bạn soi vào vùng nhớ RAM sẽ thấy g_initialized chạy từ 0xDEADBEEF, 
        // và g_zeroed chạy từ 0.
        g_initialized++;
        g_zeroed++;

        // Busy-wait delay (tương đối). 
        // Từ khóa 'volatile' ép trình biên dịch không được phép bỏ qua vòng lặp trống này.
        // Xung nhịp HSI mặc định là 8MHz, vòng lặp này mất vài chu kỳ máy mỗi lần lặp.
        for (volatile i = 0; i < 500000; i++) {
            // Không làm gì cả
            // Done
        }
    }

    return 0;
}