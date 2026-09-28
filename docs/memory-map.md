# Phân tích Bản đồ Bộ nhớ & Tiến trình Định tuyến (Memory Map & Linker Evolution)

Tài liệu này ghi lại quá trình định hình bản đồ bộ nhớ (Memory Map) cho dự án `bcm-lite-stm32` chạy trên vi điều khiển STM32F103C8T6 (64KB Flash, 20KB RAM), qua đó làm rõ những cạm bẫy của quá trình biên dịch tĩnh.

## 1. Tiến trình 3 giai đoạn của `objdump -h` (Bài học về "Silent Success")

Trong quá trình xây dựng hệ thống build, cấu trúc file `.elf` đã trải qua 3 trạng thái biến đổi. Đây là minh chứng rõ nhất cho sự nguy hiểm của lỗi "Thành công ảo" (Silent Success):

1. **Giai đoạn 1: Có Linker script trên đĩa nhưng quên cờ `-T`**
   * **Hiện tượng:** File `.ld` đã được viết xong, nhưng ta không truyền cờ `-T` cho CMake. Trình biên dịch âm thầm dùng linker mặc định.
   * **Hệ quả:** Mã lệnh `.text` bị đẩy về địa chỉ rác `0x8018`. Điểm chết người là **quá trình build vẫn báo Success (xanh)**. Nếu nạp xuống phần cứng, vi điều khiển sẽ vấp HardFault ngay lập tức.
2. **Giai đoạn 2: Cấp cờ `-T` nhưng thiếu Entry Point**
   * **Hiện tượng:** Linker đã đọc file `.ld` nhưng không tìm thấy hàm `Reset_Handler`. 
   * **Hệ quả:** Cờ `--gc-sections` quét qua và xóa sạch toàn bộ code vì coi đó là phần rác không ai gọi tới. **Build vẫn báo xanh**, nhưng file đầu ra trống rỗng (Flash 0 byte).
3. **Giai đoạn 3: Định tuyến hoàn chỉnh**
   * Sau khi thêm Startup, đặt `KEEP` cho `.isr_vector` và trỏ đúng Entry Point, `.text` và các section khác mới thực sự được neo chặt vào đúng địa chỉ vật lý `0x08000000`.

## 2. Giải thích cơ chế VMA và LMA trong section `.data`

Lệnh định tuyến cho `.data`: `>RAM AT> FLASH`

*   **VMA (Virtual Memory Address):** Địa chỉ CPU **đọc/ghi** khi chạy (`>RAM`, `0x2000xxxx`). Tốc độ truy xuất tối đa.
*   **LMA (Load Memory Address):** Địa chỉ vật lý **lưu trữ tĩnh** khi ngắt điện (`AT> FLASH`, `0x0800xxxx`).
*   **Thực tế:** Biến `uint32_t g_initialized = 0xDEADBEEF;` được nung cứng trên Flash. Khi khởi động, vòng lặp trong `Reset_Handler` sẽ copy nó sang RAM.

## 3. Giải phẫu kích thước file phẳng `.bin` và các khoảng đệm

File `.bin` là một ảnh phẳng nhị phân thô (flat binary). Cơ chế hoạt động của lệnh `objcopy -O binary` rất rạch ròi: Nó lọc lấy **tất cả các section mang cờ `ALLOC` và `LOAD`**, sau đó trải phẳng chúng từ LMA thấp nhất đến LMA cao nhất.

Dưới đây là bảng tính dung lượng dựa trên output `objdump -h` của bản build hiện tại (đã bổ sung 2 biến toàn cục `g_initialized` và `g_zeroed`):

| Tên Section | Cờ (Flags) | LMA | Kích thước (Size) |
|---|---|---|---|
| `.isr_vector` | ALLOC, LOAD | `0x08000000` | `0xec` (236 bytes) |
| `.text`       | ALLOC, LOAD | `0x080000ec` | `0x9c` (156 bytes) |
| `.rodata`     | ALLOC, LOAD | `0x08000188` | `0x00` (0 bytes) |
| `.data`       | ALLOC, LOAD | `0x08000188` | `0x04` (4 bytes) |

*(Lưu ý: Section `.bss` có kích thước `0x04` byte để chứa biến `g_zeroed`, nhưng nó chỉ có cờ `ALLOC` mà không có cờ `LOAD`, do đó nó hoàn toàn không chiếm dung lượng trong file `.bin`).*

**Phép toán xác thực độ dài file:**
1. **LMA cao nhất** thuộc về section `.data`: `0x08000188`.
2. **Kích thước** của section `.data`: `0x04`.
3. **Điểm kết thúc vật lý:** `0x08000188 + 0x04 = 0x0800018C`.
4. **Độ dài file `.bin`:** Điểm kết thúc (`0x0800018C`) trừ đi Điểm bắt đầu (`0x08000000`) = **`0x18C` (396 bytes)**.

File `bcm.bin` có dung lượng chính xác tuyệt đối là 396 bytes. Mọi byte đều được tính toán và kiểm soát khép kín. Không có byte nào bị bỏ sót.

**Quyết định thiết kế đối với `.ARM.exidx` và `.ARM.extab`:**
*   **Quyết định:** Thêm vào khối `/DISCARD/` trong file Linker Script để vứt bỏ vĩnh viễn.
*   **Lý do:** Hai section này chứa thông tin cho Exception Unwinding (truy ngược stack khi có lỗi) của C++ và thư viện chuẩn. Trong dự án Bare-metal C nghiêm ngặt, ta không dùng exception và không có hệ điều hành để hỗ trợ backtrace. Dù hiện tại kích thước của chúng đang bằng `0`, nhưng để lại chúng là một rủi ro tiềm ẩn. Việc chủ động vứt bỏ triệt để ngay từ bây giờ đảm bảo file `.bin` sẽ không bao giờ bị phình to bởi những metadata lãng phí này khi code lớn lên.

## 4. Giải mã 4 byte đầu tiên của Bảng Vector & Chế độ Thumb

Tại địa chỉ Flash `0x08000000`[cite: 1]:
1.  **4 byte đầu (`0x08000000`):** Giá trị `0x20005000` (tương ứng `_estack`). Ngay khi có điện, phần cứng tự động nạp nó vào thanh ghi Stack Pointer (`MSP`).
2.  **4 byte sau (`0x08000004`):** Giá trị `0x08000109`.
    *   **Bản chất Bit 0:** Nếu dò trong mã máy, địa chỉ thực của `Reset_Handler` là `0x08000108`. Tuy nhiên, trình biên dịch đã chủ động set bit thấp nhất (bit 0) lên `1` (thành `0x08000109`). Đây là cờ báo cho vi xử lý Cortex-M biết phải thực thi tập lệnh ở **chế độ Thumb (Thumb state)**. Nếu bit này là `0`, CPU sẽ hiểu lầm là tập lệnh ARM 32-bit và văng lỗi HardFault ngay lập tức.

## 5. Ràng buộc `ASSERT` kiểm tra Stack

`ASSERT(_estack - _ebss >= _Min_Stack_Size, "Loi Linker: Khong con du RAM cho Stack!");`

*   **Phạm vi tác dụng:** Đây thuần túy là **ràng buộc tại thời điểm link (Link-time constraint)**. Nó kiểm tra khoảng trống tĩnh còn lại giữa `.bss` và đỉnh RAM có đủ 2KB hay không.
*   **Giới hạn:** Nó **không** bảo vệ hay ngăn chặn được lỗi Stack Overflow lúc chạy (Runtime). Lệnh này không biết gì về đệ quy, ISR lồng nhau, hay mảng cục bộ. Stack vẫn có thể tràn lúc chạy. Cơ chế phát hiện tràn stack tại runtime là việc của các ticket khác (như cấu hình MPU hoặc stack canaries).