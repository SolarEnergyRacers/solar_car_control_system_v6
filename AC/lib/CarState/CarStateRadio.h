//
// Car State for Radio
//

#ifndef CARSTATERADIO_H
#define CARSTATERADIO_H

#include <list>
#include <map>
#include <sstream>
#include <string>

#include <global_definitions.h>
#include "../definitions.h"

#include <CANBus.h>
#include <CANPacket.h>

using namespace std;

enum class SEND_MODE { BINARY, ASCII };
static const char *SEND_MODE_str[] = {"BINARY", "ASCII"};

class CarStateRadio {
private:
  unsigned long lastSendMillis;
  void send_ascii();
  void send_binary();

  std::map<uint16_t, CANPacket> packet_cache;

  list<uint16_t> radio_packets = {
      // mandatory
      // DC / AC control state
      (uint16_t)DC_BASE0x00,   // lifeSign, potentiometer, acceleration, deceleration
      (uint16_t)DC_BASE0x01,   // targetSpeed, targetPower, accelDisplay, speed + drive flags
      (uint16_t)AC_BASE0x00,   // lifeSign, constant mode, Kp/Ki/Kd, glide mode
      // MPPT output telemetry (per tracker)
      (uint16_t)Mppt1Base0x01, // MPPT1 output (voltage/current/power)
      (uint16_t)Mppt2Base0x01, // MPPT2 output (voltage/current/power)
      (uint16_t)Mppt3Base0x01, // MPPT3 output (voltage/current/power)
      (uint16_t)Mppt4Base0x01, // MPPT4 output (voltage/current/power)
      // BMS core telemetry / CMU data
      (uint16_t)BmsBase0x00,   // BMU heartbeat
      (uint16_t)BmsBase0x01,   // CMU temperatures set 1
      (uint16_t)BmsBase0x02,   // CMU temperatures set 2
      (uint16_t)BmsBase0x03,   // CMU temperatures set 3
      (uint16_t)BmsBase0x03,   // CMU temperatures set 3 (duplicate kept intentionally)
      (uint16_t)BmsBase0x04,   // CMU1 cell voltages part 1
      (uint16_t)BmsBase0x05,   // CMU1 cell voltages part 2
      (uint16_t)BmsBase0x06,   // CMU2 cell voltages part 1
      (uint16_t)BmsBase0x07,   // CMU2 cell voltages part 2
      (uint16_t)BmsBase0x08,   // CMU3 cell voltages part 1
      (uint16_t)BmsBase0x09,   // CMU3 cell voltages part 2
      (uint16_t)BmsBase0x0A,   // BMS extended CMU/pack telemetry
      (uint16_t)BmsBase0x0B,   // BMS extended CMU/pack telemetry
      (uint16_t)BmsBase0x0C,   // BMS extended CMU/pack telemetry
      // BMS pack state / status blocks
      (uint16_t)BmsBase0xF4,   // pack SOC
      (uint16_t)BmsBase0xF5,   // balance SOC
      (uint16_t)BmsBase0xF6,   // charger control
      (uint16_t)BmsBase0xF7,   // precharge status
      (uint16_t)BmsBase0xF8,   // min/max cell voltage
      (uint16_t)BmsBase0xF9,   // min/max cell temperature
      (uint16_t)BmsBase0xFA,   // pack voltage + pack current
      (uint16_t)BmsBase0xFB,   // pack status
      (uint16_t)BmsBase0xFC,   // pack fan status
      (uint16_t)BmsBase0xFD,   // external pack status / battery error bits
      (uint16_t)BmsBase0xF7,   // precharge status (duplicate kept intentionally)
      (uint16_t)BmsBase0xF8,   // min/max cell voltage (duplicate kept intentionally)
      (uint16_t)BmsBase0xF9,   // min/max cell temperature (duplicate kept intentionally)
      (uint16_t)BmsBase0xFA,   // pack voltage + pack current (duplicate kept intentionally)
      (uint16_t)BmsBase0xFD,   // external pack status / battery error bits (duplicate kept intentionally)
      // (uint16_t)McBase0x09,    //
      // (uint16_t)McBase0x0e,    //
      // (uint16_t)McBase0x0f,    //
      // (uint16_t)McBase0x10,    //
      // (uint16_t)McBase0x1b,    //
      // nice to have
      (uint16_t)Mppt1Base0x00, // MPPT1 input (panel side voltage/current/power)
      (uint16_t)Mppt1Base0x02, // MPPT1 temperature
      (uint16_t)Mppt2Base0x00, // MPPT2 input (panel side voltage/current/power)
      (uint16_t)Mppt2Base0x02, // MPPT2 temperature
      (uint16_t)Mppt3Base0x00, // MPPT3 input (panel side voltage/current/power)
      (uint16_t)Mppt3Base0x02, // MPPT3 temperature
      (uint16_t)Mppt4Base0x00, // MPPT4 input (panel side voltage/current/power)
      (uint16_t)Mppt4Base0x02, // MPPT4 temperature
      (uint16_t)BmsBase0x01,   // CMU temperatures set 1 (extra refresh)
      (uint16_t)BmsBase0x02    // CMU temperatures set 2 (extra refresh)
  };

public:
  CarStateRadio() {
    // BEGIN prevent stupid compiler warnings "defined but not used"
    (void)SEND_MODE_str;
    // BEGIN prevent stupid compiler warnings "defined but not used"
  }
  SEND_MODE mode = SEND_MODE::BINARY;
  void send();
  void push_if_radio_packet(uint16_t adr, CANPacket packet);
  bool verboseModeRadioSend = false;
};

#endif // CARSTATERADIO_H