#pragma once
#include <Arduino.h>
#include "schedule.h"

// Seeed XIAO ESP32C3 pins. No strapping pins (2, 8, 9).
// Buttons must be on GPIO0-5: the only ones that can wake the C3 from deep sleep.
constexpr uint8_t PIN_BTN_OK     = 3;  // D1, to GND, external 100k pull-up
constexpr uint8_t PIN_BTN_SNOOZE = 5;  // D3, to GND, external 100k pull-up
constexpr uint8_t PIN_VBAT       = 4;  // D2, 1M/1M divider from the battery
constexpr uint8_t PIN_BUZZER     = 6;  // D4, NPN base through 1k
constexpr uint8_t PIN_MOTOR      = 7;  // D5, NPN base through 1k

constexpr const char* TZ_INFO = sched::TZ_ITALY;
constexpr const char* NTP_SERVER1 = "pool.ntp.org";
constexpr const char* NTP_SERVER2 = "time.google.com";
constexpr const char* AP_SSID_PREFIX = "Drinkuino-";
constexpr const char* AP_PASSWORD = "bevimi123";  // WPA2, at least 8 characters

// Alarm: vibration first, then beeps with rising volume
constexpr uint32_t BUZZER_FREQ     = 2700;  // set to your buzzer's resonant frequency (often 2048 or 2700 Hz)
constexpr uint16_t BUZZER_DUTY_MIN = 8;     // out of 1023
constexpr uint16_t BUZZER_DUTY_MAX = 512;   // 50% = loudest
constexpr uint32_t ALARM_SILENT_MS = 15000; // vibration only
constexpr uint32_t ALARM_RAMP_MS   = 75000; // from quiet to loudest
constexpr uint32_t ALARM_MAX_MS    = 120000;
constexpr uint8_t  ALARM_RETRIES   = 3;     // attempts when nobody answers
constexpr time_t   RETRY_S         = 10 * 60;

// Buttons
constexpr uint32_t RESET_HOLD_MS  = 10000;  // both: factory reset
constexpr uint32_t PORTAL_HOLD_MS = 5000;   // snooze only: reopen the portal keeping the settings

// Network
constexpr uint32_t PORTAL_TIMEOUT_MS = 10 * 60 * 1000;
constexpr uint32_t WIFI_TIMEOUT_MS   = 10000;
constexpr uint32_t NTP_TIMEOUT_MS    = 5000;
constexpr time_t   SYNC_EVERY_S      = 20 * 3600;
constexpr time_t   SYNC_RETRY_S      = 3 * 3600;  // without WiFi, don't retry at every reminder

// Battery
constexpr uint8_t  VBAT_DIVIDER = 2;
constexpr uint16_t VBAT_LOW_MV  = 3500;

#ifdef DRINK_DEBUG
#define LOG(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...) do {} while (0)
#endif
