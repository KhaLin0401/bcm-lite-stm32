#include <stdint.h>
#include "stm32f1xx.h" 

/* Định nghĩa kiểu con trỏ hàm chuẩn cho Bảng Vector */
typedef void (*isr_func_t)(void);

/* 
 * KỸ THUẬT VƯỢT MẶT -Wpedantic:
 * Khai báo _estack như một hàm thay vì một biến extern uint32_t.
 * Nhờ đó, trình biên dịch C sẽ hiểu nó là một function pointer, 
 * khớp hoàn hảo với kiểu isr_func_t mà không cần bất kỳ cú pháp ép kiểu nào.
 */
extern void _estack(void);

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int main(void);
extern void SystemInit(void);

void Reset_Handler(void);
void Default_Handler(void);

void Default_Handler(void) {
    while (1) {
    }
}

void HardFault_Handler(void) {
    /* TODO (M4 - Dem): Kiểm tra trạng thái debugger trước khi bkpt */
    __BKPT(0);
    while (1) {
    }
}

#define WEAK_ALIAS(target) __attribute__((weak, alias(#target)))

/* System Exceptions */
void NMI_Handler(void)          WEAK_ALIAS(Default_Handler);
void MemManage_Handler(void)    WEAK_ALIAS(Default_Handler);
void BusFault_Handler(void)     WEAK_ALIAS(Default_Handler);
void UsageFault_Handler(void)   WEAK_ALIAS(Default_Handler);
void SVC_Handler(void)          WEAK_ALIAS(Default_Handler);
void DebugMon_Handler(void)     WEAK_ALIAS(Default_Handler);
void PendSV_Handler(void)       WEAK_ALIAS(Default_Handler);
void SysTick_Handler(void)      WEAK_ALIAS(Default_Handler);

/* Peripheral IRQs */
void WWDG_IRQHandler(void)               WEAK_ALIAS(Default_Handler);
void PVD_IRQHandler(void)                WEAK_ALIAS(Default_Handler);
void TAMPER_IRQHandler(void)             WEAK_ALIAS(Default_Handler);
void RTC_IRQHandler(void)                WEAK_ALIAS(Default_Handler);
void FLASH_IRQHandler(void)              WEAK_ALIAS(Default_Handler);
void RCC_IRQHandler(void)                WEAK_ALIAS(Default_Handler);
void EXTI0_IRQHandler(void)              WEAK_ALIAS(Default_Handler);
void EXTI1_IRQHandler(void)              WEAK_ALIAS(Default_Handler);
void EXTI2_IRQHandler(void)              WEAK_ALIAS(Default_Handler);
void EXTI3_IRQHandler(void)              WEAK_ALIAS(Default_Handler);
void EXTI4_IRQHandler(void)              WEAK_ALIAS(Default_Handler);
void DMA1_Channel1_IRQHandler(void)      WEAK_ALIAS(Default_Handler);
void DMA1_Channel2_IRQHandler(void)      WEAK_ALIAS(Default_Handler);
void DMA1_Channel3_IRQHandler(void)      WEAK_ALIAS(Default_Handler);
void DMA1_Channel4_IRQHandler(void)      WEAK_ALIAS(Default_Handler);
void DMA1_Channel5_IRQHandler(void)      WEAK_ALIAS(Default_Handler);
void DMA1_Channel6_IRQHandler(void)      WEAK_ALIAS(Default_Handler);
void DMA1_Channel7_IRQHandler(void)      WEAK_ALIAS(Default_Handler);
void ADC1_2_IRQHandler(void)             WEAK_ALIAS(Default_Handler);
void USB_HP_CAN1_TX_IRQHandler(void)     WEAK_ALIAS(Default_Handler);
void USB_LP_CAN1_RX0_IRQHandler(void)    WEAK_ALIAS(Default_Handler);
void CAN1_RX1_IRQHandler(void)           WEAK_ALIAS(Default_Handler);
void CAN1_SCE_IRQHandler(void)           WEAK_ALIAS(Default_Handler);
void EXTI9_5_IRQHandler(void)            WEAK_ALIAS(Default_Handler);
void TIM1_BRK_IRQHandler(void)           WEAK_ALIAS(Default_Handler);
void TIM1_UP_IRQHandler(void)            WEAK_ALIAS(Default_Handler);
void TIM1_TRG_COM_IRQHandler(void)       WEAK_ALIAS(Default_Handler);
void TIM1_CC_IRQHandler(void)            WEAK_ALIAS(Default_Handler);
void TIM2_IRQHandler(void)               WEAK_ALIAS(Default_Handler);
void TIM3_IRQHandler(void)               WEAK_ALIAS(Default_Handler);
void TIM4_IRQHandler(void)               WEAK_ALIAS(Default_Handler);
void I2C1_EV_IRQHandler(void)            WEAK_ALIAS(Default_Handler);
void I2C1_ER_IRQHandler(void)            WEAK_ALIAS(Default_Handler);
void I2C2_EV_IRQHandler(void)            WEAK_ALIAS(Default_Handler);
void I2C2_ER_IRQHandler(void)            WEAK_ALIAS(Default_Handler);
void SPI1_IRQHandler(void)               WEAK_ALIAS(Default_Handler);
void SPI2_IRQHandler(void)               WEAK_ALIAS(Default_Handler);
void USART1_IRQHandler(void)             WEAK_ALIAS(Default_Handler);
void USART2_IRQHandler(void)             WEAK_ALIAS(Default_Handler);
void USART3_IRQHandler(void)             WEAK_ALIAS(Default_Handler);
void EXTI15_10_IRQHandler(void)          WEAK_ALIAS(Default_Handler);
void RTC_Alarm_IRQHandler(void)          WEAK_ALIAS(Default_Handler);
void USBWakeUp_IRQHandler(void)          WEAK_ALIAS(Default_Handler);

/* BẢNG VECTOR NGẮT (Nằm trên Flash) */
__attribute__((section(".isr_vector"), used))
const isr_func_t isr_vector[] = {
    _estack,         /* Không cần ép kiểu, _estack tự động decay thành function pointer */
    Reset_Handler,   /* Không dùng dấu & nữa */
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
    /* ... (giữ nguyên các ngắt ngoại vi bên dưới như cũ) ... */
    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMPER_IRQHandler,
    RTC_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Channel1_IRQHandler,
    DMA1_Channel2_IRQHandler,
    DMA1_Channel3_IRQHandler,
    DMA1_Channel4_IRQHandler,
    DMA1_Channel5_IRQHandler,
    DMA1_Channel6_IRQHandler,
    DMA1_Channel7_IRQHandler,
    ADC1_2_IRQHandler,
    USB_HP_CAN1_TX_IRQHandler,
    USB_LP_CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_IRQHandler,
    TIM1_UP_IRQHandler,
    TIM1_TRG_COM_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    USBWakeUp_IRQHandler
};

void Reset_Handler(void)
{
    uint32_t *src;
    uint32_t *dest;

    /* Copy .data từ LMA (Flash) sang VMA (RAM) */
    src = &_sidata;
    // for (dest = &_sdata; dest < &_edata;)
    // {
    //     *dest++ = *src++;
    // }

    /* Xóa .bss */
    for (dest = &_sbss; dest < &_ebss;)
    {
        *dest++ = 0;
    }

    SystemInit();
    main();

    /* Safety trap: main() không được phép return */
    for (;;)
    {
    }
}