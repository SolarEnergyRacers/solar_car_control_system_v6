# HowTos for SER V6 with ESP32 and platformio

## Run MenuConfig

writes the file `sdconfig` and sdconfig.esp32dev

`pio run -t menuconfig`

## Upload without recompile

`pio run -t nobuild -t upload --disable-auto-clean`

`platformio run --target upload --target monitor --environment esp32dev-linux` 

## Thread Safe C++

[c-thread-safe-queue](https://www.educba.com/c-thread-safe-queue/)

## Error: Brownout detector was triggered

[Hardware and Software Solution](https://arduino.stackexchange.com/questions/76690/esp32-brownout-detector-was-triggered-upon-wifi-begin)

## Platformio vscode

### Change `Upload` to `Upload and Monitor`

in `~/.config/Code/User/settings.json`:

behind

```json
{
  "text": "$(arrow-right)",
  "tooltip": "PlatformIO: Upload",
  "commands": "platformio-ide.upload"
},
```

add

```json
{
  "text": "$(arrow-right)$(plug)",
  "tooltip": "PlatformIO: Upload and Monitor",
  "commands": "platformio-ide.uploadAndMonitor"
},
```

**complete `platformio-ide` part in** `~/.config/Code/User/settings.json`:

```json
    "platformio-ide.forceUploadAndMonitor": true,
    "platformio-ide.disablePIOHomeStartup": true,
    "platformio-ide.customPATH": "",
    "platformio-ide.toolbar": [
        {
            "text": "$(home)",
            "tooltip": "PlatformIO: Home",
            "commands": "platformio-ide.showHome"
        },
        {
            "text": "$(check)",
            "tooltip": "PlatformIO: Build",
            "commands": "platformio-ide.build"
        },
        {
            "text": "$(arrow-right)",
            "tooltip": "PlatformIO: Upload",
            "commands": "platformio-ide.upload"
        },
        {
            "text": "$(arrow-right)$(plug)",
            "tooltip": "PlatformIO: Upload and Monitor",
            "commands": "platformio-ide.uploadAndMonitor"
        },
        {
            "text": "$(trashcan)",
            "tooltip": "PlatformIO: Clean",
            "commands": "platformio-ide.clean"
        },
        {
            "text": "$(beaker)",
            "tooltip": "PlatformIO: Test",
            "commands": "platformio-ide.test"
        },
        {
            "text": "$(plug)",
            "tooltip": "PlatformIO: Serial Monitor",
            "commands": "platformio-ide.serialMonitor"
        },
        {
            "text": "$(terminal)",
            "tooltip": "PlatformIO: New Terminal",
            "commands": "platformio-ide.newTerminal"
        }
    ],
    "platformio-ide.reopenSerialMonitorDelay": 1000,

```

## Library Notes
### PWM

Theorie, Tutorials:

- [readthedocs.io | python](https://super-starter-kit-for-esp32-s3-wroom.readthedocs.io/en/latest/2.Python_Tutorial/4_analog%26pwm.html)
- [www.electronicshub.org | esp32 freeRTOS](https://www.electronicshub.org/esp32-pwm-tutorial/)

Libraries:

- [github.com | ESP32 PWM](https://github.com/khoih-prog/ESP32_PWM?tab=readme-ov-file) ([www.arduinolibraries.info | libraries/esp](https://www.arduinolibraries.info/libraries/esp32_pwm))
- [registry.platformio.org | libraries/khoih-prog/ESP](https://registry.platformio.org/libraries/khoih-prog/ESP32_PWM)

#### Running Demo on DC,v2-Controller (SER6) 1

```cpp
  const int PinACCL = PO_AccelPWM_GPIO16; /* GPIO16 */
  const int PinDECCL = PO_DeccelPWM_GPIO02; /* GPIO02 */

  int dutyCycle;
  /* Setting PWM Properties */
  //const int PWMFreq = 5000; /* 5 KHz */
  const int PWMFreq = 50; /* 50 Hz */
  const int PWMChannelACCL = 0;
  const int PWMChannelDECL = 1;
  const int PWMResolution = 10;
  const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);


  uint32_t retValue = ledcSetup(PWMChannelACCL, PWMFreq, PWMResolution);
  /* Attach the LED PWM Channel to the GPIO Pin */
  ledcAttachPin(PinACCL, PWMChannelACCL);
  ledcAttachPin(PinDECCL, PWMChannelDECL);

  console << "PWM inited [" << retValue << "] at GPIO " << PO_AccelPWM_GPIO16_name << "(" << PO_AccelPWM_GPIO16 << ")" << NL;
  while (1) {
    /* Increasing the LED brightness with PWM */
    console << "increasing 0 - " << MAX_DUTY_CYCLE << "...";
    for (dutyCycle = 0; dutyCycle <= MAX_DUTY_CYCLE; dutyCycle++) {
      ledcWrite(PWMChannelACCL, dutyCycle);
      ledcWrite(PWMChannelDECL, MAX_DUTY_CYCLE-dutyCycle);
      vTaskDelay(10);
    }
    /* Decreasing the LED brightness with PWM */
    console << " --- decreasing " << MAX_DUTY_CYCLE << " - 0...";
    for (dutyCycle = MAX_DUTY_CYCLE; dutyCycle >= 0; dutyCycle--) {
      ledcWrite(PWMChannelACCL, dutyCycle);
      ledcWrite(PWMChannelDECL, MAX_DUTY_CYCLE-dutyCycle);
      vTaskDelay(10);
    }
    console << NL << "next cycle: ";
  }
```

#### Running Demo on DC,v2-Controller (SER6) 2

```cpp
  // Espressif 2.0
  // const int PinACCL = PO_AccelPWM_GPIO16; /* GPIO16 */
  // const int PinDECCL = PO_DeccelPWM_GPIO02; /* GPIO02 */

  // int dutyCycle;
  // /* Setting PWM Properties */
  // //const int PWMFreq = 5000; /* 5 KHz */
  // const int PWMFreq = 50; /* 50 Hz */
  // const int PWMChannelACCL = 0;
  // const int PWMChannelDECL = 1;
  // const int PWMResolution = 10;
  // const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);

  // uint32_t retValue = ledcSetup(PWMChannelACCL, PWMFreq, PWMResolution);
  // /* Attach the LED PWM Channel to the GPIO Pin */
  // ledcAttachPin(PinACCL, PWMChannelACCL);
  // ledcAttachPin(PinDECCL, PWMChannelDECL);

  // console << "PWM inited [" << retValue << "] at GPIO " << PO_AccelPWM_GPIO16_name << "(" << PO_AccelPWM_GPIO16 << ")" << NL;
  // while (1) {
  //   /* Increasing the LED brightness with PWM */
  //   console << "increasing 0 - " << MAX_DUTY_CYCLE << "...";
  //   for (dutyCycle = 0; dutyCycle <= MAX_DUTY_CYCLE; dutyCycle++) {
  //     ledcWrite(PWMChannelACCL, dutyCycle);
  //     ledcWrite(PWMChannelDECL, MAX_DUTY_CYCLE-dutyCycle);
  //     vTaskDelay(10);
  //   }
  //   /* Decreasing the LED brightness with PWM */
  //   console << " --- decreasing " << MAX_DUTY_CYCLE << " - 0...";
  //   for (dutyCycle = MAX_DUTY_CYCLE; dutyCycle >= 0; dutyCycle--) {
  //     ledcWrite(PWMChannelACCL, dutyCycle);
  //     ledcWrite(PWMChannelDECL, MAX_DUTY_CYCLE-dutyCycle);
  //     vTaskDelay(10);
  //   }
  //   console << NL << "next cycle: ";
  // }

  // ledc_timer_config_t conf_timer;
  // conf_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
  // conf_timer.duty_resolution = LEDC_TIMER_8_BIT;
  // conf_timer.timer_num = LEDC_TIMER_0;
  // conf_timer.freq_hz = 1000 ;
  // //conf_timer.clk_cfg = LEDC_AUTO_CLK;
  // if(ledc_timer_config(&conf_timer) == ESP_OK)
  // {
  // 	printf("led driver inited \n");
  // }
  // else
  // {
  // 	printf("led driver init failed \n");
  // }

  // Espressif 3.0
  // #if CONFIG_PM_ENABLE
  //   esp_pm_config_t pm_config = {.max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
  //                                .min_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
  // #if CONFIG_FREERTOS_USE_TICKLESS_IDLE
  //                                .light_sleep_enable = true
  // #endif
  //   };
  //   ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
  // #endif
  //   // Set the LEDC peripheral configuration
  //   example_ledc_init();
  //   // Set duty to 50%
  //   ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
  //   // Update duty to apply the new value
  //   ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

  // while (1) {
  //     /* Increasing the LED brightness with PWM */
  //     console << "increasing 0 - " << LEDC_DUTY << "...";
  //     for (int dutyCycle = 0; dutyCycle <= LEDC_DUTY; dutyCycle++) {
  //       ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, dutyCycle);
  //       vTaskDelay(10);
  //     }
  //     /* Decreasing the LED brightness with PWM */
  //     console << " --- decreasing " << LEDC_DUTY << " - 0...";
  //     for (int dutyCycle = LEDC_DUTY; dutyCycle >= 0; dutyCycle--) {
  //       ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, dutyCycle);
  //       vTaskDelay(10);
  //     }
  //     console << NL << "next cycle: ";
  //   }
```

## ttyUSB

`ll /dev/ttyU* ; ll /dev/esp32-*`

```bash
sudo nano /etc/udev/rules.d/99-esp32.rules
```

```rules
# Rule for first ESP32 board (plugged into port with KERNELS 1-1.2)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", KERNELS=="1-2", SYMLINK+="esp32-ac"

# Rule for second ESP32 board (plugged into port with KERNELS 1-1.3)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", KERNELS=="1-1", SYMLINK+="esp32-dc"
```

```bash
sudo udevadm control --reload-rules && sudo udevadm trigger

```

