/*
 * Global definitions (pinout, device settings, ..)
 */
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define DEFINE_CONST(name, value) \
  constexpr auto name = value; \
  constexpr const char* name##_name = #name;
  
#define SER4TYPE "DC"

#include "../lib/LocalFunctionsAndDevices.h"

#define FILENAME_SER4CONFIG "/SER4CNFG.INI"

/* Non free selectable addresses:
 *
 * 6-axis inertial sensor, gyro:
 * #define BMI088_ACC_ADDRESS 0x19
 * #define BMI088_GYRO_ADDRESS 0x69
 *
 * RTC
 * const uint8_t DS1307_ADDRESS = 0x68;
 * 2026: DS1307 replaced by DS3231
 */
#define I2C_ADDRESS_DS3231 0x68

/*
 *  GPInputOutput
 */
#define GPIO_INTERRUPT_PIN 33

/*
 * OneWire
 */
#define ONEWIRE_PIN 12

/*
 * I2C
 */
#define I2C_SDA 23
#define I2C_SCL 22
//#define I2C_FREQ 200000 // 200kHz
#define I2C_FREQ 100000 // 100kHz  (DS1307 limit)
//#define I2C_FREQ 50000 // 50kHz

// analog digital coder
#define I2C_ADDRESS_ADS1x15 0x48
#define ADC_MAX 65535

// Puls width modifier
#define PWM_NUM_PORTS 16
#define PWM_MAX_VALUE 4096
#define I2C_ADDRESS_PCA9685 0x42

// Extended digital IOs
// #define MCP23017_NUM_DEVICES 1
// #define MCP23017_NUM_PORTS 16
//change#define IOExtPINCOUNT (MCP23017_NUM_DEVICES * MCP23017_NUM_PORTS)
#define IOPINCOUNT 8
//#define I2C_ADDRESS_MCP23017_IOExt0 0x20
//#define I2C_ADDRESS_MCP23017_IOExt1 0x21
//#define I2C_INTERRUPT 33

// // // digital potentiometer
// // // address = b0101{DS1803_ADDR2, DS1803_ADDR1, DS1803_ADDR0}
// // #define DS1803_BASE_ADDR 0x28
// // #define DS1803_ADDR0 0 // pulled down to ground
// // #define DS1803_ADDR1 0 // pulled down to ground
// // #define DS1803_ADDR2 0 // pulled down to ground
// // #define I2C_ADDRESS_DS1803 (DS1803_BASE_ADDR | (DS1803_ADDR2 << 2) | (DS1803_ADDR1 << 1) | DS1803_ADDR0)

// /*
//  * SERIAL, SERIAL2
//  *
//  * RX and TX are defined in pins_arduino.h, all others here
//  */
// #define SERIAL2_RX 16
// #define SERIAL2_TX 17

/*
 * CAN Bus
 *
 *  GPIO25  TX
 *  GPIO26  RX
 */
DEFINE_CONST(CAN_TX, 25)
DEFINE_CONST(CAN_RX, 26)

// /*
//  *  SPI
//  *
//  *  ESP32  - SPI PIN
//  *  --------------
//  *  VSPI
//  *  GPIO19   MOSI
//  *  GPIO18   MISO
//  *  GPIO5    CLK
//  *  GPIO21   CS (first spi device)
//  *
//  */
DEFINE_CONST(SPI_MOSI, 18)
DEFINE_CONST(SPI_MISO, 19)
DEFINE_CONST(SPI_CLK, 5)

DEFINE_CONST(SPI_DC, 4)
DEFINE_CONST(SPI_RST, 21)

DEFINE_CONST(SPI_CS_TFT, 32)
DEFINE_CONST(SPI_CS_SDCARD, 14)

// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
// esp32 GIO pins: X in [Analog,Digital,Pwm], Y in [I,O]
// DEFINE_CONST(XY_signalName_GPIO00, 0)
DEFINE_CONST(DO_TXDO_GPIO01, 1)
DEFINE_CONST(PO_DeccelPWM_GPIO02, 2)
DEFINE_CONST(DI_RXDO_GPIO03, 3)
DEFINE_CONST(DI_BreakPedal_GPIO04, 4)
DEFINE_CONST(DI_ButtonSet_GPIO05, 5)

DEFINE_CONST(DI_TDIGPIO12, 12)
DEFINE_CONST(DI_TCK_GPIO13, 13)
DEFINE_CONST(DI_TMS_GPIO14, 14)
DEFINE_CONST(DO_DTO_GPIO15, 15)
DEFINE_CONST(PO_AccelPWM_GPIO16, 16)
DEFINE_CONST(Dx_TestPoint_GPIO17, 17)
DEFINE_CONST(DI_ButtonPlus_GPIO18, 18)
DEFINE_CONST(DI_ButtonMinus_GPIO19, 19)

DEFINE_CONST(DO_SCL_GPIO22, 22)
DEFINE_CONST(DIO_SDA_GPIO23, 23)

DEFINE_CONST(DO_CAN_D_GPIO25, 25)
DEFINE_CONST(DI_CAN_R_GPIO26, 26)
DEFINE_CONST(DO_BreakLight_GPIO27, 27)

DEFINE_CONST(Xx_signal_GPIO32, 32)
DEFINE_CONST(Xx_signal_GPIO33, 33)
DEFINE_CONST(Xx_signal_GPIO34, 34)
DEFINE_CONST(Xx_signal_GPIO35, 35)

DEFINE_CONST(DI_Button_Confirm_GPIO21, 21) // "PinDI_Confirm_GPIO21"
DEFINE_CONST(DI_MCONOFF_SENSOR_VP_GPIO36, 36) //"DI_SENSOR_VP"
DEFINE_CONST(DI_FWD_BWD_SENSOR_VN_GPIO39, 39) // "DI_SENSOR_VN"


/*
 * ESP32 JTAG Debug Probe Wiring
 *
 *  ESP32  - Probe
 *  --------------
 *  GPIO12 - TDI
 *  GPIO15 - TDO
 *  GPIO13 - TCK
 *  GPIO14 - TMS
 *
 *  Tutorial:
 * https://medium.com/@manuel.bl/low-cost-esp32-in-circuit-debugging-dbbee39e508b
 *
 *  General ESP32 Pinout:
 * https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
 */

#endif // DEFINITIONS_H
