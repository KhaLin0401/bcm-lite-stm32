# BCM-Lite on STM32 — Project Plan

> Mô phỏng một Body Control Module ô tô trên nền tảng STM32, theo kiến trúc và quy trình AUTOSAR Classic.
> Mục tiêu: xây dựng năng lực automotive firmware thực chiến trong ~5 tháng, trong một repo duy nhất.

| | |
|---|---|
| **Owner** | (tên bạn) |
| **Role** | Junior Embedded SW Engineer |
| **Bắt đầu** | 2026-09-14 |
| **Dự kiến hoàn thành** | 2027-02 |
| **Repo** | `bcm-lite-stm32` (một repo duy nhất, commit history sạch) |

---

## 1. Bối cảnh & phạm vi

### 1.1 Feature scope

Module mô phỏng gồm 4 nhóm chức năng của một BCM thật:

- **Exterior lighting** — đèn xi-nhan, hazard, đèn sương mù, welcome light
- **Door lock** — khóa/mở từ remote key qua CAN
- **Window lift** — auto close khi khóa xe
- **Wiper** — điều khiển tốc độ theo tín hiệu

### 1.2 Kiến trúc node

```
┌─────────────────┐         CAN bus          ┌─────────────────┐
│   Board A       │◄────────────────────────►│   Board B       │
│   BCM (DUT)     │      120Ω        120Ω    │  ECU Simulator  │
│                 │                          │  (VehicleSpeed, │
│  - MCAL         │                          │   DoorStatus,   │
│  - Com/CanIf    │              ▲           │   RemoteKey)    │
│  - Dcm/Dem/NvM  │              │           └─────────────────┘
│  - App SWCs     │              │
└─────────────────┘              │
                        ┌────────┴────────┐
                        │  USB-CAN dongle │
                        │  (Tester / PC)  │
                        └─────────────────┘
```

### 1.3 Điều gì map được từ dự án thật, điều gì không

| Trong dự án automotive thật | Thay thế trên STM32 | Mức độ map |
|---|---|---|
| CAN FD / CAN 2.0B | FDCAN (G4/H7) hoặc bxCAN (F1/F4) | Đầy đủ |
| LIN | USART LIN mode | Đầy đủ |
| UDS / ISO-TP | Tự implement theo ISO 14229 / 15765-2 | Đầy đủ — giá trị cao nhất |
| Dem / DTC / debounce / aging | Tự implement theo spec | Đầy đủ |
| NvM + CRC | Flash EEPROM emulation | Đầy đủ |
| Kiến trúc phân lớp AUTOSAR | Tự dựng MCAL / ECUAL / Service / App | Map khái niệm |
| MISRA C:2012 | `cppcheck --addon=misra` | ~60% rule — đủ học tư duy |
| VectorCAST | Ceedling (Unity + CMock) + gcov | Đầy đủ |
| EB tresos / DaVinci Configurator | **Không có** | Khoảng trống |
| CANoe / CAPL | python-can + cantools + SavvyCAN | Một phần |
| Work product ISO 26262 | **Không có** | Chỉ học lý thuyết |

> Ba khoảng trống cuối là thật. Khi phỏng vấn, nói thẳng: *"Em chưa dùng tool, nhưng em hiểu nó giải quyết vấn đề gì."* Đáng tin hơn nói vòng vo.

---

## 2. Chuẩn bị

### 2.1 Phần cứng (ngân sách < 1.5 triệu VND)

- [ ] 2× **Nucleo-G474RE** (có FDCAN) — hoặc rẻ hơn: 2× Blue Pill STM32F103 + ST-Link V2
- [ ] 2× **CAN transceiver** TJA1050 hoặc SN65HVD230
- [ ] 2× điện trở **120Ω** terminate hai đầu bus
- [ ] 1× **USB-CAN analyzer** (CANable / CANable Pro) — đóng vai CANoe
- [ ] 1× **logic analyzer 8 kênh** + PulseView — soi timing, LIN frame, PWM
- [ ] 1× module **INA219** (đo dòng cho Milestone 6)
- [ ] LED, nút bấm, relay module, breadboard, dây

> **Bắt buộc 2 board.** Một node thì không có bus, không có gì để test.

### 2.2 Phần mềm

- [ ] `arm-none-eabi-gcc` + CMake + Ninja (không phụ thuộc CubeIDE auto-generate)
- [ ] OpenOCD hoặc pyOCD để flash/debug
- [ ] **Ceedling** (Unity + CMock) cho unit test
- [ ] **cppcheck** + MISRA addon
- [ ] **python-can** + **cantools** (parse file DBC tự viết)
- [ ] **SavvyCAN** để xem trace
- [ ] GitHub Actions cho CI

### 2.3 Tài liệu cần tải

- [ ] AUTOSAR `SWS_Dio`, `SWS_Pwm`, `SWS_Adc`, `SWS_Can` — miễn phí trên autosar.org
- [ ] ISO 14229-1 (UDS) — tìm bản tóm tắt nếu không mua được
- [ ] ISO 15765-2 (ISO-TP) — có nhiều tài liệu mô tả đầy đủ public
- [ ] Reference Manual của MCU đang dùng

---

## 3. Milestones

### Milestone 1 — Nền móng (2–3 tuần)

**Mục tiêu:** Build system và scheduler của riêng bạn, kiến trúc phân lớp đúng từ ngày đầu.

- [ ] Dựng CMake project build bằng `arm-none-eabi-gcc`, tự viết linker script và startup file
- [ ] Hiểu và ghi chú lại memory map: `.text`, `.data`, `.bss`, stack, heap, vector table
- [ ] Dựng **cooperative scheduler**: task 1ms / 10ms / 100ms chạy từ SysTick (mô hình AUTOSAR OS basic task)
- [ ] Đo **CPU load** và **WCET** từng task bằng DWT cycle counter, ghi thành bảng trong README
- [ ] Cấu trúc thư mục phân lớp rõ ràng, tầng trên không include header tầng dưới ngoài interface

**Cấu trúc thư mục đề xuất**

```
bcm-lite-stm32/
├── app/              # SWC: LightMgr, DoorMgr, WindowMgr, WiperMgr
├── service/          # Com, Dcm, Dem, NvM, EcuM, Scheduler
├── ecual/            # Led, Button, Relay — driver mức thiết bị
├── mcal/             # Dio, Pwm, Adc, Can, Mcu — API theo AUTOSAR
├── common/           # Std_Types.h, Compiler.h, Platform_Types.h
├── config/           # Cấu hình tĩnh từng module
├── test/             # Ceedling unit test
├── tools/            # Script Python, file .dbc
└── docs/             # Architecture, CAN matrix, DTC list, test report
```

**Acceptance criteria**

- Build sạch, zero warning với `-Wall -Wextra -Werror`
- Jitter của task 1ms đo được < 50µs
- CPU load < 30% ở trạng thái idle-ish
- Bảng WCET có số thật, không phải ước lượng

**Definition of Done**

> Đưa bạn một board khác (F103 thay G474), bạn **chỉ phải thay tầng MCAL**, không đụng tầng App.
> Nếu phải sửa App → kiến trúc sai, làm lại.

---

### Milestone 2 — MCAL theo chuẩn AUTOSAR (2 tuần)

**Mục tiêu:** Viết được MCAL AUTOSAR-compliant thật, không phải mô phỏng cho có.

- [ ] Viết `Std_Types.h`, `Platform_Types.h`, `Compiler.h` theo đúng spec
- [ ] Implement **Dio** theo `SWS_Dio` — đúng chữ ký API
- [ ] Implement **Pwm** theo `SWS_Pwm`
- [ ] Implement **Adc** theo `SWS_Adc`
- [ ] Det error reporting (`Det_ReportError`) cho tất cả API
- [ ] Cấu hình tĩnh kiểu `PostBuild` — config tách khỏi code

**Chữ ký API phải đúng chuẩn**

```c
void            Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);
Dio_LevelType   Dio_ReadChannel(Dio_ChannelType ChannelId);
Std_ReturnType  Adc_StartGroupConversion(Adc_GroupType Group);
void            Pwm_SetDutyCycle(Pwm_ChannelType ChannelNumber, uint16 DutyCycle);
```

**Acceptance criteria**

- Mọi API trả `Std_ReturnType` với `E_OK` / `E_NOT_OK`
- Gọi API với ChannelId không hợp lệ → Det báo lỗi, không crash
- Không có magic number trong code, tất cả qua config

**Definition of Done**

> Đây là task ROI cao nhất cho CV. Xong milestone này, câu *"em đã viết MCAL AUTOSAR-compliant"* là sự thật có thể chứng minh.

---

### Milestone 3 — CAN + DBC + Com layer (3 tuần)

**Mục tiêu:** Có bus thật, có signal thật, có bảo vệ dữ liệu thật.

- [ ] Tự viết file `.dbc` với ~10 signal: `VehicleSpeed`, `DoorStatus`, `TurnIndicatorReq`, `LightCmd`, `RemoteKeyCmd`, `WiperReq`, ...
- [ ] Implement **CanIf** — abstraction trên driver CAN
- [ ] Implement **Com layer**: signal packing/unpacking, byte order (Intel/Motorola), scaling + offset
- [ ] Cyclic TX 100ms cho các PDU định kỳ
- [ ] **RX timeout detection** — phát hiện mất tín hiệu, đưa signal về substitution value
- [ ] Board B + script python-can đóng vai ECU khác, gửi frame theo DBC bằng `cantools`
- [ ] Implement **E2E Protection Profile 1**: CRC8 + Alive Counter cho một safety signal
- [ ] Fault injection: cố tình gửi CRC sai, counter nhảy cóc, frame lặp — chứng minh phát hiện được

**Acceptance criteria**

- Trace trên SavvyCAN khớp chính xác với file DBC
- Signal Motorola byte order pack/unpack đúng (đây là chỗ hay sai nhất)
- RX timeout kích hoạt đúng sau đúng thời gian cấu hình
- Báo cáo fault injection: 5 loại lỗi, 5 lần phát hiện

---

### Milestone 4 — UDS + Bootloader ⭐ (4 tuần)

**Mục tiêu:** Đây là phần làm bạn khác biệt hẳn so với junior khác. Đừng cắt ngắn.

#### 4.1 ISO-TP (ISO 15765-2)

- [ ] Single Frame, First Frame, Consecutive Frame, Flow Control
- [ ] Xử lý `STmin` và `BlockSize`
- [ ] Timeout N_As / N_Bs / N_Cr
- [ ] **Tự viết, không copy lib.** Copy lib thì không học được gì.

#### 4.2 UDS Server (ISO 14229)

- [ ] `0x10` DiagnosticSessionControl — Default / Programming / Extended
- [ ] `0x22` ReadDataByIdentifier — bao gồm DID `F190` (VIN)
- [ ] `0x2E` WriteDataByIdentifier
- [ ] `0x27` SecurityAccess — seed/key, có delay timer khi sai key
- [ ] `0x14` ClearDiagnosticInformation
- [ ] `0x19` ReadDTCInformation — ít nhất sub-function `0x02`
- [ ] `0x31` RoutineControl
- [ ] `0x3E` TesterPresent + S3 session timeout
- [ ] Negative Response Code đúng chuẩn: `0x11`, `0x12`, `0x13`, `0x22`, `0x31`, `0x33`, `0x78`

#### 4.3 Dem — Diagnostic Event Manager

- [ ] DTC cho open-load đèn sương mù (đọc ADC dòng tải)
- [ ] **Debounce counter** — không set DTC ngay lần fail đầu
- [ ] **DTC status byte 8 bit** đầy đủ: `testFailed`, `testFailedThisOperationCycle`, `pendingDTC`, `confirmedDTC`, `testNotCompletedSinceLastClear`, ...
- [ ] **Aging** — DTC tự xóa sau 40 driving cycle không tái phát
- [ ] Freeze frame lưu VehicleSpeed tại thời điểm lỗi

> Chỗ junior sai nhiều nhất: nhầm giữa **event status** và **DTC status byte**. Làm đúng là điểm cộng lớn khi phỏng vấn.

#### 4.4 CAN Bootloader

- [ ] `0x34` RequestDownload / `0x36` TransferData / `0x37` RequestTransferExit
- [ ] CRC32 check toàn bộ application trước khi jump
- [ ] A/B partition hoặc valid-flag để chống brick
- [ ] Script Python flash firmware qua CAN

**Acceptance criteria**

- Flash được firmware mới hoàn toàn qua CAN, không cần ST-Link
- **Rút dây nguồn giữa lúc flash 10 lần — board vẫn boot được 10/10**
- Tester script chạy full UDS sequence, in ra pass/fail từng service

---

### Milestone 5 — Application + Test Automation (3 tuần)

**Mục tiêu:** Logic ứng dụng đúng spec, và chứng minh được là đúng.

#### 5.1 Turn Indicator / Hazard SWC

- [ ] State machine: Off / LeftTurn / RightTurn / Hazard
- [ ] Chu kỳ nháy **340ms ±10%**
- [ ] Hazard đè lên xi-nhan khi cả hai cùng active
- [ ] Bulb outage → nháy nhanh gấp 2× (170ms) + set DTC
- [ ] **Dùng logic analyzer chứng minh timing đạt** — chụp ảnh đưa vào docs

#### 5.2 NvM

- [ ] Flash EEPROM emulation, wear leveling cơ bản
- [ ] Lưu welcome-light preference
- [ ] CRC check khi đọc, fallback về default khi CRC fail
- [ ] Default value khi first boot (flash trắng)
- [ ] **Chịu được mất điện lúc đang ghi** — test bằng cách rút nguồn 20 lần

#### 5.3 Unit test

- [ ] Ceedling cho toàn bộ tầng App và Service
- [ ] Mock tầng dưới bằng CMock
- [ ] Statement coverage **≥ 90%**, branch coverage **≥ 85%**
- [ ] Xuất báo cáo gcov vào `docs/coverage/`

#### 5.4 Test automation (bản nghèo của CAPL)

- [ ] Script Python: gửi CAN frame → đọc phản hồi → assert kết quả
- [ ] Test suite cho: lighting logic, UDS services, DTC lifecycle, NvM persistence
- [ ] Xuất báo cáo HTML pass/fail

#### 5.5 CI — GitHub Actions

- [ ] Mỗi push: build → unit test → cppcheck MISRA
- [ ] **Fail build nếu coverage tụt** dưới ngưỡng
- [ ] Badge trạng thái trên README

---

### Milestone 6 — Sleep/Wake & Tài liệu (2 tuần)

- [ ] ECU vào **STOP mode** khi bus im lặng 5 giây
- [ ] Wake-up bằng CAN RX pin (EXTI)
- [ ] Đo dòng tiêu thụ bằng INA219: active vs sleep — ghi số thật vào docs
- [ ] Implement **EcuM** state machine đơn giản: Startup / Run / Sleep / Wakeup
- [ ] Viết `docs/architecture.md` — sơ đồ layer, mô tả từng tầng
- [ ] Viết `docs/can_matrix.md` — bảng signal đầy đủ
- [ ] Viết `docs/dtc_list.md` — DTC code, điều kiện set, điều kiện clear
- [ ] Viết `docs/test_report.md` — kết quả test, ảnh logic analyzer, coverage report
- [ ] Viết **README** như một tài liệu kỹ thuật thật

> Người phỏng vấn đọc README trong 3 phút và biết ngay bạn ở level nào. Đừng viết README hời hợt sau khi đã làm 5 tháng.

---

## 4. Timeline tổng

| Milestone | Thời lượng | Tuần | Deliverable chính |
|---|---|---|---|
| M1 — Nền móng | 2–3 tuần | 1–3 | Build system, scheduler, WCET report |
| M2 — MCAL AUTOSAR | 2 tuần | 4–5 | Dio/Pwm/Adc theo SWS |
| M3 — CAN + Com | 3 tuần | 6–8 | DBC, Com layer, E2E profile 1 |
| M4 — UDS + Bootloader ⭐ | 4 tuần | 9–12 | ISO-TP, UDS server, Dem, bootloader |
| M5 — App + Test | 3 tuần | 13–15 | SWC, NvM, unit test, CI |
| M6 — Sleep + Docs | 2 tuần | 16–17 | EcuM, tài liệu hoàn chỉnh |

**Tổng: ~17 tuần (4 tháng)** nếu làm đều 10–15h/tuần.

> **Xong Milestone 4 là đã mạnh hơn phần lớn junior nộp CV vào Bosch / Marvell / FPT Automotive.**
> Đừng đợi làm xong hết mới đi xin việc.

---

## 5. Quy tắc làm việc

Phần này quan trọng ngang kỹ thuật.

- **Stuck quá 60 phút thì hỏi** — nhưng khi hỏi phải kèm: đã thử gì, quan sát được gì, hypothesis hiện tại là gì
- **Mọi thay đổi phải trả lời được:** *cái này ảnh hưởng tới function nào khác?* Trong automotive, một dòng sai trong lighting là recall
- **Không commit code chưa chạy trên target thật** ít nhất một lần
- **Estimate và báo sớm khi trễ.** Trễ không sao, im lặng rồi trễ mới là vấn đề
- **Một repo duy nhất, commit history sạch, tăng dần qua 4–5 tháng** — nó chứng minh bạn làm được dự án dài hơi, thứ automotive quan tâm hơn là biết bao nhiêu peripheral
- Commit message theo Conventional Commits: `feat(mcal): add Dio_WriteChannel per SWS_Dio`
- Mỗi milestone kết thúc bằng một tag Git: `v0.1-mcal`, `v0.2-com`, ...

---

## 6. Rủi ro & cách xử lý

| Rủi ro | Dấu hiệu | Cách xử lý |
|---|---|---|
| Sa lầy ở MCAL, cầu toàn quá mức | Tuần 6 vẫn đang sửa Dio | Timebox cứng 2 tuần, phần thiếu ghi vào backlog |
| ISO-TP khó, mất động lực | Debug 3 ngày không ra frame | Test từng frame type riêng bằng python-can trước khi ghép |
| Bootloader brick board | Board không boot | Luôn giữ ST-Link để recover, test trên board B trước |
| Làm xong không viết docs | README 5 dòng | Viết docs **trong lúc** làm, không để dồn cuối |
| Bỏ dở giữa chừng | Không commit 2 tuần | Chia task nhỏ ≤ 4h, commit mỗi ngày làm |

---

## 7. Backlog — làm sau nếu còn thời gian

- [ ] LIN master/slave qua USART LIN mode (mô phỏng door module)
- [ ] FreeRTOS thay cooperative scheduler, so sánh WCET hai bên
- [ ] Watchdog + stack overflow detection + hard fault handler in ra call stack
- [ ] MISRA C:2012 full compliance report, giải thích từng deviation
- [ ] Học lý thuyết ISO 26262: HARA, ASIL decomposition, FMEA — viết một tài liệu ngắn cho feature lighting
- [ ] Secure boot với chữ ký ECDSA
- [ ] SOME/IP hoặc DoIP nếu muốn hướng Adaptive AUTOSAR

---

## 8. Mốc đánh giá

**Sau 4 tháng, tự kiểm tra:**

- [ ] Nhận một yêu cầu cỡ trung bình → tự phân tích, tự implement, tự test, không cần ai dắt
- [ ] Giải thích được *tại sao* chọn kiến trúc này, không chỉ *nó làm gì*
- [ ] Debug được lỗi timing bằng logic analyzer, không đoán mò
- [ ] Trình bày dự án trong 10 phút cho người chưa biết gì về nó, họ hiểu được

Đạt 4/4 → sẵn sàng phỏng vấn vị trí automotive firmware.
