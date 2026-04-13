/*
 * MCP23017 I/O Extension over I2C  !!! UNTESTED !!!
 */

#ifndef SER_IOEXT_H
#define SER_IOEXT_H

#include "../definitions.h"

#include <list>
#include <map>
#include <string>

#include <AbstractTask.h>

#include <CarState.h>
#include <CarStatePin.h>

enum class PinHandleMode { NORMAL, FORCED };
// extern OneWireBus oneWireBus;

class IOExt : public AbstractTask {
public:
  // RTOS task
  string getName(void) { return "IOExt"; };
  string init(void);
  string re_init(void);
  void exit(void);
  void task(void *pvParams);

  // Class member and functions
  int getPort(int port);
  void setPort(int port, bool value);
  void writeAllPins(PinHandleMode mode = PinHandleMode::NORMAL);
  void readAllPins();
  bool readAndHandlePins(PinHandleMode mode = PinHandleMode::NORMAL);

  // static int getIdx(int devNr, int pin) { return devNr * 16 + pin; };
  // static int getIdx(int port) { return (port >> 4) * 16 + (port & 0x0F); };

  bool verboseModeDIn = false;
  bool verboseModeDInHandler = false;
  bool verboseModeDOut = false;

private:
  // bool isInInterruptHandler = false;
  bool isInInputHandler = false;

};
#endif // SER_IOEXT_H
