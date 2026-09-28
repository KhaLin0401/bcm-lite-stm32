# Tài liệu Thực nghiệm Phá hoại có Chủ đích (Fault-Injection & Review Lab)

**Hướng dẫn:** Tạo nhánh Git tạm (`git checkout -b temp/bcm-001-experiments`) trước khi thực hiện để quan sát hành vi hệ thống khi thiếu hụt các thành phần cốt lõi.
## Câu hỏi 1: Tại sao phần tử đầu tiên của vector table là một giá trị stack pointer chứ không phải địa chỉ hàm? Nếu giá trị đó sai thì chuyện gì xảy ra ngay sau reset?

*   **Lý do:** Kiến trúc ARM Cortex-M được thiết kế để có thể viết 100% bằng C (C-friendly). Để hàm C (kể cả ngắt) chạy được, Stack phải sẵn sàng ngay lập tức. Do đó, phần cứng được thiết kế cứng (hardwired) để tự động đọc 4 byte đầu tiên tại `0x08000000` và nạp thẳng vào thanh ghi Stack Pointer (`MSP`). Sau đó nó mới đọc 4 byte tiếp theo để nạp vào Program Counter (`PC`) và nhảy tới `Reset_Handler`.
*   **Nếu sai:** Nếu giá trị này trỏ ra ngoài không gian RAM hợp lệ, thì ngay ở lệnh `PUSH` đầu tiên (khi gọi hàm con hoặc khi lưu context do có ngắt xen ngang), CPU sẽ truy cập vào một vùng nhớ vô định. Hệ thống lập tức sụp đổ và nhảy vào `HardFault` (thường là lỗi *Imprecise Data Bus Error* hoặc *Stack Fault*) ngay trong những mili-giây đầu tiên sau khi cấp nguồn.

## Câu hỏi 2: Hậu quả của việc thiếu vòng lặp copy `.data`

*   **Thao tác:** Trong `startup_stm32f103.c`, comment vòng lặp copy dữ liệu từ Flash sang RAM trong `Reset_Handler`.
*   **Thực nghiệm:** Build, nạp code, đặt breakpoint tại `main` và kiểm tra giá trị `g_initialized` (vốn là `0xDEADBEEFu`).
*   **Kết quả quan sát:** Biến `g_initialized` mang giá trị `0` hoặc giá trị rác ngẫu nhiên.
*   **Bài học cốt lõi:** Biến toàn cục có giá trị khởi tạo không tự bay vào RAM. Không có code dời dữ liệu thủ công từ Flash (nơi lưu trữ tĩnh) vào RAM (nơi thực thi), trạng thái ban đầu của phần mềm sẽ sụp đổ.

## Câu hỏi 3: Sức mạnh hủy diệt của trình biên dịch khi thiếu `volatile` (`-O2`)

*   **Thao tác:** Trong `app/main.c`, bỏ từ khóa `volatile` ở vòng lặp delay:
    `for (uint32_t i = 0; i < 500000; i++) {}`
    Đảm bảo mức tối ưu hóa trong CMake là `-O2` (hoặc `-Os`).
*   **Thực nghiệm:** Dùng `arm-none-eabi-objdump -d build/bcm.elf` xem mã hợp ngữ của hàm `main`.
*   **Kết quả quan sát:** 
    Theo **As-if Rule** của chuẩn C, trình biên dịch thấy vòng lặp không thay đổi trạng thái phần cứng hay biến toàn cục ra ngoài. Nó **xóa sạch toàn bộ vòng lặp** khỏi mã máy đầu ra.
*   **Hệ quả:** Hàm `main` đảo chân LED ở tốc độ tối đa của CPU, khiến LED sáng mờ hoặc nhấp nháy không kiểm soát.
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