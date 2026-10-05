#include "schedule.h"

namespace sched {

uint16_t targetMl(const Profile& p) {
  if (p.overrideMl) return p.overrideMl;
  float w = p.weightKg;
  if (p.heightCm >= 100) {
    float h2 = (p.heightCm / 100.0f) * (p.heightCm / 100.0f);
    float ref = 25.0f * h2;                          // weight at BMI 25
    if (w / h2 > 30.0f) w = ref + 0.4f * (w - ref);  // adjusted weight if BMI > 30
  }
  float ml = w * (p.age > 55 ? 30 : 35);
  if (ml > MAX_TARGET_ML) ml = MAX_TARGET_ML;
  if (ml < MIN_TARGET_ML) ml = MIN_TARGET_ML;
  return (uint16_t)(ml + 0.5f);
}

uint8_t glasses(const Profile& p) {
  uint16_t g = p.glassMl ? p.glassMl : 250;
  uint16_t n = (targetMl(p) + g - 1) / g;
  return n > MAX_GLASSES ? MAX_GLASSES : (uint8_t)n;
}

// Minute of the day in local time, on the day of ref + dayOffset. mktime handles DST changes.
static time_t atLocal(time_t ref, int dayOffset, int minuteOfDay) {
  struct tm t;
  localtime_r(&ref, &t);
  t.tm_mday += dayOffset;
  t.tm_hour = 0;
  t.tm_min = minuteOfDay;
  t.tm_sec = 0;
  t.tm_isdst = -1;
  return mktime(&t);
}

static Day window(time_t start, const Profile& p) {
  int len = (int)p.sleepMin - (int)p.wakeMin;
  if (len <= 0) len += 24 * 60;  // bedtime after midnight
  return {start, atLocal(start, 0, p.wakeMin + len)};
}

Day dayFor(time_t now, const Profile& p) {
  time_t s = atLocal(now, 0, p.wakeMin);
  if (now < s) s = atLocal(now, -1, p.wakeMin);
  return window(s, p);
}

Day nextDay(const Day& d, const Profile& p) {
  return window(atLocal(d.start, 1, p.wakeMin), p);
}

time_t nextReminder(time_t now, const Day& d, const Profile& p, uint8_t drunk) {
  const uint8_t goal = glasses(p);
  const time_t tomorrow = nextDay(d, p).start + FIRST_OFFSET_S;
  if (now >= d.end) return tomorrow;
  if (drunk >= goal) {
    // goal reached: only the bedtime glass is left
    if (drunk == goal && now + MIN_INTERVAL_S <= d.end) return d.end;
    return tomorrow;
  }
  const time_t first = d.start + FIRST_OFFSET_S;
  if (drunk == 0 && now < first) return first;
  time_t interval = (d.end - now) / (goal - drunk);
  if (interval < MIN_INTERVAL_S) interval = MIN_INTERVAL_S;
  const time_t t = now + interval;
  return t < d.end ? t : d.end;
}

time_t afterEarlyDrink(time_t now, time_t scheduled, const Day& d, const Profile& p, uint8_t drunk) {
  if (drunk >= glasses(p) || scheduled <= now + EARLY_CANCEL_S) return nextReminder(now, d, p, drunk);
  return scheduled;
}

}  // namespace sched
