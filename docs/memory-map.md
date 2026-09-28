# Phân tích Bản đồ Bộ nhớ & Tiến trình Định tuyến (Memory Map & Linker Evolution)

Tài liệu này ghi lại quá trình định hình bản đồ bộ nhớ (Memory Map) cho dự án `bcm-lite-stm32` chạy trên vi điều khiển STM32F103C8T6 (64KB Flash, 20KB RAM), thông qua việc phân tích sự tiến hóa của Linker Script và các mốc kiểm chứng thực tế.

## 1. Tiến trình 3 giai đoạn của `objdump -h`

Trong quá trình xây dựng hệ thống build từ đầu, cấu trúc các section trong file `.elf` đã trải qua 3 trạng thái biến đổi rõ rệt:

1. **Giai đoạn 1: Mặc định của Toolchain (Không có Linker Script tùy chỉnh)**
   * **Hiện tượng:** Trình biên dịch sử dụng Linker Script mặc định ẩn bên trong bộ toolchain GCC.
   * **Hệ quả:** Đoạn mã `.text` bị đẩy về các địa chỉ giả định (như `0x00008000` hoặc lệch pha như `0x8018`). Nếu nạp xuống phần cứng thật, chip sẽ vấp lỗi `HardFault` ngay lập tức do trỏ vào vùng nhớ không được ánh xạ.

2. **Giai đoạn 2: Bị cờ `--gc-sections` quét sạch (Có `.ld` nhưng thiếu Entry Point)**
   * **Hiện tượng:** Khi áp dụng file `.ld` tự viết nhưng chưa có file Startup (`startup_stm32f103.c`) chứa `Reset_Handler`, Linker không tìm thấy điểm neo (entry point). 
   * **Hệ quả:** Cờ `--gc-sections` lầm tưởng toàn bộ code là "rác" không ai gọi đến và cắt bỏ hoàn toàn. Bảng `objdump -h` lúc này cho thấy Flash trống trơn (0 bytes).

3. **Giai đoạn 3: Định tuyến hoàn chỉnh (Neo vững chắc tại Flash phần cứng)**
   * **Hiện tượng:** Sau khi tích hợp file Startup, định nghĩa tường minh `.isr_vector` với thuộc tính `KEEP`, và trỏ đúng Entry Point.
   * **Hệ quả:** Toàn bộ các section `.text`, `.rodata`, `.data` quay về đúng địa chỉ vật lý bắt đầu từ `0x08000000`.

## 2. Giải thích cơ chế VMA và LMA trong section `.data`

Trong file Linker Script, định tuyến cho `.data` được viết:
`>RAM AT> FLASH`

*   **VMA (Virtual Memory Address):** Là địa chỉ mà CPU sẽ **đọc/ghi** khi chương trình đang chạy. Ở đây ta gán nó vào `>RAM` (`0x2000xxxx`) để tốc độ truy xuất đạt mức tối đa.
*   **LMA (Load Memory Address):** Là địa chỉ vật lý nơi dữ liệu được **lưu trữ tĩnh** khi ngắt điện. Ta đặt nó ở `AT> FLASH` (`0x0800xxxx`).
*   **Ý nghĩa thực tế:** Khai báo `uint32_t g_initialized = 0xDEADBEEF;`, giá trị `0xDEADBEEF` được nung cứng trên Flash. Khi MCU khởi động, `Reset_Handler` sẽ copy khối dữ liệu này từ Flash sang vùng RAM tương ứng.

## 3. Bản đồ Sử dụng Bộ nhớ (Memory Utilization)

| Memory Region | Used Size | Maximum Size | Utilization (%) | Ý nghĩa |
|---|---|---|---|---|
| **FLASH** | ~372 Bytes | 64 KB | ~0.56% | Chứa Bảng Vector (`.isr_vector`), Mã lệnh (`.text`), Hằng số (`.rodata`) và giá trị khởi tạo của `.data`. |
| **RAM** | 0 Bytes (Tĩnh) / ~8 Bytes (Động) | 20 KB | ~0.04% | Không tốn dung lượng tĩnh trên Flash cho biến `.bss`, chỉ cấp phát vùng nhớ khi chạy. |

*   **Tại sao có sự chênh lệch (ví dụ 484 bytes tổng ELF so với 372 bytes code)?**
    Con số 372 bytes là kích thước thực tế mã máy nạp vào Flash. 484 bytes trong file `.elf` bao gồm bảng ký hiệu (Symbol Tables), Header ELF, và thông tin Debug, vốn chỉ dùng cho GDB chứ không chiếm không gian trên silicon.

## 4. Giải mã 4 byte đầu tiên của Bảng Vector Ngắt

Tại địa chỉ `0x08000000`:
1.  **4 byte đầu tiên (`0x08000000`):** Lưu trữ giá trị `0x20005000` (`_estack` - Đỉnh của RAM). Phần cứng tự động nạp giá trị này vào thanh ghi Stack Pointer (`SP`).
2.  **4 byte tiếp theo (`0x08000004`):** Lưu trữ địa chỉ của hàm `Reset_Handler` (ví dụ `0x080000f1`). Phần cứng tự động nạp vào Program Counter (`PC`) để bắt đầu thực thi chuỗi khởi động.

## 5. Rationale của lệnh `ASSERT` kiểm tra Stack

`ASSERT(_estack - _ebss >= _Min_Stack_Size, "Loi Linker: Khong con du RAM cho Stack!");`

*   **Con số chọn:** `_Min_Stack_Size = 0x800` (2KB, 10% RAM của STM32F103).
*   **Lý do:** Stack phát triển từ trên xuống (`_estack`), `.data` và `.bss` phát triển từ dưới lên. Lệnh `ASSERT` ép Linker dừng build ở Compile-time nếu RAM dư không đủ 2KB, ngăn chặn tuyệt đối lỗi Stack Overflow ngầm lúc Runtime.