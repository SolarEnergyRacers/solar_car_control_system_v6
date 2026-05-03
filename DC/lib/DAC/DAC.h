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
  const int PinACCL = PO_AccelPWM_GPIO16; /* GPIO16 */
  const int PinDECCL = PO_DeccelPWM_GPIO02; /* GPIO02 */

  /* Setting PWM Properties */
  //const int PWMFreq = 5000; /* 5 KHz */
  const int PWMFreq = 50; /* 50 Hz */
  const int PWMChannelACCL = 0;
  const int PWMChannelDECL = 1;
  const int PWMResolution = 10;

public:
  enum pot_chan {
    POT_CHAN0_ACC = 0, // acceleration input
    POT_CHAN1_DEC = 1, // deceleration (recuperation) input
    POT_CHAN_ALL = 2,
  };

private:
  void unlock_acceleration() { isLocked = false; };
  uint8_t pot0 = 0;
  uint8_t pot1 = 0;

public:
  string getName(void) { return "DAC"; };
  string init();
  string re_init();
  bool reset_pot();
  void lock_acceleration();
  void reset_and_lock_pot();
  bool set_pot(uint8_t val, pot_chan channel);
  uint16_t get_pot(pot_chan channel);

  bool verboseModeDAC = false;
};

#endif // SOLAR_CAR_CONTROL_SYSTEM_DAC_H
