#pragma once
#include <Arduino.h>
#include "config.h"

enum class Press : uint8_t { None, Ok, Snooze, Portal, Reset };
enum class AlarmResult : uint8_t { Drank, Snoozed, Ignored, Portal, Reset };

void hwBegin();

// Buttons
bool okDown();
bool snoozeDown();
void waitRelease(uint32_t timeoutMs);
// Follows the current press until release (or until the portal/reset threshold).
// sawOk/sawSnooze: buttons already seen, e.g. the one that woke the chip.
Press readGesture(bool sawOk = false, bool sawSnooze = false);

// Full alarm: vibration, rising beeps, until a button press or the timeout.
AlarmResult runAlarm();

uint16_t batteryMv();

// Actuators and short feedback signals
void buzzer(uint16_t duty, uint32_t freq = BUZZER_FREQ);
void motor(bool on);
void allOff();
void fbBoot();
void fbAck();
void fbSnooze();
void fbGoal();
void fbLowBatt();
void fbError();
void fbPortal();
void fbReset();

// Deep sleep until the next alarm (timer) or a button press.
[[noreturn]] void deepSleep(time_t seconds);
// Deep sleep without timer: only a button wakes it up.
[[noreturn]] void sleepUntilButton();
