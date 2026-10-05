#pragma once
#include <stdint.h>
#include <time.h>

// Day state in RTC memory: survives deep sleep and software resets, but not a
// battery disconnect (the magic no longer matches and everything starts over).
// Keeping it here avoids writing flash at every wake-up.
struct RtcState {
  uint32_t magic;
  time_t   dayStart;
  time_t   nextAlarm;
  time_t   nextSync;
  uint8_t  drunk;
  uint8_t  goal;
  uint8_t  retries;
  uint8_t  prevDrunk;  // yesterday, shown in the portal
  uint8_t  prevGoal;
};

constexpr uint32_t RTC_MAGIC = 0xD71C0000u | sizeof(RtcState);

extern RtcState rtc;

inline bool timeValid() { return time(nullptr) > 1735689600; }  // after 2025-01-01
