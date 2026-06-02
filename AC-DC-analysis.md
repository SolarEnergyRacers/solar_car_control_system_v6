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
- **CHECK: **`write_rx_packet()` enters a critical section only around `CAN.endPacket()`; if `CAN.beginPacket()` is not atomic, this may be a small risk.
- `CmdHandler` can inject arbitrary packets into `canBus.pushIn(...)`, which is useful for debugging but may bypass normal bus validation.

### Paddle input → display flow

This describes the full path from paddle ADC reading on the DC side through to the driver display on the AC side:

- ADC sampling: `DC/lib/ADC/ADC_SER.cpp` reads the paddle ADC channels (`STW_ACC_PORT`, `STW_DEC_PORT`) and computes `acc` and `dec` after subtracting `carState.StartOffset_acc` / `carState.StartOffset_dec`.
- Normalization and damping: `DC/lib/CarControl/CarControlDC.cpp::read_paddles()` calls `normalize_0_UINT16(ads_min_acc, ads_max_acc, adc.stw_acc)` and the equivalent for deceleration, applying `carState.PaddleDamping` and `ads_min_*`/`ads_max_*` ranges.
- Display value calculation: `CarControlDC::calculate_acceleration_display()` maps the normalized ADC values into the display range using `transformArea(...)`, producing `carState.AccelerationDisplay`.
- DAC output & local actuation: `CarControlDC::set_DAC()` converts `AccelerationDisplay` into DAC setpoints (`valueDAC_acc` / `valueDAC_dec`) and writes them via `DC/lib/DAC/DAC.cpp` to the motor controller hardware.
- CAN publishing: `CarControlDC::task()` sends `DC_BASE_ADDR | 0x00` (raw ADC/driver inputs) and `DC_BASE_ADDR | 0x01` (target speed/power, `AccelerationDisplay`, `Speed`, flags) using `canBus.writePacket(...)`.
- AC receive and update: `lib_common/CAN/CANBusHandlerAC.cpp` handles `DC_BASE_ADDR | 0x01` and updates `carState.AccelerationDisplay` on the AC side.
- Driver screen refresh: `AC/lib/DriverDisplay/DriverDisplay.cpp` monitors `carState.AccelerationDisplay` and calls `write_acceleration()` when the value changes (or on first init), so the driver sees the new paddle value.

Key tuning points to reduce lag or increase responsiveness:

- ADC sampling interval (`adc.init_t(...)` in `DC/src/main.cpp`) — lower the sleep/polling delay to sample paddles more frequently.
- Normalization ranges (`ads_max_acc`, `ads_max_dec` in `DC/lib/CarControl/CarControl.h`) — reducing the max range increases sensitivity.
- Paddle damping (`PaddleDamping` in `DC/lib/CarState/CarState.h`) — lowering reduces the deadband around zero.
- CAN send forcing (`force` parameter in `CarControlDC::task()` calls to `canBus.writePacket`) — make sure updated `AccelerationDisplay` is transmitted immediately when it changes.

These steps together ensure paddle movement is sampled quickly on DC, converted into a stable display value, sent immediately over CAN, and shown promptly on the AC driver screen.

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

## CAN BUS Error Analysis

Inspecting CAN bus code and receive handling to identify crash root causes.

Searched for regex `CAN.onReceive|onReceive\(|pushIn\(|write_rx_packet|xQueueSendFromISR|isPacketToRenew|re_init|CAN.beginPacket|CAN.endPacket|CAN.setPins|CAN.begin` (`**/lib_common/CAN/*.{cpp,h}`), 21 results

Read [](file:///home/ksc/work/ser6/solar_car_control_system_v6/lib_common/CAN/CANBus.cpp#1-1), lines 1 to 260

Read [](file:///home/ksc/work/ser6/solar_car_control_system_v6/lib_common/CAN/CANBusHandlerDC.cpp)

Checking CAN queue and transmit handling for unsafe ISR or queue behavior that would crash with extra devices.

Read [](file:///home/ksc/work/ser6/solar_car_control_system_v6/lib_common/CAN/CANBus.cpp#260-260), lines 260 to 393

Read [](file:///home/ksc/work/ser6/solar_car_control_system_v6/lib_common/CAN/CANBus.h)

Read [](file:///home/ksc/work/ser6/solar_car_control_system_v6/lib_common/CAN/CANRxBuffer.cpp)

## Most likely cause

This is almost certainly a physical CAN-layer problem, not a bug in the paddle/display logic.

### Why it crashes when another device is plugged in
- The code uses a raw CAN transceiver interface (`CAN.setPins()`, `CAN.begin(CAN_SPEED)`).
- It assumes the bus is already healthy and does not handle severe CAN physical-layer faults.
- Adding a device changes the bus wiring, and if the bus is not correctly terminated or the new node is not powered/grounded properly, the CAN transceiver can get stuck or go bus-off.
- That can make the ESP32/CAN library fail in receive/send paths and appear as a crash or hang.

---

## What to check first

1. **Bus termination**
   - Valid CAN bus must have exactly two 120Ω terminators: one at each physical end.
   - If the cable plus new device adds a termination resistor in the middle, the bus impedance is wrong.

2. **Ground reference**
   - All CAN nodes must share a common ground.
   - A floating or missing ground on the new device can corrupt the differential bus.

3. **Cable type**
   - CAN needs twisted pair for RX/TX.
   - Using ordinary wires or long untwisted cable can cause reflections and bus errors.

4. **CAN speed and ID conflict**
   - The new device must use the same bitrate (`125 kb/s` in this project).
   - If it uses a duplicate CAN ID or floods the bus, the system can fail badly.

---

## Why the software may not recover

In CANBus.cpp:
- `CAN.onReceive(onReceive)` reads packets in ISR.
- `CANBus::task()` will reinitialize after repeated `counter*_notAvail > 8`.
- But there is no real bus-off recovery or error-status handling from the physical transceiver.
- If the bus is physically bad, the task may repeatedly reinit and eventually the system seems to crash.

---

## Specific code-related risk areas

- `CANBus::write_rx_packet()` holds `mutex_out` and calls `CAN.beginPacket()` / `CAN.endPacket()`.
- If the bus is corrupted by the new device, `CAN.endPacket()` may fail or block.
- `CANRxBuffer::push()` drops packets only when full, but does not protect against a hardware bus error.

---

## Most likely fix

### Hardware fix
- Ensure only two terminators on the whole CAN trunk.
- Make sure the new device is on the same ground and uses the same 125 kb/s.
- Use proper CAN twisted-pair cable and avoid long unterminated stubs.

### Software fix (if you want to harden it)
- Add bus error / bus-off detection in `CANBus::task()`.
- Add checks for `CAN.endPacket()` / send failure.
- Reduce dependence on a single transceiver and reinit more robustly.

---

## Bottom line

The crash is very likely caused by the new device changing the physical CAN bus conditions:
- wrong termination,
- missing ground,
- different bitrate,
- or noisy cabling.

The code is fragile to that condition, so fixing the wiring and termination is the first and best step.


## Exact software fixes

The crash is most likely because the CAN stack currently assumes the bus is healthy and does not properly detect or recover from physical errors. Here are the concrete fixes and where to apply them.

---

### 1. Harden the ISR receive path
File: CANBus.cpp

Current `onReceive()` is:

- doing `CAN.packetId()` and then `CAN.available()`
- not checking `packetSize`
- not handling cases where the bus is busy or malformed

Fix it by validating inputs first and dropping bad packets cleanly:

```cpp
void onReceive(int packetSize) {
  if (!SystemInited || packetSize <= 0 || packetSize > 8)
    return;

  if (!CAN.available()) return;

  uint16_t packetId = CAN.packetId();
  if (canBus.is_to_ignore_packet(packetId)) return;

  uint64_t rxData = 0;
  for (int i = 0; i < packetSize; i++) {
    int byte = CAN.read();
    if (byte < 0) return;
    rxData |= ((uint64_t)byte << (i * 8));
  }

  canBus.pushIn(CANPacket(packetId, rxData));
  canBus.counterI++;
  canBus.counterI_notAvail = 0;
  canBus.setPacketTimeStamp(packetId, millis());
}
```

That avoids undefined behavior when a bad cable or node corrupts the bus.

---

### 2. Check CAN transmit success
File: CANBus.cpp

Current `write_rx_packet()` writes bytes and then blindly calls `CAN.endPacket()`.

Change it to detect failure:

```cpp
void CANBus::write_rx_packet(CANPacket packet) {
  if (packet.getId() == 0) return;

  if (xSemaphoreTake(mutex_out, (TickType_t)32) != pdTRUE) {
    counterW_notAvail++;
    return;
  }

  bool ok = true;
  CAN.beginPacket(packet.getId());
  for (int i = 0; i < 8; ++i) {
    CAN.write(packet.getData_u8(i));
  }
  ok = CAN.endPacket();

  xSemaphoreGive(mutex_out);

  if (!ok) {
    counterW_notAvail++;
    if (counterW_notAvail > 4) {
      console << "CAN transmit failed, forcing reinit\n";
      canBus.re_init();
    }
  } else {
    counterW_notAvail = 0;
  }
}
```

This is important because a corrupted bus may cause `CAN.endPacket()` to fail and leave the stack in a bad state.

---

### 3. Add explicit bus-error recovery
File: CANBus.cpp

Add a bus error counter and recover aggressively:

```cpp
static int canBusErrorCount = 0;

void CANBus::task(void *pvParams) {
  while (1) {
    if (SystemInited) {
      if (rxBufferOut.isAvailable()) {
        write_rx_packet(rxBufferOut.pop());
      }

      if (counterW_notAvail > 8 || counterI_notAvail > 8 || counterR_notAvail > 8) {
        console << "CANBus REINIT trigger\n";
        canBus.re_init();
      }

      #ifdef CAN_STATUS_AVAILABLE
      uint32_t status = CAN.status(); // or CAN.error()
      if ((status & CAN_ERROR_BUSOFF) || (status & CAN_ERROR_PASSIVE)) {
        canBusErrorCount++;
        if (canBusErrorCount > 2) {
          console << "CAN bus error state, reinit\n";
          canBus.re_init();
          canBusErrorCount = 0;
        }
      } else {
        canBusErrorCount = 0;
      }
      #endif
    }
    taskSuspend();
  }
}
```

If your library exposes `CAN.status()` or `CAN.error()`, use that to detect bus-off/passive states explicitly. If not, rely on repeated transmit/receive failures.

---

### 4. Improve queue handling for ISR packets
File: CANRxBuffer.cpp

Your current code already discards the oldest packet when full, which is okay. But you should also track send failures:

```cpp
void CANRxBuffer::push(const CANPacket& packet) {
  CANPacket trashcan;
  if(xPortInIsrContext()) {
    if (!xQueueSendFromISR(queue, &packet, 0)) {
      xQueueReceiveFromISR(queue, &trashcan, 0);
      xQueueSendFromISR(queue, &packet, 0);
      // optionally increment a CAN bus overflow counter
    }
  } else {
    ...
  }
}
```

That avoids dropping silently and gives you a clear recovery signal.

---

### 5. Add a safe `re_init()` path
File: CANBus.cpp

Your `re_init()` currently just calls `CANBus::init()`. Make sure it also:

- resets counters
- closes the bus cleanly
- waits a small delay before restarting
- optionally toggles the transceiver power lines if available

Example:

```cpp
string CANBus::re_init() {
  CAN.end();
  vTaskDelay(50 / portTICK_PERIOD_MS);
  counterI_notAvail = counterR_notAvail = counterW_notAvail = 0;
  return init();
}
```

That reduces the chance of repeated immediate failures.

---

### 6. Ignore unknown/corrupted IDs instead of crashing
File: CANBus.cpp and CANBusHandlerDC.cpp

If a new device sends IDs not in your handled map, do not let the ISR or task crash. Keep `is_to_ignore_packet()` strict, but ensure it does not access invalid map entries.

Prefer:

```cpp
bool CANBus::is_to_ignore_packet(uint16_t packetId) {
  return packetId != (DC_BASE_ADDR | 0x00) &&
         packetId != (DC_BASE_ADDR | 0x01) &&
         max_ages.find(packetId) == max_ages.end();
}
```

That is safer than indexing `max_ages[packetId]` for unknown addresses.

---

## In short

The exact software changes are:

1. validate `packetSize` and `CAN.available()` in `onReceive()`
2. check `CAN.endPacket()` return value and count failures
3. add a bus-error counter in `CANBus::task()`
4. reinitialize CAN cleanly on repeated errors
5. make `is_to_ignore_packet()` safe for unknown IDs
6. optionally use `CAN.status()` / `CAN.error()` if the library supports it

These are the exact software hardening steps that will make the system recover from a bad cable/node instead of crashing.