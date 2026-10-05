#include <stdlib.h>
#include <time.h>
#include <unity.h>
#include "schedule.h"

using namespace sched;

static Profile prof(uint16_t w = 70, uint8_t age = 40, uint16_t h = 175) {
  Profile p{};
  p.weightKg = w;
  p.heightCm = h;
  p.age = age;
  p.wakeMin = 7 * 60;
  p.sleepMin = 23 * 60;
  p.glassMl = 250;
  return p;
}

static time_t at(int y, int mo, int d, int h, int mi) {
  struct tm t {};
  t.tm_year = y - 1900;
  t.tm_mon = mo - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = mi;
  t.tm_isdst = -1;
  return mktime(&t);
}

void setUp() {}
void tearDown() {}

// ------------------------------------------------------------ daily target

void test_target_floor() { TEST_ASSERT_EQUAL_UINT16(2500, targetMl(prof(60))); }  // 60 * 35 = 2100
void test_target_formula() { TEST_ASSERT_EQUAL_UINT16(3150, targetMl(prof(90))); }
void test_target_over_55() { TEST_ASSERT_EQUAL_UINT16(2700, targetMl(prof(90, 60))); }
void test_target_bmi_adjusted() { TEST_ASSERT_EQUAL_UINT16(3197, targetMl(prof(120, 40, 170))); }
void test_target_cap() { TEST_ASSERT_EQUAL_UINT16(MAX_TARGET_ML, targetMl(prof(200, 40, 0))); }

void test_target_override_wins() {
  Profile p = prof(90);
  p.overrideMl = 2000;
  TEST_ASSERT_EQUAL_UINT16(2000, targetMl(p));
}

void test_glasses_round_up() { TEST_ASSERT_EQUAL_UINT8(13, glasses(prof(90))); }  // 3150 / 250 = 12.6

// ------------------------------------------------------------ day window

void test_night_belongs_to_previous_day() {
  Day d = dayFor(at(2026, 10, 5, 6, 0), prof());
  TEST_ASSERT_EQUAL(at(2026, 10, 4, 7, 0), d.start);
  TEST_ASSERT_EQUAL(at(2026, 10, 4, 23, 0), d.end);
}

void test_day_after_wake() {
  Day d = dayFor(at(2026, 10, 5, 8, 0), prof());
  TEST_ASSERT_EQUAL(at(2026, 10, 5, 7, 0), d.start);
}

void test_sleep_after_midnight() {
  Profile p = prof();
  p.wakeMin = 10 * 60;
  p.sleepMin = 2 * 60;
  Day d = dayFor(at(2026, 10, 6, 1, 0), p);
  TEST_ASSERT_EQUAL(at(2026, 10, 5, 10, 0), d.start);
  TEST_ASSERT_EQUAL(at(2026, 10, 6, 2, 0), d.end);
}

void test_dst_spring_forward() {
  Day d = dayFor(at(2026, 3, 28, 12, 0), prof());
  Day n = nextDay(d, prof());
  TEST_ASSERT_EQUAL(at(2026, 3, 29, 7, 0), n.start);
  TEST_ASSERT_EQUAL(23 * 3600, n.start - d.start);  // 23-hour night
  TEST_ASSERT_EQUAL(16 * 3600, n.end - n.start);
}

void test_dst_fall_back() {
  Day d = dayFor(at(2026, 10, 24, 12, 0), prof());
  TEST_ASSERT_EQUAL(25 * 3600, nextDay(d, prof()).start - d.start);
}

// ------------------------------------------------------------ reminders

static const Profile P = prof();  // 2500 ml = 10 glasses, 07:00-23:00

void test_first_reminder_after_wake() {
  Day d = dayFor(at(2026, 10, 5, 7, 0), P);
  TEST_ASSERT_EQUAL(at(2026, 10, 5, 7, 15), nextReminder(at(2026, 10, 5, 7, 0), d, P, 0));
}

void test_on_time_drinking_is_evenly_spaced() {
  Day d = dayFor(at(2026, 10, 5, 7, 0), P);
  time_t t = at(2026, 10, 5, 7, 15);
  for (uint8_t k = 1; k < 10; k++) {
    t = nextReminder(t, d, P, k);
    TEST_ASSERT_EQUAL(at(2026, 10, 5, 7, 15) + k * 105 * 60, t);  // (23:00 - 07:15) / 9 = 105 min
  }
  TEST_ASSERT_EQUAL(d.end, t);
  TEST_ASSERT_EQUAL(at(2026, 10, 6, 7, 15), nextReminder(t, d, P, 10));
}

void test_goal_reached_early_leaves_bedtime_glass() {
  Day d = dayFor(at(2026, 10, 5, 15, 0), P);
  TEST_ASSERT_EQUAL(d.end, nextReminder(at(2026, 10, 5, 15, 0), d, P, 10));
  TEST_ASSERT_EQUAL(at(2026, 10, 6, 7, 15), nextReminder(at(2026, 10, 5, 15, 0), d, P, 11));
}

void test_behind_schedule_keeps_min_interval() {
  Day d = dayFor(at(2026, 10, 5, 22, 0), P);
  TEST_ASSERT_EQUAL(at(2026, 10, 5, 22, 20), nextReminder(at(2026, 10, 5, 22, 0), d, P, 2));
}

void test_last_reminder_capped_at_bedtime() {
  Day d = dayFor(at(2026, 10, 5, 22, 50), P);
  TEST_ASSERT_EQUAL(d.end, nextReminder(at(2026, 10, 5, 22, 50), d, P, 5));
}

void test_after_bedtime_goes_to_tomorrow() {
  Day d = dayFor(at(2026, 10, 5, 23, 30), P);
  TEST_ASSERT_EQUAL(at(2026, 10, 6, 7, 15), nextReminder(at(2026, 10, 5, 23, 30), d, P, 5));
}

void test_early_drink_cancels_reminder_within_30_min() {
  Day d = dayFor(at(2026, 10, 5, 9, 40), P);
  time_t now = at(2026, 10, 5, 9, 40), scheduled = at(2026, 10, 5, 10, 0);
  time_t next = afterEarlyDrink(now, scheduled, d, P, 3);
  TEST_ASSERT_EQUAL(nextReminder(now, d, P, 3), next);
  TEST_ASSERT_TRUE(next > scheduled);
}

void test_early_drink_keeps_far_reminder() {
  Day d = dayFor(at(2026, 10, 5, 9, 0), P);
  time_t scheduled = at(2026, 10, 5, 10, 0);
  TEST_ASSERT_EQUAL(scheduled, afterEarlyDrink(at(2026, 10, 5, 9, 0), scheduled, d, P, 3));
}

void test_early_drink_reaching_goal_reschedules() {
  Day d = dayFor(at(2026, 10, 5, 15, 0), P);
  TEST_ASSERT_EQUAL(d.end, afterEarlyDrink(at(2026, 10, 5, 15, 0), at(2026, 10, 5, 17, 0), d, P, 10));
}

int main() {
  setenv("TZ", TZ_ITALY, 1);
  tzset();
  UNITY_BEGIN();
  RUN_TEST(test_target_floor);
  RUN_TEST(test_target_formula);
  RUN_TEST(test_target_over_55);
  RUN_TEST(test_target_bmi_adjusted);
  RUN_TEST(test_target_cap);
  RUN_TEST(test_target_override_wins);
  RUN_TEST(test_glasses_round_up);
  RUN_TEST(test_night_belongs_to_previous_day);
  RUN_TEST(test_day_after_wake);
  RUN_TEST(test_sleep_after_midnight);
  RUN_TEST(test_dst_spring_forward);
  RUN_TEST(test_dst_fall_back);
  RUN_TEST(test_first_reminder_after_wake);
  RUN_TEST(test_on_time_drinking_is_evenly_spaced);
  RUN_TEST(test_goal_reached_early_leaves_bedtime_glass);
  RUN_TEST(test_behind_schedule_keeps_min_interval);
  RUN_TEST(test_last_reminder_capped_at_bedtime);
  RUN_TEST(test_after_bedtime_goes_to_tomorrow);
  RUN_TEST(test_early_drink_cancels_reminder_within_30_min);
  RUN_TEST(test_early_drink_keeps_far_reminder);
  RUN_TEST(test_early_drink_reaching_goal_reschedules);
  return UNITY_END();
}
