# Third-party: CMSIS

This directory contains unmodified third-party headers.
**Do not edit files here.** To update, follow the procedure below.

## Components

### CMSIS-Core

| Field       | Value |
|-------------|-------|
| Supplier    | Arm Ltd. |
| Upstream    | https://github.com/ARM-software/CMSIS_6 |
| Version     | v6.3.1-dev-32-g26206e47 |
| Commit      | 26206e47dcf0abfbdc64eb753a0b6334b24439f6 |
| Commit date | 2026-09-11 |
| Imported on | 2026-09-21 |
| License     | <xem file LICENSE trong repo gốc> |
| Source path | `CMSIS/Core/Include/` |
| Local path  | `external/cmsis/core/` |
| Modified    | No |

### STM32F1 device headers

| Field       | Value |
|-------------|-------|
| Supplier    | STMicroelectronics |
| Upstream    | https://github.com/STMicroelectronics/cmsis_device_f1 |
| Version     | v4.3.5-1-gc8e9a4a |
| Commit      | c8e9a4a4f16b6d2cb2a2083cbe5161025280fb22 |
| Commit date | 2025-03-21 |
| Imported on | 2026-09-21 |
| License     | <xem file LICENSE trong repo gốc> |
| Source path | `Include/` |
| Local path  | `external/cmsis/device/` |
| Modified    | No |
| Not imported| `Source/` (startup and system templates — written in-house per BCM-001) |

## Usage

- Device selection macro: `STM32F103xB` (defined in CMakeLists.txt)
- Entry header: `#include "stm32f1xx.h"`

## Update procedure

1. Clone upstream, check out the new release tag.
2. Replace the local directory entirely (do not merge by hand).
3. Update the tables above: version, commit, dates.
4. Build and run all tests.
5. Commit as a single change: `chore(external): update CMSIS-Core to vX.Y.Z`