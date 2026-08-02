/*
 * Car controll loops
 */

#ifndef SER_CAR_CONTROL_H
#define SER_CAR_CONTROL_H

#include <list>
#include <map>
#include <string>

#include <AbstractTask.h>
#include <global_definitions.h>
#include "../definitions.h"

class CarControl : public AbstractTask {

public:
  // RTOS task
  string getName(void) { return "CarControlAC"; };
  string init(void);
  string re_init(void);
  void exit(void);
  void task(void *pvParams);

  bool verboseModeCarControl = false;
  bool verboseModeCarControlDebug = false;

private:
  int valueDisplayLast = INT_MAX;
  bool justInited = true;

  bool isInValueChangedHandler = false;
  void _handleValueChanged();
};
#endif // SER_CAR_CONTROL_H
