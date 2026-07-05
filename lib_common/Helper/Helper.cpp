//
// Helper Functions
//
#include <fmt/core.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <global_definitions.h>

#include <iostream>
#include <string>
// #include <../.pio/libdeps/esp32dev/ESP32Time/ESP32Time.h>
// #include <../.pio/libdeps/esp32dev/RTCDS1307/RTCDS1307.h>

#include <Console.h>
#include <Helper.h>
#ifdef DCMODE
#pragma message "Helper - DCMODE"
/*no rtc timer available*/
#else
#pragma message "Helper - ACMODE"
#include <RtcDateTime.h>
#endif

extern Console console;
// extern RTC rtc;
// extern ESP32Time esp32time;

using namespace std;

char* fgets_stdio_blocking(char* str, int n) {
    char c;
    int i = 0;
    do {
        c = getchar();
        if (c == 255) {  // no char available to consume
            // taskYIELD(); // does not seem to do the trick
            vTaskDelay(100 / portTICK_PERIOD_MS);
        } else {  // store char
            str[i++] = c;
        }
        console << "[" << i << ":" << c << "]";
    } while (i < (n - 1) && c != '\n' && c != '\r');
    str[i] = 0;  // add zero-termination

    // on success
    return str;
}

void xSemaphoreTakeT(xQueueHandle mutex) {
    if (!xSemaphoreTake(mutex, portMAX_DELAY)) {
        console << "ERROR: mutex ************************************ " << mutex
                << " ****************\n";
        throw runtime_error("ERROR: mutex");
    }
}

#ifdef DCMODE
#pragma message "Helper - DCMODE"
string getDateTime() {
    return "xx.xx.xxxx xx:xx";
}  // esp32time.getTime("%Y-%m-%d,%H:%M:%S").c_str(); }
string getTime() { return "xx:xx"; }  // esp32time.getTime("%H:%M:%S").c_str(); }
#else
#pragma message "Helper - ACMODE"
// https://github.com/fbiego/ESP32Time
string formatDateTime(RtcDateTime now) {
    string static dateTimeString = fmt::format(
        "{:04d}-{:02d}-{:02d},{:02d}:{:02d}:{:02d}", now.Year(), now.Month(),
        now.Day(), now.Hour(), now.Minute(), now.Second());
    return dateTimeString;
}
#endif

string getTimeStamp() {
    unsigned long seconds = millis() / 1000;
    unsigned long secsRemaining = seconds % 3600;
    int runHours = seconds / 3600;
    int runMinutes = secsRemaining / 60;
    int runSeconds = secsRemaining % 60;
    return fmt::format("T{:02d}:{:02d}:{:02d}", runHours, runMinutes, runSeconds);
}

uint16_t normalize_0_UINT16(uint16_t minOriginValue, uint16_t maxOriginValue,
                            uint16_t value) {
    float k = (float)UINT16_MAX / (maxOriginValue - minOriginValue);
    value = value < minOriginValue ? minOriginValue : value;
    value = value > maxOriginValue ? maxOriginValue : value;
    return (uint16_t)round((value - minOriginValue) * k);
}

int transformArea(int minViewValue, int maxViewValue, int minOriginValue,
                  int maxOriginValue, int value) {
    float k =
        (float)(maxViewValue - minViewValue) / (maxOriginValue - minOriginValue);
    value = value < minOriginValue ? minOriginValue : value;
    value = value > maxOriginValue ? maxOriginValue : value;
    value = (int)round((value - minOriginValue) * k);
    value = value < minViewValue ? minViewValue : value;
    return value;
}

void vTaskDelay(int delay_ms, string msg) {
    console << msg;
    vTaskDelay(10);
}

bool isValidString(const String& s) {
    return isValidString(std::string(s.c_str()));
}

bool isValidString(const std::string& s) {
    return !isBlank(s) && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isprint(c); });
}

bool isBlank(const String& s) {
    return isBlank(std::string(s.c_str()));
}

bool isBlank(const std::string& s) {
    return s.empty() || std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
}

std::string stringToHex(const std::string& input, int radix) {
    if (input.empty()) return "";
    
    std::string result;
    int charsPerUnit;
    std::string separator;
    
    // Determine format based on radix
    switch (radix) {
        case 4:   charsPerUnit = 1; separator = " "; break;  // 1 hex char per nibble
        case 8:   charsPerUnit = 2; separator = " "; break;  // 2 hex chars per byte
        case 16:  charsPerUnit = 4; separator = " "; break;  // 4 hex chars per word
        default:  charsPerUnit = 2; separator = " "; break;  // default to byte
    }
    
    int unitsPerLine = 8 / charsPerUnit;  // 8 output units per line
    int unitCount = 0;
    
    for (size_t i = 0; i < input.length(); i++) {
        unsigned char byte = input[i];
        
        if (radix == 4) {
            // 4-bit: two nibbles per byte
            result += fmt::format("{:x} ", (byte >> 4) & 0x0F);
            unitCount++;
            if (unitCount >= unitsPerLine && i < input.length() - 1) {
                result += "\n";
                unitCount = 0;
            }
            result += fmt::format("{:x} ", byte & 0x0F);
            unitCount++;
        } else if (radix == 8) {
            // 8-bit: full byte as hex
            result += fmt::format("{:02x} ", byte);
            unitCount++;
        } else if (radix == 16) {
            // 16-bit: two bytes as word (need to handle pairs)
            if (i % 2 == 0) {
                if (i + 1 < input.length()) {
                    unsigned char nextByte = input[i + 1];
                    result += fmt::format("{:02x}{:02x} ", byte, nextByte);
                } else {
                    result += fmt::format("{:02x}00 ", byte);  // pad with 00 if odd byte
                }
            } else {
                continue;  // skip, already processed as part of word
            }
            unitCount++;
        }
        
        if (unitCount >= unitsPerLine && i < input.length() - 1) {
            result += "\n";
            unitCount = 0;
        }
    }
    
    // Remove trailing whitespace/newline
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

std::string stringToHex(const String& input, int radix) {
    return stringToHex(std::string(input.c_str()), radix);
}

