//
// Digital to Analog Converter
//

#ifndef SOLAR_CAR_CONTROL_SYSTEM_DAC_H
#define SOLAR_CAR_CONTROL_SYSTEM_DAC_H

#define DAC_MAX 255

class DAC {
private:
  bool isLocked = false; // TODO true;
  int MAX_DUTY_CYCLE = 1;
  const int PinBacklight = ESP32_AC_BACKLIGHT_PWM_GPIO33; /* GPIO16 */

  /* Setting PWM Properties */
  // const int PWMFreq = 5000; /* 5 KHz */
  const int PWMFreq = 1000; /* 1 KHz */
  //const int PWMFreq = 50; /* 50 Hz */
  const int PWMChannelBackLight = 0;
  const int PWMResolution = 10;

public:
  enum pot_chan {
    POT_CHAN0_BACKLIGHT = 0, // acceleration input
    POT_CHAN_ALL = 2,
  };

private:
  uint8_t pot0 = 0;

public:
  string getName(void) { return "DAC"; };
  string init();
  string re_init();
  bool reset_pot();
  bool set_pot(uint8_t val);
  uint16_t get_pot();

  bool verboseModeDAC = false;
};

#endif // SOLAR_CAR_CONTROL_SYSTEM_DAC_H
