# Tài liệu Thực nghiệm Phá hoại có Chủ đích (Fault-Injection & Review Lab)

**Hướng dẫn:** Tạo nhánh Git tạm (`git checkout -b temp/bcm-001-experiments`) trước khi thực hiện để quan sát hành vi hệ thống khi thiếu hụt các thành phần cốt lõi.
## Câu hỏi 1: Tại sao phần tử đầu tiên của vector table là một giá trị stack pointer chứ không phải địa chỉ hàm? Nếu giá trị đó sai thì chuyện gì xảy ra ngay sau reset?

*   **Lý do:** Kiến trúc ARM Cortex-M được thiết kế để có thể viết 100% bằng C (C-friendly). Để hàm C (kể cả ngắt) chạy được, Stack phải sẵn sàng ngay lập tức. Do đó, phần cứng được thiết kế cứng (hardwired) để tự động đọc 4 byte đầu tiên tại `0x08000000` và nạp thẳng vào thanh ghi Stack Pointer (`MSP`). Sau đó nó mới đọc 4 byte tiếp theo để nạp vào Program Counter (`PC`) và nhảy tới `Reset_Handler`.
*   **Nếu sai:** Nếu giá trị này trỏ ra ngoài không gian RAM hợp lệ, thì ngay ở lệnh `PUSH` đầu tiên (khi gọi hàm con hoặc khi lưu context do có ngắt xen ngang), CPU sẽ truy cập vào một vùng nhớ vô định. Hệ thống lập tức sụp đổ và nhảy vào `HardFault` (thường là lỗi *Imprecise Data Bus Error* hoặc *Stack Fault*) ngay trong những mili-giây đầu tiên sau khi cấp nguồn.

## Câu hỏi 2: Hậu quả của việc thiếu vòng lặp copy `.data`

*   **Thao tác:** Trong `startup_stm32f103.c`, comment vòng lặp copy dữ liệu từ Flash sang RAM trong `Reset_Handler`. Giữ nguyên vòng lặp dọn `.bss`.
*   **Thực nghiệm:** Build, nạp code, đặt breakpoint tại `main` bằng GDB. Đọc giá trị 2 biến `g_initialized` (kỳ vọng `0xDEADBEEF`) và `g_zeroed` (kỳ vọng `0`).
*   **Kết quả quan sát thực tế (GDB):** 
    *   `$1 = 0xdeadf02a` (Biến `g_initialized` ra một giá trị rác ngẫu nhiên).
    *   `$2 = 0x0` (Biến `g_zeroed` vẫn giữ đúng giá trị 0).
*   **Bài học cốt lõi:** 
    1. Bộ nhớ SRAM không tự động reset về 0 khi cấp nguồn. Nếu không có code thủ công "bốc" dữ liệu tĩnh từ Flash thả vào RAM, biến toàn cục sẽ mang giá trị rác ngẫu nhiên, khiến logic chương trình sụp đổ ngay lập tức.
    2. Biến `g_zeroed` vẫn đúng vì vòng lặp lấp số 0 cho `.bss` vẫn đang hoạt động. Việc chia tách `.data` và `.bss` giúp hệ thống khởi động tối ưu hơn: những biến bằng 0 không cần lưu trên Flash, MCU chỉ việc dùng lệnh ghi siêu tốc để dọn sạch RAM (memset 0).

## Câu hỏi 3: Sức mạnh hủy diệt của trình biên dịch khi thiếu `volatile` (`-O2`)

*   **Thao tác:** Trong `app/main.c`, bỏ từ khóa `volatile` ở vòng lặp delay:
    `for (uint32_t i = 0; i < 500000; i++) {}`
    Đảm bảo mức tối ưu hóa trong CMake là `-O2` (hoặc `-Os`).
*   **Thực nghiệm:** Dùng `arm-none-eabi-objdump -d build/bcm.elf` xem mã hợp ngữ của hàm `main`.
*   **Kết quả quan sát:** 
    Vì không có volatile, trình biên dịch không cấp phát bộ nhớ (RAM/Stack) cho biến i. Vòng lặp đếm lùi này chạy hoàn toàn bên trong lõi CPU bằng thanh ghi r3. Mỗi vòng lặp chỉ tốn đúng 2 đến 3 chu kỳ máy (clock cycles). Với 500,000 vòng ở xung nhịp 8MHz, nó chạy vèo qua chỉ trong khoảng hơn 100 mili-giây (0.1s).
    *   **Hệ quả:** Khi bạn bỏ volatile ở biến đếm i, vòng lặp for (uint32_t i = 0; i < 500000; i++) không bị xóa hẳn (như một số phiên bản GCC cũ thường làm), mà bị biến đổi thành một vòng lặp siêu tốc trên thanh ghi:
    800010a: ldr r3, [pc, #20] -> GCC nạp hằng số 500000 (0x0007a120) vào thanh ghi r3.
    8000112: subs r3, #1 -> Trừ r3 đi 1.
    8000114: bne.n 8000112 -> Nếu chưa bằng 0 thì quay lại lệnh trừ.Hàm `main` đảo chân LED ở tốc độ tối đa của CPU, khiến LED nhấp nháy không kiểm soát.
*   **Bài học cốt lõi:** `volatile` là lời cảnh báo tối cao với trình biên dịch: *"Ô nhớ/thanh ghi này có thể thay đổi bất cứ lúc nào, cấm tối ưu hóa!"*. Bắt buộc dùng cho mọi thao tác truy xuất thanh ghi (Memory-Mapped I/O) hoặc vòng lặp delay busy-wait.

## Câu hỏi 4: Vì sao `--gc-sections` cần đi cùng `-ffunction-sections`? Vector table không được ai gọi tới — tại sao nó không bị linker xóa mất, và bạn đã chặn chuyện đó bằng cách nào?

*   **Sự phụ thuộc:** `-ffunction-sections` ra lệnh cho Compiler đóng gói mỗi hàm vào một "ngăn" (section) riêng biệt (vd: `.text.main`, `.text.delay`). Khi đó, `--gc-sections` (Garbage Collection) của Linker mới có thể cắt tỉa từng ngăn độc lập nếu không có ai gọi đến chúng. Nếu không có `-ffunction-sections`, toàn bộ code nằm chung trong một khối `.text` khổng lồ, chỉ cần 1 hàm được gọi là Linker buộc phải giữ lại toàn bộ khối đó (không dọn được rác).
*   **Bảo vệ Vector Table:** Bảng vector không được gọi bởi phần mềm (C code) mà được gọi bởi *Phần cứng*. Trình dọn rác của Linker không biết điều này nên sẽ coi nó là code thừa và xóa mất. Ta đã chặn lại bằng hai ổ khóa:
    1. Bọc khối section trong file Linker bằng từ khóa `KEEP(*(.isr_vector))`.
    2. Gắn thuộc tính `__attribute__((used))` vào mảng ở code C để cấm Compiler vứt bỏ mảng này trong quá trình tối ưu.

## Câu hỏi 5: Chuyện gì xảy ra khi quên bật Clock cho ngoại vi?

*   **Thao tác:** Trong `app/main.c`, comment dòng lệnh cấp clock cho PORT C:
    `// RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;`
*   **Thực nghiệm:** Build, nạp code xuống board và quan sát.
*   **Kết quả quan sát:** 
    Hệ thống bus ngắt mạch cấp điện clock đến khối GPIOC. Khi CPU cố ghi giá trị vào thanh ghi `CRH` của ngoại vi đang "khóa băng" clock, tập lệnh không thể hoàn tất. MCU có thể rơi vào `HardFault` (BusFault), chip đứng hình, mạch ngưng hoạt động.
*   **Bài học cốt lõi:** **"Luôn bật Clock trước, cấu hình ngoại vi sau"**. Mọi thao tác truy xuất thanh ghi khi chưa cấp clock đều là hành vi bất hợp pháp đối với phần cứng.

## Câu hỏi 6: `nosys.specs` làm gì? Điều gì sẽ xảy ra khi link nếu gọi `printf` mà không có nó?

*   **Bản chất:** `nosys.specs` báo cho GCC Linker sử dụng thư viện `libnosys`. Đây là thư viện cung cấp các "hàm giả" (stub functions) trống rỗng. 
*   **Giải thích:** Các hàm của C Standard Library (như `printf`, `malloc`) ở tầng dưới cùng sẽ phải gọi xuống Hệ Điều Hành (System Calls) như `_write` (để in ra màn hình) hay `_sbrk` (để xin cấp RAM). Trên hệ thống Bare-metal không hề có hệ điều hành nào tồn tại.
*   **Hậu quả nếu thiếu:** Nếu bạn gọi `printf` mà không có `nosys.specs`, bước Compile sẽ qua (vì đã include `<stdio.h>`), nhưng bước Linker sẽ đánh Fail toàn tập với hàng loạt thông báo lỗi báo đỏ: `undefined reference to '_write'`, `undefined reference to '_sbrk'`. Thêm `nosys.specs` giúp xoa dịu Linker bằng cách cung cấp các hàm dummy rỗng trả về lỗi, cho phép quá trình build thành công.