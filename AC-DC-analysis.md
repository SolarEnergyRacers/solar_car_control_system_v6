# AC/DC Control System Analysis

## Overview

This repository contains two ESP32-based control applications:

- `AC/` — Auxiliary Controller (AC)
- `DC/` — Drive Controller (DC)

Both use FreeRTOS and a shared CAN bus implementation in `lib_common/CAN/`.
The two controllers communicate with each other and with additional devices using CAN IDs defined in `lib_common/include/global_definitions.h`.

## FreeRTOS Task Structure

### AC Controller (`AC/src/main.cpp`)

The AC application creates the following main tasks:

- `CANBus` (`canBusTask`) — priority 22, stack 10000, pinned to core 0, polling interval 20ms
- `CmdHandler` (`cmdHandlerTask`) — priority 1, stack 10000, pinned to core 1, polling interval 310ms
- `EngineerDisplay` (`engineerDisplayTask`) — priority 1, stack 10000, pinned to core 1, polling interval 30ms
- `DriverDisplay` (`driverDisplayTask`) — priority 1, stack 10000, pinned to core 1, polling interval 30ms
- `CarControl` (`carControlTask`) — priority 10, stack 10000, pinned to core 1, polling interval 20ms

Notes:
- Each task is created with `xTaskCreatePinnedToCore(...)`.
- `AbstractTask::init_t()` configures core, priority, stack, and sleep polling.
- Tasks use `taskSuspend()` to perform `vTaskDelay()` for periodic polling.

### DC Controller (`DC/src/main.cpp`)

The DC application creates these main tasks:

- `CANBus` (`canBusTask`) — priority 20, stack 10000, pinned to core 0, polling interval 20ms
- `CmdHandler` (`cmdHandlerTask`) — priority 1, stack 10000, pinned to core 1, polling interval 310ms (conditional via `COMMANDHANDLER_ON`)
- `CarControl` (`carControlTask`) — priority 25, stack 10000, pinned to core 1, polling interval 20ms
- `IOExt` (`ioExtTask`) — priority 10, stack 10000, pinned to core 1, polling interval 100ms
- `ADC` (`adcTask`) — priority 20, stack 10000, pinned to core 1, polling interval 100ms
- `ConstSpeed` (`constSpeedTask`) — priority 15, stack 10000, pinned to core 1, polling interval 160ms

Notes:
- DC gives the highest priority to `CarControl`, reflecting its real-time drive command processing role.
- The CAN task remains high priority but slightly lower than the DC car control task.

## CAN Bus Architecture

### Core CAN implementation

Shared CAN code lives under `lib_common/CAN/`:

- `CANBus.h` — task wrapper, packet queues, write helpers, verbosity flags
- `CANBus.cpp` — CAN initialization, ISR-based receive callback, task execution loop
- `CANPacket.h` — packet abstraction for 8-byte data payloads
- `CANRxBuffer.*` — static FreeRTOS queue with overwrite-on-full behavior
- `CANBusHandlerAC.cpp` / `CANBusHandlerDC.cpp` — mode-specific receive handling

### CAN initialization

- CAN pins are configured in `CANBus::init()` with `CAN.setPins(CAN_RX, CAN_TX)`.
- Bus speed is `CAN_SPEED` defined in `global_definitions.h` as `125E3` (125 kb/s).
- `CAN.onReceive(onReceive)` registers the receive ISR callback.

### Interrupt path and buffering

- `onReceive(int packetSize)` is called from CAN ISR context.
- It reads the incoming ID with `CAN.packetId()` and the payload bytes with `CAN.read()`.
- The ISR pushes a `CANPacket` into `canBus.pushIn(...)`, which stores packets in `rxBufferIn`.
- `CANRxBuffer::push()` uses `xQueueSendFromISR()` and drops the oldest packet if the queue is full.

### CAN task loop

- `CANBus::task()` runs continuously once `SystemInited` is true.
- It processes inbound packets from `rxBufferIn` using `handle_rx_packet(...)`.
- It processes outbound packets from `rxBufferOut` by calling `write_rx_packet(...)`.
- The task also has a recovery path: if repeated queue availability errors occur, it calls `canBus.re_init()`.

### Outbound packet de-duplication

- `CANBus::writePacket(...)` stores the last packet for each address in `packetsLast`.
- A new packet is only pushed to `rxBufferOut` if the data differs or `force` is true.
- This reduces redundant CAN traffic.

## Message Mapping and Controller Communication

### Addressing scheme

From `global_definitions.h`:

- `AC_BASE_ADDR = 0x630`
- `DC_BASE_ADDR = 0x660`
- `MC_BASE_ADDR = 0x500`
- `MPPT1_BASE_ADDR = 0x600`
- `MPPT2_BASE_ADDR = 0x610`
- `MPPT3_BASE_ADDR = 0x620`
- `BMS_BASE_ADDR = 0x700`

### AC → DC / CAN output

AC sends packets to the CAN bus via `CarControlAC::task()`:

- `AC_BASE0x00` (0x630) carries:
  - LifeSign
  - PID parameters `Kp`, `Ki`, `Kd`
  - constant mode selector (speed/power)
  - driver confirmation flag

This packet is also optionally copied into radio packet cache via `CarStateRadio::push_if_radio_packet(...)`.

### DC → AC / CAN output

DC sends two packet types in `CarControlDC::task()`:

- `DC_BASE_ADDR | 0x00` (0x660) carries:
  - LifeSign
  - potentiometer value
  - acceleration ADC value
  - deceleration ADC value

- `DC_BASE_ADDR | 0x01` (0x661) carries:
  - target speed
  - target power
  - display acceleration
  - display speed
  - forward/backward direction
  - brake pedal state
  - motor on/off
  - constant mode on/off
  - confirm-driver-info flag

### AC CAN receive handling (`CANBusHandlerAC.cpp`)

- Incoming `DC_BASE_ADDR | 0x00` updates AC `carState` with live driver inputs.
- Incoming `DC_BASE_ADDR | 0x01` updates AC `carState` with target speed/power and drive state.
- AC also parses selected BMS, MPPT, MC, and other packets for battery and PV telemetry.
- A normalization map rewrites some MC addresses before handling.

The AC mode ignores packets unless they are DC packets or packets allowed by `isPacketToRenew(...)`.

### DC CAN receive handling (`CANBusHandlerDC.cpp`)

- Incoming `AC_BASE0x00` updates DC `carState` with PID tuning, constant-mode selection, and driver confirmation.
- DC also parses selected BMS and MPPT telemetry packets, similar to AC.
- DC ignores non-critical bus traffic unless the packet is due for renewal based on `max_ages`.

## CAN packet lifetime and renewal

The system keeps max-age limits in `CANBus::init_ages()`.

- Many packets are configured with `0` age, meaning they may be kept fresh on every update.
- Some packets use finite ages such as:
  - `MAXAGE_BALANCE_SOC = 5000`
  - `MAXAGE_PACK_VOLTAGE = 500`
  - `MAXAGE_MPPT_TEMP = 5000`
  - `MAXAGE_MPPT_POWER_CONN = 5000`

`CANBus::isPacketToRenew(packetId)` returns `true` when a packet is due for a refresh or when `max_age == 0`.
This is used to allow repeated reception of important periodic telemetry.

## Observations and Checks

### Overall design

- The two controllers share a common CAN stack and packet abstraction.
- AC is oriented toward display, operator interaction, and radio forwarding.
- DC is oriented toward sensor acquisition, motor control, and closed-loop speed/power commands.
- Both controllers create their main tasks explicitly and bind them to CPU cores.

### CAN bus design strengths

- ISR-based receive with queue buffering isolates CAN reception from application code.
- Packet de-duplication reduces bus load.
- Mode-specific handlers keep AC/DC receive behavior separate.
- CAN speed is set consistently to 125 kb/s.

### Potential concerns

- The CAN task uses `taskSuspend()` for its loop, so the actual polling interval depends on `sleep_polling` and system timing.
- Packet delivery from ISR to task uses a fixed-size queue with overwrite behavior; this is acceptable but means old packets can be discarded under load.
- `write_rx_packet()` enters a critical section only around `CAN.endPacket()`; if `CAN.beginPacket()` is not atomic, this may be a small risk.
- `CmdHandler` can inject arbitrary packets into `canBus.pushIn(...)`, which is useful for debugging but may bypass normal bus validation.

## Summary

The AC/DC control system is organized around a shared CAN communication layer and a set of FreeRTOS tasks for core modules. The AC controller prioritizes display, command handling, and CAN reception, while the DC controller prioritizes drive control, ADC sampling, and output updates.

The CAN bus path is:

1. `CAN.onReceive(...)` captures packets in ISR
2. `canBus.pushIn(...)` enqueues them in `rxBufferIn`
3. `CANBus::task()` dequeues and dispatches them with `handle_rx_packet(...)`
4. Application logic updates `carState` and shared state
5. Outbound packets are built using `CANBus::writePacket(...)`
6. `CANBus::task()` sends outbound packets from `rxBufferOut`

This report should serve as a basis for further tuning of task priorities, polling intervals, and CAN packet handling.

## Speed and Button Reaction Improvement Options

### Primary latency sources

- `DC/src/main.cpp` creates `IOExt` with `init_t(1, 10, 10000, base_offset_suspend + 90)`, which means `IOExt::task()` polls pins every ~100ms.
- `IOExt::task()` calls `readAndHandlePins()` once per cycle, so a button press can wait up to one polling interval before being seen.
- `CarControlDC` then reacts in its own loop, which is also driven by `taskSuspend()` delays.
- In `DC/lib/IOExt/IOExt.cpp`, `CarStatePin` entries use `debounceTime_ms = 200`, adding another delay window before a press is accepted.

### Recommended changes

1. Reduce `IOExt` polling interval
   - Change `base_offset_suspend + 90` to a lower value such as `base_offset_suspend + 10` or `20` in `DC/src/main.cpp`.
   - This reduces worst-case button latency from ~100ms to ~10-20ms.
   - Example: `ioExt.init_t(1, 10, 10000, base_offset_suspend + 20);`

2. Lower software debounce times for direct buttons
   - DC uses 200ms debounce for `DI_ButtonPlus`, `DI_ButtonMinus`, `DI_ButtonSet`, and `DI_Button_Confirm`.
   - If hardware is clean, reduce this to 20-50ms in `DC/lib/IOExt/IOExtPins.cpp`.
   - This improves responsiveness while still filtering bounce.

3. Use interrupt-driven input where possible
   - Currently `IOExt` polls all pins in `IOExt::task()` instead of using GPIO interrupts.
   - For ESP32 GPIO inputs, use `attachInterrupt(digitalPinToInterrupt(pin), handler, CHANGE/RISING/FALLING)` or `gpio_isr_handler_add(...)`.
   - For MCP23017 if used in the future, enable the INT pin and handle it via an ESP32 GPIO interrupt rather than polling.

4. Raise `IOExt` task priority if button latency is critical
   - `IOExt` is currently priority 10 while `CarControl` is 25 and `ADC` is 20.
   - If input handling must preempt slower background tasks, consider raising `IOExt` to 15 or higher.
   - Keep `CarControl` higher if closed-loop control remains the top real-time requirement.

5. Keep AC direct-button polling faster if needed
   - AC `CarControl` already polls direct buttons every 20ms via `taskSuspend()`.
   - If extra speed is needed, reduce AC `CarControl` sleep from `base_offset_suspend + 10` to `10` or `5`.
   - Also consider reducing button debounce values in `AC/lib/CarControl/CarControl.h` from `500`ms to `50-100`ms if hardware supports it.

### Good first patch

- In `DC/src/main.cpp`, set IOExt polling to `base_offset_suspend + 20`.
- In `DC/lib/IOExt/IOExtPins.cpp`, lower `debounceTime_ms` for buttons to `50`.
- Optionally add a comment to `DC/lib/IOExt/IOExt.cpp` noting that `attachInterrupt` would be preferable to polling.

### What to watch for

- Reducing debounce below 50ms may expose mechanical bounce on noisy buttons.
- Faster polling increases CPU wakeups, but the ESP32 can handle 10-20ms for a few simple tasks.
- If the system is I/O-bound, prioritize input-handling tasks over non-critical loops.
