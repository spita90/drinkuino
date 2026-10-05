#pragma once
#include <stdint.h>
#include <time.h>

// Reminder logic, plain C++ (no Arduino): also compiled by the native tests.

struct Profile {
  uint16_t weightKg;
  uint16_t heightCm;    // 0 = not given
  uint16_t wakeMin;     // wake-up time, minutes after midnight
  uint16_t sleepMin;    // bedtime
  uint16_t glassMl;
  uint16_t overrideMl;  // doctor's target, 0 = use the formula
  uint8_t  age;
};

// A "day" runs from wake-up to bedtime; the night belongs to the previous day.
struct Day {
  time_t start;
  time_t end;
};

namespace sched {

constexpr char     TZ_ITALY[]     = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr uint16_t MIN_TARGET_ML  = 2500;     // floor for kidney stone formers (EAU guidelines)
constexpr uint16_t MAX_TARGET_ML  = 4500;     // cap on the formula, not on the doctor's target
constexpr uint8_t  MAX_GLASSES    = 40;
constexpr time_t   FIRST_OFFSET_S = 15 * 60;  // first reminder after wake-up
constexpr time_t   MIN_INTERVAL_S = 20 * 60;
constexpr time_t   SNOOZE_S       = 10 * 60;
constexpr time_t   EARLY_CANCEL_S = 30 * 60;  // "drank" outside an alarm cancels the next one if it is within 30 min

uint16_t targetMl(const Profile& p);
uint8_t  glasses(const Profile& p);

Day dayFor(time_t now, const Profile& p);
Day nextDay(const Day& d, const Profile& p);

// Next reminder: time left until bedtime divided by the glasses still to drink.
time_t nextReminder(time_t now, const Day& d, const Profile& p, uint8_t drunk);

// "Drank" pressed with no alarm running (drunk already includes this glass).
time_t afterEarlyDrink(time_t now, time_t scheduled, const Day& d, const Profile& p, uint8_t drunk);

}  // namespace sched
