//
// Car Speed PID Control
//

#include "../definitions.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <fmt/core.h>
#include <fmt/format.h>
#include <inttypes.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>

#include <CarControl.h>
#include <CarState.h>
#include <ConstSpeed.h>

// #include <ADC_SER.h>
#include <Console.h>
#include <DAC.h>
#include <Helper.h>
#include <PID_v1.h>

// extern ADC adc;
extern Console console;
extern PID pid;
extern ConstSpeed constSpeed;
extern CarState carState;
extern CarControl carControl;
extern bool SystemInited;
// extern DAC dac;

float normalisation_factor = (float)MAX_ACCELERATION_DISPLAY_VALUE / DAC_MAX;

string ConstSpeed::re_init() {
  pid = PID(&input_value, &output_setpoint, &target_speed, carState.Kp, carState.Ki, carState.Kd, DIRECT);
  output_setpoint = 0;
  pid.SetMode(AUTOMATIC);
  return "PID reinited";
}

string ConstSpeed::init() {
  bool hasError = false;
  console << "[  ] Init 'ConstSpeed'...\n";
  target_speed = 0;
  pid = PID(&input_value, &output_setpoint, &target_speed, carState.Kp, carState.Ki, carState.Kd, DIRECT);
  output_setpoint = 0;
  pid.SetMode(AUTOMATIC);
  pid.SetOutputLimits(-DAC_MAX, DAC_MAX);
  return fmt::format("[{}] ConstSpeed initialized.", hasError ? "--" : "ok");
}

void ConstSpeed::exit(void) { set_target_speed(0); }
// ------------------

void ConstSpeed::set_target_speed(double speed) { target_speed = speed; }

void ConstSpeed::target_speed_incr(void) { target_speed += speed_increment; }

void ConstSpeed::target_speed_decr(void) { target_speed -= speed_increment; }

double ConstSpeed::get_target_speed() { return target_speed; }

double ConstSpeed::get_current_speed() { return carState.Speed; }

void ConstSpeed::set_pid(double Kp, double Ki, double Kd) {
  carState.Kp = Kp;
  carState.Ki = Ki;
  carState.Kd = Kd;
  pid.SetTunings(carState.Kp, carState.Ki, carState.Kd);
}

double ConstSpeed::getKp() { return pid.GetKp(); }
double ConstSpeed::getKi() { return pid.GetKi(); }
double ConstSpeed::getKd() { return pid.GetKd(); }
  
void ConstSpeed::update_pid() {
  if (carState.Kp != pid.GetKp() || carState.Ki != pid.GetKi() || carState.Kd != pid.GetKd())
    pid.SetTunings(carState.Kp, carState.Ki, carState.Kd);
}

void ConstSpeed::task(void *pvParams) {

  /*
   * How is accelration / decelartion handled in hardware?
   *
   * - acceleration: Digital to analog converter value representing the target speed -> 0V: stop, 5V -> max speed
   * - a separate I/O is used for forward/reverse switching
   * - deceleration/recuperation: Digital to analog converter value representing the recuperation amount: -> 0V: no rec, 5V: max recup
   * -> Note: In case of recup > 0, we should have acceleration 0
   *
   * TODO: ini file: recuperate on constant speed mode , or just let it roll (i.e. let it roll if the speed is too high is less convenient
   * for the driver, however, it conserves energy since we do not over-regulate) For the moment, we recuperate
   */

  while (1) {
    if (SystemInited && carState.ConstantModeOn && carState.ConstantMode == CONSTANT_MODE::SPEED) {
      // read target speed
      input_value = carState.SpeedExact;
      target_speed = carState.TargetSpeed;

      int accelerationDisplay_paddle = carControl.calculate_acceleration_display(carState.Deceleration, carState.Acceleration);

      // update pid controller
      bool hasNewValue = pid.Compute();
      if (verboseModePID) {
        console << fmt::format("# curSpeed={:4.0f} -> tarSpeed={:4.0f} => accD={:3d}, oSP={:8.2f}", input_value, target_speed,
                               carState.AccelerationDisplay, output_setpoint);
      }
      if (!hasNewValue) {
        if (verboseModePID) {
          console << fmt::format(" ==> OK.") << NL;
        }
        return;
      }
      if (verboseModePID) {
        console << fmt::format(" ==> CTRL: ");
      }
      // set acceleration & deceleration
      uint8_t acc = 0;
      uint8_t dec = 0;
      int accelerationDisplay_SetPoint = 0;

      carControl.read_paddles();

      if (output_setpoint > 0) {
        acc = round(output_setpoint);
      } else if (output_setpoint < 0) {
        dec = round(-output_setpoint * carState.GlideMode / 7.);
      }
      accelerationDisplay_SetPoint = round((acc > 0 ? acc : -dec) * normalisation_factor);

      if (accelerationDisplay_paddle > accelerationDisplay_SetPoint)
        carState.AccelerationDisplay = accelerationDisplay_paddle;
      else
        carState.AccelerationDisplay = accelerationDisplay_SetPoint;

      carControl.set_DAC();

      if (verboseModePID) {
        console << fmt::format("dec={:5d}, acc={:5d}, nSP:{:8.2f}", dec, acc, output_setpoint)
                << fmt::format(" | Decl={:6d} | Accl={:6d} | => [{:4d}|{:4d}]  | Brake P:{:3s}[L:{:3s}]\n", carState.Deceleration,
                               carState.Acceleration, carState.AccelerationDisplay, accelerationDisplay_paddle,
                               carState.BreakPedal ? "ON" : "OFF", carState.getPin(DO_BreakLight_GPIO27)->value ? "ON" : "OFF");
      }
    }
    taskSuspend();
  }
}
