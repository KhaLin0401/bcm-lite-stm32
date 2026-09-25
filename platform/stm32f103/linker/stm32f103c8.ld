/* 1. KHAI BÁO ENTRY POINT */
ENTRY(Reset_Handler)

/* 2. KHAI BÁO BẢN ĐỒ BỘ NHỚ VẬT LÝ */
/* Mục 6: Sửa LENGTH FLASH thành 64K chuẩn spec cho STM32F103C8 */
MEMORY
{
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
}

/* Mục 4: Chuyển _estack xuống dưới MEMORY để tránh forward reference */
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Stack_Size = 0x800; /* 2KB */

/* 3. ĐỊNH TUYẾN CÁC SECTION */
SECTIONS
{
  /* --- Vùng chứa Bảng Vector Ngắt --- */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* --- Vùng chứa Mã lệnh (Code thuần túy) --- */
  .text :
  {
    . = ALIGN(4);
    *(.text)           
    *(.text*)          
    . = ALIGN(4);
    _etext = .;        
  } >FLASH

  /* Mục 5: Tách riêng .rodata ra khỏi .text */
  /* Việc tách này giúp file .map và lệnh size phân biệt rõ Code và Const Data */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* Mục 3: Định tuyến tường minh cho .ARM.extab và .ARM.exidx (Stack Unwinding) */
  .ARM.extab :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM.exidx :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__exidx_start = .);
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    PROVIDE_HIDDEN (__exidx_end = .);
    . = ALIGN(4);
  } >FLASH

  /* --- Lưu lại địa chỉ LMA của .data --- */
  /* Lưu ý: _sidata phải nằm SAU tất cả các section lưu trên FLASH (text, rodata, arm.exidx) */
  _sidata = LOADADDR(.data);

  /* --- Vùng chứa Biến toàn cục có khởi tạo (.data) --- */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        
    *(.data)           
    *(.data*)
    . = ALIGN(4);
    _edata = .;        
  } >RAM AT> FLASH

  /* --- Vùng chứa Biến toàn cục không khởi tạo (.bss) --- */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;         
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;         
  } >RAM
}

  /* --- ASSERT kiểm tra RAM cho Stack --- */
  ASSERT(_estack - _ebss >= _Min_Stack_Size, "Loi Linker: Khong con du RAM cho Stack! Nguy co Stack Overflow.");