//
// MCP23017 I/O Extension over I2C
//
#include "../definitions.h"

#include <CarControl.h>
#include <Helper.h>
#include <IOExt.h>
#include <IOExtHandler.h>

CarStatePin CarState::pins[] = {
    // GPIO, I/O mode, value, oldValue, inited, timestamp, name, handler, continuos mode, debounce time (==0 for toggles!)
    // esp32 GIO pins
    {DI_ButtonPlus_GPIO18,        INPUT_PULLUP,   0, 0, false, 0l, DI_ButtonPlus_GPIO18_name,        buttonPlusHandler,              true,  200},
    {DI_ButtonMinus_GPIO19,       INPUT_PULLUP,   0, 0, false, 0l, DI_ButtonMinus_GPIO19_name,       buttonMinusHandler,             true,  200},
    {DI_ButtonSet_GPIO05,         INPUT_PULLUP,   0, 0, false, 0l, DI_ButtonSet_GPIO05_name,         buttonSetHandler,               false, 200},
    {DI_BreakPedal_GPIO04,        INPUT_PULLUP,   0, 0, false, 0l, DI_BreakPedal_GPIO04_name,        breakPedalHandler,              false, 0},
    {DO_BreakLight_GPIO27,        OUTPUT,         1, 1, false, 0l, DO_BreakLight_GPIO27_name,        NULL,                           false, 200},
    {DI_FWD_BWD_SENSOR_VN_GPIO39, INPUT,          0, 0, false, 0l, DI_FWD_BWD_SENSOR_VN_GPIO39_name, fwdBwdHandler,                  false, 0},  // don't pull z-diode!
    {DI_MCONOFF_SENSOR_VP_GPIO36, INPUT,          0, 0, false, 0l, DI_MCONOFF_SENSOR_VP_GPIO36_name, mcOnOffHandler,                 false, 0},  // don't pull z-diode!
    {DI_Button_Confirm_GPIO21,    INPUT_PULLUP,   0, 0, false, 0l, DI_Button_Confirm_GPIO21_name,    buttonConfirmDriverInfoHandler, false, 200}
};

