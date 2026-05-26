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
  // reset_and_lock_pot();
  dacInited = reset_pot();
  return fmt::format("[{}] DAC re-inited.", dacInited ? "ok" : "--");
}

string DAC::init() {
  console << "[  ] Init 'DAC'...\n";
  dacInited = reset_pot();
  console << fmt::format("     DAC initialisation {}.\n", dacInited ? "successful" : "failed");
  return fmt::format("[{}] DAC initialized.", dacInited ? "ok" : "--");
}

void DAC::reset_and_lock_pot() {
  lock_acceleration();
  dacInited = reset_pot();
}

void DAC::lock_acceleration() {
  // #SAFETY#: acceleration lock
  isLocked = true;
  carState.AccelerationLocked = true;
};

bool DAC::reset_pot() {
  bool success_ACCL = true;
  bool success_DECCL = true;
  MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

  // #SAFTY#
  uint32_t retValue = ledcSetup(PWMChannelACCL, PWMFreq, PWMResolution);

  if (!retValue) {
    console << "PWM initialization failed!" << NL;
    return false;
  }
  /* Attach the LED PWM Channel to the GPIO Pin */
  ledcAttachPin(PinACCL, PWMChannelACCL);
   ledcAttachPin(PinDECCL, PWMChannelDECL);

  console << "PWM inited [" << retValue << "] ACCEL at GPIO " << PO_AccelPWM_GPIO16_name << "(" << PO_AccelPWM_GPIO16 << ")"
          << "DECCEL at GPIO " << PO_DeccelPWM_GPIO02_name << "(" << PO_DeccelPWM_GPIO02 << ")" << NL;

  return success_ACCL && success_DECCL;
}

bool DAC::set_pot(uint8_t val, pot_chan channel) {
  if (!dacInited)
    return false;
  bool success = true;
  // #SAFETY#: acceleration lock
  if (isLocked) {
    if (carState.AccelerationDisplay != 0)
      return false;

    // release unlock state and take over into to car state
    unlock_acceleration();
    carState.AccelerationLocked = false;
    string s = "DAC unlocked.\n";
    console << s;
  }
  uint8_t oldValue = get_pot(channel);
  if (oldValue == val) {
    // if (verboseModeDAC) {
    //   console << fmt::format("dac:    {:02x}-chn: val:{:5d} -->No Change", channel, val) << NL;
    // }
    return false;
  }
  
  int dutyCycle;
  try {
    dutyCycle = (float)val / DAC_MAX * MAX_DUTY_CYCLE;
    if (channel == POT_CHAN0_ACC) {
      pot0 = val;
      ledcWrite(PWMChannelACCL, dutyCycle);
    }
    if(channel == POT_CHAN1_DEC) {
      pot1 = val;
      ledcWrite(PWMChannelDECL, dutyCycle);
    }
  } catch (exception &ex) {
    success = false;
  }
  if (verboseModeDAC) {
    console << fmt::format("dac:    {:02x}-chn: val:{:5d} --> {:5d} acce:{:5d} | decc:{:5d}  | display:{:5d} | AccelerationLocked={} (DOAX_MAX={}, MAX_DUTY_CYCLE={})\n", 
      channel, val, dutyCycle, carState.Acceleration, carState.Deceleration, carState.AccelerationDisplay, carState.AccelerationLocked, DAC_MAX, MAX_DUTY_CYCLE);
  }

  return success;
}

uint16_t DAC::get_pot(pot_chan channel) {
  if (!dacInited)
    return 0;
  if (channel == POT_CHAN_ALL) {
    return pot0 | (pot1 << 8);
  } else if (channel == POT_CHAN0_ACC) {
    return pot0;
  } else { // POT_CHAN1_DEC
    return pot1;
  }
}
