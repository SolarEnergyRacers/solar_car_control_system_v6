//
// CarControlx: Main control module of SER4
//
#include "../definitions.h"
#include <global_definitions.h>

#include <fmt/core.h>
#include <fmt/printf.h>
#include <iostream>
#include <stdio.h>
#include <string>

#include <Wire.h> // I2C

#include <CANBus.h>
#include <CarControl.h>
#include <CarState.h>
#include <CarStateRadio.h>
#include <Console.h>
#include <Helper.h>
#include <I2CBus.h>
#include <SDCard.h>
#include <SPIBus.h>

#include <System.h>

extern CANBus canBus;
extern CarControl carControl;
extern CarState carState;
extern CarStateRadio carStateRadio;
extern Console console;
extern I2CBus i2cBus;
extern SDCard sdCard;

extern bool SystemInited;

using namespace std;

unsigned long millisNextCanSend = millis();
unsigned long millisNextStampCsv = millis();
unsigned long millisNextStampSnd = millis();
unsigned long millisNextEngineerInfoCleanup = millis();

// ------------------
// FreeRTOS functions

string CarControl::re_init() { return init(); }

string CarControl::init() {
  bool hasError = false;
  justInited = true;
  carState.AccelerationDisplay = -99;
  return fmt::format("[{}] {} initialized.", hasError ? "--" : "ok", getName());
}

void CarControl::exit(void) {
  // TODO
}

string carStateEngineerInfoLast = "";
uint16_t carStateLifeSignLast = 0;
void CarControl::task(void *pvParams) {
  while (1) {
    report_task_stack(this);
    if (SystemInited) {

      bool force = false;
      if (millis() > millisNextCanSend || carStateLifeSignLast != carState.LifeSign) {
        millisNextCanSend = millis() + LIFESIGN_FREQUENCY_MS;
        force = true;
        carStateLifeSignLast = carState.LifeSign;
      }
      // vTaskDelay(10);
#ifndef SUPRESS_CAN_OUT_AC
      bool constantMode = carState.ConstantMode == CONSTANT_MODE::SPEED ? true : false;
      CANPacket packet = canBus.writePacket(AC_BASE0x00,
                                            (uint16_t)carState.LifeSign,      // LifeSign
                                            (uint8_t)(carState.Kp * 4),       // Kp
                                            (uint8_t)(carState.Ki * 10),      // Ki
                                            (uint8_t)(carState.Kd * 10),      // Kd
                                            (bool)constantMode,               // switch constant mode Speed / Power
                                            (bool)carState.ConfirmDriverInfo, // got confirm of driver about info
                                            (bool)force                       // force or not
      );
      carStateRadio.push_if_radio_packet(AC_BASE0x00, packet);
#endif
  CarStatePin *buttonNextScreenPin = carState.getPin(ESP32_AC_BUTTON_NEXT_SCREEN_GPIO27_name);
  CarStatePin *buttonConstModePin = carState.getPin(ESP32_AC_BUTTON_CONST_MODE_GPIO02_name);
  if (carControl.verboseModeCarControlDebug)
        console << fmt::format("[I:{:02d}|{:02d},O::{:02d}|{:02d}] CAN.PacketId=0x{:03x}-S-data:LifeSign={:4x}, buttonNextScreen = {:1x}, buttonConst = {:1x} ",
                               canBus.availablePacketsIn(), canBus.getMaxPacketsBufferInUsage(), canBus.availablePacketsOut(),
                               canBus.getMaxPacketsBufferOutUsage(), AC_BASE0x00, carState.LifeSign,
           buttonNextScreenPin != NULL ? buttonNextScreenPin->value : -1,
           buttonConstModePin != NULL ? buttonConstModePin->value : -1)
                << NL;
      // self destroying engineer info
      if (carState.EngineerInfo.compare(carStateEngineerInfoLast) != 0) {
        carStateEngineerInfoLast = carState.EngineerInfo;
        millisNextEngineerInfoCleanup = millis() + 7000;
      }
      if (millis() > millisNextEngineerInfoCleanup && carState.EngineerInfo.length() > 0) {
        carStateEngineerInfoLast = carState.EngineerInfo = "";
      }
      // log file one data row per LogInterval
      if ((millis() > millisNextStampCsv) || (millis() > millisNextStampSnd)) {
        if (verboseModeCarControl)
          console << carState.drive_data();
        string record = carState.csv("log");
        if (sdCard.isMounted() && millis() > millisNextStampCsv) {
          millisNextStampCsv = millis() + carState.LogInterval;
          if (sdCard.verboseModeSdCard)
            console << "SDCARD:: Interval=" << carState.LogInterval << ", Rec: " << record;
          sdCard.write_log(record);
        }
        // vTaskDelay(10);
        if (millis() > millisNextStampSnd) {
          carStateRadio.send();
          millisNextStampSnd = millis() + carState.CarDataSendPeriod;
        }
      }
    }
    taskSuspend();
  }
}
