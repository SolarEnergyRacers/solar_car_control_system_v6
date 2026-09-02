//
// Digital to Analog Converter
//

#include "../definitions.h"

#include <fmt/core.h>
#include <fmt/printf.h>
#include <inttypes.h>
#include <iostream>
#include <stdio.h>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <Arduino.h>
#include <Wire.h> // I2C

#include <CarState.h>
#include <Console.h>
#include <DAC.h>
#include <Helper.h>
#include <I2CBus.h>

#define BASE_ADDR_CMD 0xA8

extern CarState carState;
extern I2CBus i2cBus;
extern Console console;
extern bool dacInited;

string DAC::re_init() {
  dacInited = reset_pot();
  return fmt::format("[{}] DAC re-inited.", dacInited ? "ok" : "--");
}

string DAC::init() {
  console << "[  ] Init 'DAC'...\n";
  dacInited = reset_pot();
  console << fmt::format("     DAC initialisation {}.\n", dacInited ? "successful" : "failed");
  return fmt::format("[{}] DAC initialized.", dacInited ? "ok" : "--");
}

bool DAC::reset_pot() {
  bool success_Backlight = true;
  MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

  uint32_t retValue = ledcSetup(PWMChannelBackLight, PWMFreq, PWMResolution);

  if (!retValue) {
    console << "PWM initialization failed!" << NL;
    return false;
  }
  /* Attach the LED PWM Channel to the GPIO Pin */
  ledcAttachPin(PinBacklight, PWMChannelBackLight);

  set_pot(carState.Backlight);

  console << "PWM inited [" << retValue << "] BACKLIGHT at GPIO " << ESP32_AC_BACKLIGHT_PWM_GPIO33_name << "("
          << ESP32_AC_BACKLIGHT_PWM_GPIO33 << ") to initial value=" << carState.Backlight << NL;

  return success_Backlight;
}

bool DAC::set_pot(uint8_t val) {
  if (!dacInited)
    return false;
  bool success = true;
  uint8_t oldValue = get_pot();
  if (oldValue == val) {
    if (verboseModeDAC) {
      console << fmt::format("dac:    {:02x}-chn: val:{:5d} --> No Change", POT_CHAN0_BACKLIGHT, val) << NL;
    }
    return false;
  }

  int dutyCycle;
  try {
    dutyCycle = (float)val / DAC_MAX * MAX_DUTY_CYCLE;
    pot0 = val;
    ledcWrite(PWMChannelBackLight, dutyCycle);
  } catch (exception &ex) {
    success = false;
  }
  if (verboseModeDAC) {
    console << fmt::format("dac:    {:02x}-chn: val:{:5d} --> {:5d} | display backlight:{:5d} (DOAX_MAX={}, MAX_DUTY_CYCLE={})\n",
                           POT_CHAN0_BACKLIGHT, val, dutyCycle, carState.Backlight, DAC_MAX, MAX_DUTY_CYCLE);
  }

  return success;
}

uint16_t DAC::get_pot() {
  if (!dacInited)
    return 0;
  return pot0;
}
