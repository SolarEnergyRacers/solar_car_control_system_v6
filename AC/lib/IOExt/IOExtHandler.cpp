//
// MCP23017 I/O Extension over I2C
//
#include "../definitions.h"

#include <stdio.h>

// standard libraries
#include <string>

#include <Console.h>
#include <Helper.h>
#include <IOExt.h>
#include <IOExtHandler.h>
#include <SDCard.h>
#include <SPIBus.h>

extern Console console;
extern IOExt ioExt;
extern CarState carState;
extern SDCard sdCard;
extern SPIBus spiBus;
extern bool SystemInited;

void nextScreenButtonHandler() {
	if (!SystemInited)
		return;

	CarStatePin *pin = carState.getPin(ESP32_AC_BUTTON_NEXT_SCREEN_GPIO27_name);
	if (pin == NULL || pin->value == 0)
		return;

	static unsigned long nextScreenButton_lastPress = 0;
	static const unsigned long nextScreenButton_debounceTime_ms = 500;
	unsigned long timestamp = millis();
	if (timestamp < nextScreenButton_lastPress + nextScreenButton_debounceTime_ms)
		return;
	nextScreenButton_lastPress = timestamp;

	switch (carState.displayStatus) {
	case DISPLAY_STATUS::ENGINEER_RUNNING:
		carState.displayStatus = DISPLAY_STATUS::DRIVER_SETUP;
		console << "Switch Next Screen toggle: switch from engineer --> driver" << NL;
		break;
	case DISPLAY_STATUS::DRIVER_RUNNING:
		carState.displayStatus = DISPLAY_STATUS::ENGINEER_SETUP;
		console << "Switch Next Screen toggle: switch from driver --> engineer" << NL;
		break;
	default:
		break;
	}
}

void constModeOrMountRequestHandler() {
	if (!SystemInited)
		return;

	CarStatePin *pin = carState.getPin(ESP32_AC_BUTTON_CONST_MODE_GPIO02_name);
	if (pin == NULL || pin->value == 0)
		return;

	static unsigned long mountrequest_lastPress = 0;
	static const unsigned long mountrequest_debounceTime_ms = 500;
	unsigned long timestamp = millis();
	if (timestamp < mountrequest_lastPress + mountrequest_debounceTime_ms)
		return;
	mountrequest_lastPress = timestamp;

	switch (carState.displayStatus) {
	case DISPLAY_STATUS::ENGINEER_RUNNING:
		if (sdCard.isMounted()) {
			sdCard.unmount();
			vTaskDelay(1000);
		} else {
			if (sdCard.mount()) {
				carState.EngineerInfo = "SD card mounted.";
				console << "     " << carState.EngineerInfo << NL;
				vTaskDelay(1000);
				string state = carState.csv("Recent State just after mounting", true); // with header
				sdCard.write_log(state);
			} else {
				carState.EngineerInfo = "SD card mount failed.";
				console << "     " << carState.EngineerInfo << NL;
			}
		}
		break;
	case DISPLAY_STATUS::DRIVER_RUNNING:
		carState.ConstantMode = (carState.ConstantMode == CONSTANT_MODE::POWER) ? CONSTANT_MODE::SPEED : CONSTANT_MODE::POWER;
		console << "Switch ConstMode toggle: switch to " << (carState.ConstantMode == CONSTANT_MODE::SPEED ? "SPEED" : "POWER") << NL;
		break;
	default:
		break;
	}
}

void sdCardDetectHandler() {
	if (!SystemInited)
		return;

	CarStatePin *pin = carState.getPin(ESP32_AC_SD_DETECT_GPIO35_name);
	if (pin == NULL)
		return;

	bool sdCardDetectOld = carState.SdCardDetect;
	carState.SdCardDetect = pin->value != 0;
	if (carState.SdCardDetect && !sdCardDetectOld) {
		carState.EngineerInfo = "SD card detected. Not mounted yet.";
		console << "     " << carState.EngineerInfo << NL;
		return;
	}

	if (!carState.SdCardDetect && sdCardDetectOld) {
		carState.EngineerInfo = "SD card removed.";
		console << "     " << carState.EngineerInfo << NL;
		sdCard.end();
	}
}
