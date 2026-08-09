//
// DS1307
//

#include "RTC_SER.h"

// std
#include <time.h>

// lib-deps
#include <RtcDS1307.h>

// project
#include <Console.h>
#include <Helper.h>
#include <I2CBus.h>
#include <iostream>

extern Console console;

/**
 * initialize GlobalTime and its RTC
 * returns sum of:
 * 0x0: ok.
 * 0x1: set_RTC_squarewave failed.
 * 0x2: set_RTC_running failed.
 * 0x4: get_RTC / set_RTC failed.
 * 0x8: set_RTC_running failed.
 */
int GlobalTime::init(DS1307SquareWaveOut sq_freq, bool do_run) {
  int err_code = 0;
  is_init = true; // must be true before accessing I2CBus
  try {
    auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    Rtc.Begin();
  } catch (const std::runtime_error &e) {
    console << "ERROR: RTC init failed: couldn't lock I2C bus for Rtc.Begin().\n";
  }
  err_code += 0x1 * !set_RTC_squarewave(sq_freq);
  err_code += 0x2 * !set_RTC_running(do_run);
  if (_datetime.TotalSeconds() == 0) {
    if (!get_RTC()) {
      err_code += 0x4;
      console << "ERROR: RTC read failed during init. Time fallback may appear as reset.\n";
    }
  } else {
    if (!_datetime.IsValid()) {
      err_code += 0x4;
      console << "ERROR: Refusing to write invalid datetime to RTC during init.\n";
    } else {
      err_code += 0x4 * !set_RTC(_datetime);
    }
  }
  err_code += 0x8 * !set_RTC_running(1);
  return err_code;
}

/**
 * @brief update internal datetime from cpu clock or RTC
 */
void GlobalTime::update() {
  constexpr int min_wait = 60; // min. time before next RTC query (seconds)
  if (!is_init)
    return;
  // auto lock = RAII_mux(mutex, portMAX_DELAY);
  // if (!lock.ok()) return;
  try {
    auto lock = RAII_mux(mutex, portMAX_DELAY);
    int sec = millis() / 1000;
    if ((sec - last_update) > min_wait || (sec - last_update) < 0) {
      // console << "INFO: Time update from RTC.\n";
      if (get_RTC())
        return;
      else
        console << "ERROR: RTC update failed, using cpu clock instead.\n";
    }
    // if get_RTC() not needed or failed:
    uint32_t sec_now = _datetime.TotalSeconds();
    sec_now += (sec - sec0);
    _datetime = RtcDateTime(sec_now);
    sec0 = sec;
    return;
  } catch (const std::runtime_error &e) {
    return;
  }
}

/** todo: rm
 * @brief get the time as an std:string object with the specified format
 * @param format
 * time format:
 * http://www.cplusplus.com/reference/ctime/strftime/
 * TL;DR: '%H:%M:%S %Y-%m-%d (%b)' for '09:10:59 2023-03-28 (Tue)'
 */

/**
 * get the time as an std:string object with the specified format:
 * http://www.cplusplus.com/reference/ctime/strftime/
 * TL;DR:
 * "%H:%M:%S %Y-%m-%d (%b)"" -> "09:10:59 2023-03-28 (Tue)"
 * "%F %R" -> "2023-03-28 09:10"
 * "%X" -> "09:10:59"
 * **max. return string length: 127 char**
 */
std::string GlobalTime::strTime(const std::string &format) {
  update();
  tm timeinfo;
  time_t now = _datetime.Unix32Time();
  gmtime_r(&now, &timeinfo);
  char s[128];
  const char *c = format.c_str();
  strftime(s, 127, c, &timeinfo);
  return std::string(s);
}

/**
 * get uptime as string in HH:MM:SS, or HH:MM:SS.ms if with_ms.
 * Does not make any use of RTC and can be used without it.
 */
string GlobalTime::strUptime(bool with_ms) {
  unsigned long seconds = millis() / 1000;
  unsigned long secsRemaining = seconds % 3600;
  int runHours = seconds / 3600;
  int runMinutes = secsRemaining / 60;
  int runSeconds = secsRemaining % 60;
  if (with_ms) {
    return fmt::format("{:02d}:{:02d}:{:02d}.{:03d}", runHours, runMinutes, runSeconds, millis() % 1000);
  } else {
    return fmt::format("{:02d}:{:02d}:{:02d}", runHours, runMinutes, runSeconds);
  }
}

// return 1 on success, 0 otherwise
bool GlobalTime::set_RTC_running(bool do_run) {
  if (!is_init) {
    console << "tried to set RTC before GlobalTime was init()ed.\n";
    return 0;
  }
  // auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
  // if (!lock.ok()) {
  try {
    auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    Rtc.SetIsRunning(do_run);
    return 1;
  } catch (const std::runtime_error &e) {
    cout << "ERROR: set_RTC_running() didn't get mux lock.\n";
    return 0;
  }
}

// 1 = on, 0 = off or communication error
bool GlobalTime::get_RTC_running() {
  if (!is_init) {
    console << "tried to set RTC before GlobalTime was init()ed.\n";
    return 0;
  }
  // auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
  // if (!lock.ok()) {
  try {
    auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    return Rtc.GetIsRunning();
  } catch (const std::runtime_error &e) {
    cout << "ERROR: get_RTC_running() didn't get mux lock.\n";
    return 0;
  }
}

bool GlobalTime::rtc_datetime_valid() {
  if (!is_init) {
    return 0;
  }
  try {
    auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    return Rtc.IsDateTimeValid();
  } catch (const std::runtime_error &e) {
    return 0;
  }
}

uint8_t GlobalTime::rtc_last_error() {
  if (!is_init) {
    return 0xff;
  }
  try {
    auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    return Rtc.LastError();
  } catch (const std::runtime_error &e) {
    return 0xff;
  }
}

// return 1 on success, 0 otherwise
bool GlobalTime::set_RTC_squarewave(DS1307SquareWaveOut sq_freq) {
  if (!is_init) {
    console << "tried to set RTC before GlobalTime was init()ed.\n";
    return 0;
  }
  // auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
  // if (!lock.ok()) {
  try {
    auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    Rtc.SetSquareWavePin(sq_freq);
    return 1;
  } catch (const std::runtime_error &e) {
    cout << "ERROR: set_RTC_squarewave() didn't get mux lock.\n";
    return 0;
  }
}

/**
 * @brief force update internal datetime from RTC
 * @retval 1 on success, 0 otherwise.
 * Does not return time directly in any form.
 */
bool GlobalTime::get_RTC() {
  if (!is_init) {
    console << "tried to read RTC before GlobalTime was init()ed.\n";
    return 0;
  }
  // auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
  // if (lock.ok()){
  try {
    auto lock = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    RtcDateTime rtc_now = Rtc.GetDateTime();
    if (!Rtc.IsDateTimeValid() || !rtc_now.IsValid()) {
      console << "ERROR: RTC datetime invalid (LastError=" << (int)Rtc.LastError()
              << "). Possible backup battery loss or uninitialized clock.\n";
      return 0;
    }
    _datetime = rtc_now;
    last_update = millis() / 1000;
    sec0 = last_update;
    return 1;
  } catch (const std::runtime_error &e) {
    console << "ERROR: RTC read failed: couldn't lock I2C bus.\n";
    return 0;
  }
}

/**
 * @brief set RTC time
 * @param secondsFrom2000: seconds since 2000-01-01
 * @retval 1 on success, 0 otherwise
 */
bool GlobalTime::set_RTC(uint32_t secondsFrom2000) {
  if (!is_init) {
    console << "tried to set RTC before GlobalTime was init()ed.\n";
    return 0;
  }
  // auto lock1 = RAII_mux(mutex, portMAX_DELAY);  // writing this->...
  // if (!lock1.ok()) return 0;
  // auto lock2 = RAII_mux(i2cBus->mutex, portMAX_DELAY);
  // if (lock2.ok()){
  try {
    auto lock1 = RAII_mux(mutex, portMAX_DELAY); // writing this->...
    auto lock2 = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    Rtc.SetDateTime(RtcDateTime(secondsFrom2000));
    _datetime = RtcDateTime(secondsFrom2000); // update internal time
    sec0 = millis() / 1000;                   // adjust for offline update
    return 1;
  } catch (const std::runtime_error &e) {
    return 0;
  }
}

/**
 * @brief set RTC time
 * @param datetime: RtcDateTime object
 * @retval 1 on success, 0 otherwise
 */
bool GlobalTime::set_RTC(const RtcDateTime &datetime) {
  if (!is_init) {
    console << "tried to set RTC before GlobalTime was init()ed.\n";
    return 0;
  }
  // auto lock1 = RAII_mux(mutex, portMAX_DELAY);  // writing this->...
  // if (!lock1.ok()) return 0;
  // auto lock2 = RAII_mux(i2cBus->mutex, portMAX_DELAY);
  // if (lock2.ok()){
  try {
    auto lock1 = RAII_mux(mutex, portMAX_DELAY); // writing this->...
    auto lock2 = RAII_mux(i2cBus->mutex, portMAX_DELAY);
    Rtc.SetDateTime(datetime);
    _datetime = datetime;   // update internal time
    sec0 = millis() / 1000; // adjust for offline update
    return 1;
  } catch (const std::runtime_error &e) {
    return 0;
  }
}