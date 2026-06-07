# AC/DC Control System Reliability Analysis

## Scope

This document focuses on CAN communication reliability between AC and DC controllers in SER6.

Primary requirement:

- AC<->DC control exchange is critical and should not be silently lost.

Secondary requirement:

- Other CAN participants (BMS, MPPT, and additional telemetry) are informational. Packet loss is acceptable because data is sent repeatedly.

## Baseline

Main code locations:

- AC application: `AC/`
- DC application: `DC/`
- Shared CAN stack: `lib_common/CAN/`
- CAN constants: `lib_common/include/global_definitions.h`

Current CAN library in both AC and DC builds:

- `https://github.com/sandeepmistry/arduino-CAN`

Current bitrate:

- `CAN_SPEED = 125E3`

CAN base IDs in code:

- `AC_BASE_ADDR = 0x650`
- `DC_BASE_ADDR = 0x660`
- `BMS_BASE_ADDR = 0x700`
- `MPPT1/2/3_BASE_ADDR = 0x600/0x610/0x620`

## Reliability Classes

Class A (critical, must-not-lose behavior):

- `0x650` (`AC_BASE_ADDR | 0x00`) AC -> DC command/control frame
- `0x660` (`DC_BASE_ADDR | 0x00`) DC -> AC driver input frame
- `0x661` (`DC_BASE_ADDR | 0x01`) DC -> AC target/state frame

Class B (telemetry, loss tolerated):

- BMS frames
- MPPT frames
- Other repeated informational traffic

## Current Critical Data Flow

TX generation:

- AC writes `0x650` in `AC/lib/CarControl/CarControlAC.cpp`.
- DC writes `0x660` and `0x661` in `DC/lib/CarControl/CarControlDC.cpp`.

Transport:

- ISR receive callback in `lib_common/CAN/CANBus.cpp`.
- RX/TX queue handling in `lib_common/CAN/CANRxBuffer.cpp` and `lib_common/CAN/CANBus.cpp`.

RX consumption:

- AC parses `0x660` and `0x661` in `lib_common/CAN/CANBusHandlerAC.cpp`.
- DC parses `0x650` in `lib_common/CAN/CANBusHandlerDC.cpp`.

## Confirmed Findings

### Critical

1. RX queue overwrite can drop critical packets.

- File: `lib_common/CAN/CANRxBuffer.cpp`
- Behavior: oldest packet is removed when queue is full.
- Impact: class A and class B traffic are not separated, so critical frames can be dropped under pressure.

### High

1. TX failure path has no bounded retry policy for critical messages.

- File: `lib_common/CAN/CANBus.cpp`
- Behavior: semaphore timeout or `CAN.endPacket()` failure increments counters and returns.
- Impact: critical packet can be lost without immediate recovery.

1. Dedup logic can keep stale control state after a lost frame.

- File: `lib_common/CAN/CANBus.cpp`
- Behavior: unchanged payload is not re-enqueued unless `force` is true.
- Impact: if a critical frame is lost and value does not change, receiver can stay stale longer.

### Medium

1. Fixed TX service budget can increase queue latency.

- File: `lib_common/CAN/CANBus.cpp`
- Behavior: `MAX_TX_PACKETS_PER_CYCLE = 4`.
- Impact: bursts can delay control updates.

1. Priority handling exists but does not provide end-to-end QoS.

- File: `lib_common/CAN/CANBus.cpp`
- Behavior: critical IDs are prioritized in parts of TX scheduling.
- Impact: no complete guarantee model (critical reservation + retry + stale contract).

## Implementation Started

Implemented now in shared CAN code:

- Added queue overwrite visibility in `CANRxBuffer::push(...)` (returns whether overwrite happened).
- Added counters in `CANBus`:
  - `counterRxOverwrite`
  - `counterTxOverwrite`
  - `counterCriticalTxDrop`
  - `counterCriticalTxFail`
- Added critical TX failure counting for semaphore and `endPacket()` failures.
- Added bounded retry/requeue for critical TX IDs (`0x650`, `0x660`, `0x661`).
- Added critical frame staleness watchdog (`CRITICAL_CAN_STALE_TIMEOUT_MS`) with stale/recovered logs and counter.
- Migrated CAN backend implementation in `CANBus` from arduino-CAN to ESP-IDF TWAI.
- Added critical sequence-gap detection for IDs `0x650`, `0x660`, and `0x661`.
- Added dedicated critical TX queue lane so telemetry queue pressure cannot evict critical frames.
- Extended reinit log output to include new reliability counters.

Changed files:

- `lib_common/CAN/CANRxBuffer.h`
- `lib_common/CAN/CANRxBuffer.cpp`
- `lib_common/CAN/CANBus.h`
- `lib_common/CAN/CANBus.cpp`
- `lib_common/include/global_definitions.h`
- `AC/platformio.ini`
- `DC/platformio.ini`

## Library Assessment

### Previous library: arduino-CAN

Short-term suitability:

- Good integration and low change risk.
- Simple API and already in production code path.

Limitations against class A reliability target:

- Reliability/QoS policy must be implemented mostly in project code.
- Less explicit application-facing control for advanced error-state handling than native ESP-IDF TWAI APIs.

### Selected target: ESP-IDF TWAI backend

Decision implemented:

1. CAN transport in `CANBus` now uses ESP-IDF TWAI APIs.
1. Build dependencies for arduino-CAN were removed from AC/DC platformio config.

Rationale:

- Better error/state visibility.
- Better recovery integration.
- Better long-term fit for must-not-lose control path requirements.

## Next Implementation Steps

1. Add critical staleness timeout policy for `0x650`, `0x660`, and `0x661`.
1. Add optional backend abstraction layer only if dual-backend support is needed.

## Validation Gates

1. No undetected stale class A state beyond defined timeout budget.
2. Class A drop/failure counters visible during stress tests.
3. Injected TX failures trigger deterministic retry/recovery behavior.
4. Class B packet loss does not impact control correctness.
