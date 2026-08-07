//
// Command Receiver and Handler
//

#ifndef SOLAR_CAR_CONTROL_SYSTEM_CMDHANDLER_H
#define SOLAR_CAR_CONTROL_SYSTEM_CMDHANDLER_H

#include <AbstractTask.h>

class CmdHandler : public AbstractTask {
public:
  // RTOS task
  string getName(void) { return "CmdHandler"; };
  string init(void);
  string re_init(void);
  void exit(void);
  void task(void *pvParams);

  // Class functions and members
  string commands = "REDPSVJMUFBICOTKGi:!sc?";
  string helpText = "Available commands (" + commands +
                    "):\n"
                    "\t-------- SYSTEM COMMANDS -------------------------\n"
                    "\tR _ _ _ _ _ _ _ _ _ _ _ _ reset and reinit driver display\n"
                    "\tE _ _ _ _ _ _ _ _ _ _ _ _ switch to engineer screen\n"
                    "\tD [DriverName]_ _ _ _ _ _ switch to driver display | set driver for logs\n"
                    "\tP [file] [tailLines]_ _ _ print directory of sdcard\n"
                    "\tS [a] _ _ _ _ _ _ _ _ _ _ print status, a-all values\n"
                    "\tV [|+]  _ _ _ _ _ _ _ _ _ write_log to sdcard | with header (+)\n"
                    "\tM/U _ _ _ _ _ _ _ _ _ _ _ mount/unmount sdcard and enable/disable logging\n"
                    "\tH _ _ _ _ _ _ _ _ _ _ _ _ memory_info\n"
                    "\tB [v] _ _ _ _ _ _ _ _ _ _ show radio (Serial2), v-toggle verbose mode\n"
                    "\t  [r]                       - set serial2 baud rate\n"
                    "\t  [i]                       - set radio send interval (ms)\n"
                    "\t  [t|b]                     - set radio send mode: text mode|binary\n"
                    "\tF _ _ _ _ _ _ _ _ _ _ _ _ Re-read configuration\n"
                    "\tI _ _ _ _ _ _ _ _ _ _ _ _ read and show IOs\n"
                    "\t  [s]                       - scan and show I2C devices\n"
                    "\tC _ _ _ _ _ _ _ _ _ _ _ _ set CAN verbose mode\n"
                    "\t  [i]                       - verbose CAN in\n"
                    "\t  [I]                       - verbose CAN in native packages\n"
                    "\t  [o]                       - verbose CAN out\n"
                    "\t  [O]                       - verbose CAN out native packages\n"
                    "\t  [S]                       - verbose sd card\n"
                    "\t  motC batV                 - inject CAN: motor current [A] and battery voltage [V]\n"
                    "\tO _ _ _ _ _ _ _ _ _ _ _ _ set CarControl verbose mode\n"
                    "\t  [o]                     - verbose\n"
                    "\t  [O]                     - verbose debug\n"
                    "\tT [yyyy mm dd hh MM ss]__ get system time / set RTC date and time\n"
                    "\tK [|kp ki kd] _ _ _ _ _ _ show / update PID constants\n"
                    "\tG [|0...7]  _ _ _ _ _ _ _ show / update glide mode\n"
                    "\ti _ _ _ _ _ _ _ _ _ _ _ _ minimal drive to console\n"
                    "\t-------- DRIVER INFO COMMANDS --------------------\n"
                    "\t:<text> _ _ _ _ _ _ _ _ _ display driver info text\n"
                    "\t!<text> _ _ _ _ _ _ _ _ _ display driver warn text\n"
                    "\ts [|+|-]  _ _ _ _ _ _ _ _ speed arrow off, green up (+), red down (-)\n"
                    "\t-------- Driver SUPPORT COMMANDS -----------------\n"
                    "\tc [-|+|s|p] _ _ _ _ _ _ _ constant mode on (-:off|+:on|s:speed|p:power)\n";

  string printSystemValues(void);
};

#endif // SOLAR_CAR_CONTROL_SYSTEM_CMDHANDLER_H
