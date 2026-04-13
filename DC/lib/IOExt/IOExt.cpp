//
// MCP23017 I/O Extension over I2C
//
#include "../definitions.h"

#include <stdio.h>

// standard libraries
#include <fmt/core.h>
#include <inttypes.h>
#include <iostream>
#include <stdio.h>
#include <string>

#include <Wire.h>     // I2C

#include <CarControl.h>
#include <Console.h>
#include <Helper.h>
#include <I2CBus.h>
#include <IOExt.h>
#include <IOExtHandler.h>

extern Console console;
extern I2CBus i2cBus;
extern IOExt ioExt;
extern CarState carState;
extern CarControl carControl;
extern bool SystemInited;

// ------------------
// FreeRTOS functions

volatile bool ioInterruptRequest;
void IRAM_ATTR ioExt_interrupt_handler() { ioInterruptRequest = true; };

string IOExt::re_init() { return init(); }

string IOExt::init() {
  bool hasError = false;
  int defined_devices_count = sizeof(CarState::pins) / sizeof(CarState::pins[0]);
  console << "[  ] Init IOExt "<< defined_devices_count<<" devices...\n";

  try {
    for (int pinNr = 0; pinNr < defined_devices_count; pinNr++) {
      CarStatePin *pin = carState.getPin(pinNr);
      console << "Setup '" << pin->name <<"', mode:"<< pin->mode << ", continuousMode:" << pin->continuousMode << ", debounceTime_ms:" << pin->debounceTime_ms << NL;
      carState.idxOfPin.insert(pair<string, int>{pin->name, pinNr});
      pinMode(pin->gpio, pin->mode);
    }
    console << "     ok " << getName() << NL;
    // inital read the io pins
    readAllPins();
  }
  catch (exception &ex) {
    hasError = true;
    console << "ERROR: Couldn not init GPIOs, ex: " << ex.what() << NL;
  }
  // setup inerrupt handling
  ioInterruptRequest = false;
  isInInputHandler = false;
  //pinMode(I2C_INTERRUPT, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(I2C_INTERRUPT), ioExt_interrupt_handler, CHANGE);
  return fmt::format("[{}] IOExt initialized.", hasError ? "--" : "ok");
}

void IOExt::exit(void) {
  // TODO
}
// ------------------

void IOExt::writeAllPins(PinHandleMode mode) {
  for (auto &pin : carState.pins) {
    if (pin.mode == OUTPUT && (pin.oldValue != pin.value || !pin.inited || mode == PinHandleMode::FORCED)) {
      digitalWrite(pin.gpio, pin.value);
      pin.oldValue = pin.value;
      pin.inited = true;
    }
  }
}

void IOExt::readAllPins() {
  for (CarStatePin &pin : carState.pins) {
    if (pin.mode != OUTPUT) {
        pin.value = digitalRead(pin.gpio);
    }
  }
  if (verboseModeDIn) {
    console << fmt::format("IOExt ({}ms)", millis())
            << ", Car state: " << carState.printIOs("", true, false) << "\n";
  }
}

bool IOExt::readAndHandlePins(PinHandleMode mode) {
  if (isInInputHandler)
    return false;
  isInInputHandler = true;
  bool hasChanges = false;
  readAllPins();
  list<void (*)()> pinHandlerList;
  for (CarStatePin &pin : carState.pins) {
    if (pin.mode != OUTPUT && (pin.handlerFunction != NULL)) {
      unsigned long timestamp = millis();
      // button: debounced time and value == 0 --> activate Handler
      // toggles: debounced time == 0, value 0|1 --> activate handler
      if ((pin.value == 0 || (pin.debounceTime_ms == 0 && pin.oldValue != pin.value)) &&
          (timestamp > pin.timestamp + pin.debounceTime_ms) &&
          (pin.continuousMode || pin.oldValue != pin.value || !pin.inited || mode == PinHandleMode::FORCED)) {
        if (ioExt.verboseModeDIn)
          console << fmt::format("Get BOOL -- 0x{:02x}: {} --> {}, inited: {}  <-- {:18s} \t({})\t -> handle ({}ms)\n", pin.gpio,
                                 pin.oldValue, pin.value, pin.inited, pin.name, millis(), timestamp - pin.timestamp);
        pinHandlerList.push_back(pin.handlerFunction);
        pin.inited = true;
        if (pin.oldValue != pin.value)
          hasChanges = true;
        pin.oldValue = pin.value;
        pin.timestamp = timestamp;
      }
      if (pin.value == 1)
        pin.oldValue = 1;
    }
  }
  // avoid multi registration:
  pinHandlerList.unique();
  // call all handlers for changed pins
  if (SystemInited) {
    // xSemaphoreTakeT(i2cBus.mutex);
    for (void (*pinHandler)() : pinHandlerList) {
      pinHandler();
    }
    // xSemaphoreGive(i2cBus.mutex);
  }
  pinHandlerList.clear();
  isInInputHandler = false;
  return hasChanges;
}

int IOExt::getPort(int gpio) {
  int value = digitalRead(gpio);
  // console << fmt::format("port 0x{:02x} [{}|{}]: {} -- {}ms\n", port, devNr, pin, value, millis());
  return value;
}

void IOExt::task(void *pvParams) {
  while (1) {
    if (SystemInited) {
      readAndHandlePins(); // PinHandleMode::FORCED);
    }
    taskSuspend();
  }
}
