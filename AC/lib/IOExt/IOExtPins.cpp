//
// MCP23017 I/O Extension over I2C
//
#include "../definitions.h"

#include <CarControl.h>
#include <Helper.h>
#include <IOExtHandler.h>

CarStatePin CarState::pins[] = {
    // GPIO, I/O mode, value, oldValue, inited, timestamp, name, handler, continouse mode, debaunce time (==0 for toggles!)
    // esp32 GIO pins
    {ESP32_AC_BUTTON_NEXT_SCREEN_GPIO27, INPUT_PULLDOWN, 0, 0, false, 0l, ESP32_AC_BUTTON_NEXT_SCREEN_GPIO27_name, NULL,   true,  200},
    {ESP32_AC_BUTTON_CONST_MODE_GPIO02,  INPUT_PULLDOWN, 0, 0, false, 0l, ESP32_AC_BUTTON_CONST_MODE_GPIO02_name,  NULL,   true,  200},
    {ESP32_AC_SD_DETECT_GPIO35,          INPUT,          0, 0, false, 0l, ESP32_AC_SD_DETECT_GPIO35_name,          NULL,   false, 200}
};

