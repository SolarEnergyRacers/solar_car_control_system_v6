//
// EngineeringDisplay
//

#ifndef SER_ENGINEER_DISPLAY_H
#define SER_ENGINEER_DISPLAY_H

#include <global_definitions.h>
#include "../definitions.h"

#include <Display.h>
#include <DisplayValue.h>
// #include <Wire.h>
// #include <AbstractTask.h>

class EngineerDisplay : public AbstractTask {
public:
  EngineerDisplay();
  ~EngineerDisplay();
  // RTOS task
  string init(void);
  string re_init(void);
  void exit(void){};
  void task(void *pvParams);

  string getName(void) { return "EngineerDisplay"; };
  bool verboseModeEngineer = false;

  int number_of_lines = 3;
  int line_length = 53;

protected:
  // DISPLAY_STATUS display_task();
  //==== overwrites from base class ==== END

private:
  //==== display cache =====================
  // ... to avoid flickering
  bool justInited;
  //=======================================

  //==== Engineer Display definitions ==== START
  // Parameters: 
  // int x, int y, string label, string format = "%4.1f", string unit = "", int textColor = ILI9341_BLACK, int bgColor = ILI9341_ORANGE, int textSize = 2
  // Stati [On/Off]
  DisplayValue<bool> MotorOn =        DisplayValue<bool>(4, 10, "MC :", "%3s", "", ILI9341_BLUE, ILI9341_ORANGE, 1);
  DisplayValue<bool> BatteryOn =      DisplayValue<bool>(4, 20, "Bat:", "%3s", "", ILI9341_BLUE, ILI9341_ORANGE, 1);
  DisplayValue<bool> PhotoVoltaicOn = DisplayValue<bool>(4, 30, "PV :", "%3s", "", ILI9341_BLUE, ILI9341_ORANGE, 1);
  // MPPTs 1...3 [A] and MPPT1...4 temperature [°C]
  DisplayValue<float> Mppt1 =        DisplayValue<float>(169,  10, "MPPT1:", "%5.2f", "A");
  DisplayValue<float> Temperature1 = DisplayValue<float>(169,  30, "      ", "%5.1f", "C");
  DisplayValue<float> Mppt2 =        DisplayValue<float>(169,  50, "    2:", "%5.2f", "A");
  DisplayValue<float> Temperature2 = DisplayValue<float>(169,  70, "      ", "%5.1f", "C");
  DisplayValue<float> Mppt3 =        DisplayValue<float>(169,  90, "    3:", "%5.2f", "A");
  DisplayValue<float> Temperature3 = DisplayValue<float>(169, 110, "      ", "%5.1f", "C");
  DisplayValue<float> Mppt4 =        DisplayValue<float>(169, 130, "    4:", "%5.2f", "A");
  DisplayValue<float> Temperature4 = DisplayValue<float>(169, 150, "      ", "%5.1f", "C");

  // Battery status [OK/Error]
  DisplayValue<string> BatteryStatus = DisplayValue<string>(4, 70, "Battery:", "%18s", "", ILI9341_BLACK, ILI9341_ORANGE, 1.5);
  // BMS error [string]
  DisplayValue<string> BmsStatus = DisplayValue<string>(4, 80, "BMS Msg:", "%18s", "", ILI9341_BLACK, ILI9341_ORANGE, 1.5);

  // Min and max BMS cell temperature [°C]
  DisplayValue<float> TemperatureMin = DisplayValue<float>(170, 170, "BATmin:", "%4.1f", "C", ILI9341_DARKCYAN);
  DisplayValue<float> TemperatureMax = DisplayValue<float>(170, 190, "   max:", "%4.1f", "C", ILI9341_DARKCYAN);

  // BMS current [A]
  DisplayValue<float> BatteryCurrent = DisplayValue<float>(4, 110, "BMS I:", "%5.2f", "A");
  // Voltage battery [V]
  DisplayValue<float> BatteryVoltage = DisplayValue<float>(4, 130, "    U:", "%5.1f", "V");
  // Voltage min, max, average [V]
  DisplayValue<float> VoltageMin = DisplayValue<float>(4, 150, "U min:", "%5.3f", "V");
  DisplayValue<float> VoltageAvg = DisplayValue<float>(4, 170, "  avg:", "%5.3f", "V");
  DisplayValue<float> VoltageMax = DisplayValue<float>(4, 190, "  max:", "%5.3f", "V");
  DisplayValue<string> EngineerInfo = DisplayValue<string>(0, 212, "", "%s", "", ILI9341_RED, ILI9341_ORANGE, 1);
  //==== Engineer Display definition ==== END

  void write_engineer_info(bool force = false);
  
  void draw_display_background();
  string display_setup();
};

#endif // SER_ENGINEER_DISPLAY_H DISPLAY_H
