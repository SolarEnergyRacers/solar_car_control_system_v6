/*
 * Global definitions (pinout, device settings, ..)
 */
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define SER4TYPE "AC"

#include "../lib/LocalFunctionsAndDevices.h"

#define FILENAME_SER4CONFIG "/SER4CNFG.INI"

#define DEFINE_CONST(name, value) \
  constexpr auto name = value; \
  constexpr const char* name##_name = #name;

/*
 * ESP32 IOs.
 */
DEFINE_CONST(ESP32_AC_BUTTON_NEXT_SCREEN_GPIO27, 27) // Next Screen
DEFINE_CONST(ESP32_AC_BUTTON_CONST_MODE_GPIO02, 2)   // switch constant mode (Speed, Power)
DEFINE_CONST(ESP32_AC_SD_DETECT_GPIO35, 35)          // SD card detect

/* Non free selectable addresses:
 *
 * 6-axis inertial sensor, gyro:
 * DEFINE_CONST(BMI088_ACC_ADDRESS, 0x19)
 * DEFINE_CONST(BMI088_GYRO_ADDRESS, 0x69)
 *
 * RTC
 * const uint8_t DS3231_ADDRESS = 0x68;
 */
DEFINE_CONST(I2C_ADDRESS_DS3231, 0x68)

/*
 *  GPInputOutput
 */
//DEFINE_CONST(GPIO_INTERRUPT_PIN, 33)

/*
 * OneWire
 */
DEFINE_CONST(ONEWIRE_PIN, 12)

/*
 * I2C
 */
DEFINE_CONST(I2C_SDA, 23)
DEFINE_CONST(I2C_SCL, 22)
//DEFINE_CONST(I2C_FREQ, 200000) // 200kHz
DEFINE_CONST(I2C_FREQ, 100000) // 100kHz  (DS1307 limit)
//DEFINE_CONST(I2C_FREQ, 50000) // 50kHz

// analog digital coder
DEFINE_CONST(I2C_ADDRESS_ADS1x15, 0x48)
DEFINE_CONST(ADC_MAX, 65535)

// Puls width modifier
// DEFINE_CONST(PWM_NUM_PORTS, 16)
// DEFINE_CONST(PWM_MAX_VALUE, 4096)
// DEFINE_CONST(I2C_ADDRESS_PCA9685, 0x42)



DEFINE_CONST(IOExtPINCOUNT, 10)
//DEFINE_CONST(I2C_INTERRUPT, 33)

/*
 * SERIAL, SERIAL2
 *
 * RX and TX are defined in pins_arduino.h, all others here
 */
DEFINE_CONST(SERIAL2_RX, 16)
DEFINE_CONST(SERIAL2_TX, 17)

/*
 * CAN Bus
 *
 *  GPIO25  TX
 *  GPIO26  RX
 */
#define CAN_TX (gpio_num_t)25
#define CAN_RX (gpio_num_t)26

/*
 *  SPI
 *
 *  ESP32  - SPI PIN
 *  --------------
 *  VSPI
 *  GPIO19   MOSI
 *  GPIO18   MISO
 *  GPIO5    CLK
 *  GPIO21   CS (first spi device)
 *
 */
DEFINE_CONST(SPI_MOSI, 18)
DEFINE_CONST(SPI_MISO, 19)
DEFINE_CONST(SPI_CLK, 5)

DEFINE_CONST(SPI_DC, 4)
DEFINE_CONST(SPI_RST, 21)

DEFINE_CONST(SPI_CS_TFT, 32)
DEFINE_CONST(SPI_CS_SDCARD, 14)

// Driver display layout offsets
// Positive values move the display area down, negative values move it up.
DEFINE_CONST(DRIVER_DISPLAY_INFO_FRAME_OFFSET_Y, 0)
DEFINE_CONST(DRIVER_DISPLAY_CONTENT_FRAME_OFFSET_Y, 0)

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
