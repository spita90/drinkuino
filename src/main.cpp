// Drinkuino - hydration reminder. The ESP32-C3 stays in deep sleep between events.
// Every wake-up (timer or button) runs setup() from the top and goes back to sleep:
// loop() is never reached.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_sntp.h>
#include "config.h"
#include "hw.h"
#include "portal.h"
#include "schedule.h"
#include "settings.h"
#include "state.h"

RTC_NOINIT_ATTR RtcState rtc;
static Settings cfg;
static volatile bool ntpDone = false;

static void onNtp(struct timeval*) { ntpDone = true; }

static bool syncTime() {
  if (cfg.ssid.isEmpty()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) delay(50);
  bool ok = false;
  if (WiFi.status() == WL_CONNECTED) {
    ntpDone = false;
    sntp_set_time_sync_notification_cb(onNtp);
    configTzTime(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
    t0 = millis();
    while (!ntpDone && millis() - t0 < NTP_TIMEOUT_MS) delay(20);
    ok = ntpDone;
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  LOG("NTP %s\n", ok ? "ok" : "failed");
  return ok;
}

static void maybeSync() {
  const time_t now = time(nullptr);
  if (cfg.ssid.isEmpty() || now < rtc.nextSync) return;
  rtc.nextSync = now + (syncTime() ? SYNC_EVERY_S : SYNC_RETRY_S);
}

// New day: reset the count and keep yesterday's for the portal.
static void rollDay(time_t now) {
  const Profile& p = cfg.profile;
  const Day d = sched::dayFor(now, p);
  if (rtc.dayStart != d.start) {
    if (rtc.dayStart) {
      rtc.prevDrunk = rtc.drunk;
      rtc.prevGoal = rtc.goal;
    }
    rtc.dayStart = d.start;
    rtc.drunk = 0;
    rtc.retries = 0;
  }
  rtc.goal = sched::glasses(p);
  const time_t latest = sched::nextDay(d, p).start + sched::FIRST_OFFSET_S;
  if (rtc.nextAlarm < d.start || rtc.nextAlarm > latest)
    rtc.nextAlarm = sched::nextReminder(now, d, p, rtc.drunk);
}

static void factoryReset() {
  fbReset();
  clearSettings();
  rtc.magic = 0;
  ESP.restart();
}

static void drank(time_t now, const Day& d, bool early) {
  rtc.drunk++;
  rtc.retries = 0;
  fbAck();
  if (rtc.drunk == rtc.goal) fbGoal();
  rtc.nextAlarm = early ? sched::afterEarlyDrink(now, rtc.nextAlarm, d, cfg.profile, rtc.drunk)
                        : sched::nextReminder(now, d, cfg.profile, rtc.drunk);
}

static void handleAlarm() {
  const bool low = batteryMv() < VBAT_LOW_MV;  // measured at rest, before buzzer and motor load the battery
  const AlarmResult r = runAlarm();
  const time_t now = time(nullptr);
  const Day d = sched::dayFor(now, cfg.profile);
  switch (r) {
    case AlarmResult::Drank:
      drank(now, d, false);
      break;
    case AlarmResult::Snoozed:
      rtc.retries = 0;
      rtc.nextAlarm = now + sched::SNOOZE_S;
      fbSnooze();
      break;
    case AlarmResult::Ignored:
      // retry in 10 minutes, but never past bedtime
      if (++rtc.retries < ALARM_RETRIES && now + RETRY_S < d.end) {
        rtc.nextAlarm = now + RETRY_S;
      } else {
        rtc.retries = 0;
        rtc.nextAlarm = sched::nextReminder(now, d, cfg.profile, rtc.drunk);
      }
      break;
    case AlarmResult::Portal:
      fbPortal();
      runPortal(cfg);
    case AlarmResult::Reset:
      factoryReset();
      break;
  }
  if (low) fbLowBatt();
  LOG("alarm: result %d, drunk %u/%u\n", (int)r, rtc.drunk, rtc.goal);
}

// Woken by a button, no alarm running
static void handleButtons() {
  const uint64_t woke = esp_sleep_get_gpio_wakeup_status();
  const Press pr = readGesture(woke & (1ULL << PIN_BTN_OK), woke & (1ULL << PIN_BTN_SNOOZE));
  const time_t now = time(nullptr);
  const Day d = sched::dayFor(now, cfg.profile);
  switch (pr) {
    case Press::Ok:
      drank(now, d, true);
      break;
    case Press::Portal:
      runPortal(cfg);
    case Press::Reset:
      factoryReset();
      break;
    case Press::Snooze:  // snooze only acts on a ringing alarm
    case Press::None:
      break;
  }
  LOG("button: %d, drunk %u/%u\n", (int)pr, rtc.drunk, rtc.goal);
}

void setup() {
  setCpuFrequencyMhz(80);  // lowest clock that still supports WiFi
  hwBegin();
#ifdef DRINK_DEBUG
  Serial.begin(115200);
#endif
  setenv("TZ", TZ_INFO, 1);
  tzset();

  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (rtc.magic != RTC_MAGIC) {  // first power-on or battery disconnected
    memset(&rtc, 0, sizeof rtc);
    rtc.magic = RTC_MAGIC;
  }

  if (!loadSettings(cfg)) runPortal(cfg);
  if (!timeValid()) {
    // time lost: try WiFi first, otherwise get it from the phone through the portal
    if (!syncTime()) runPortal(cfg);
    rtc.nextSync = time(nullptr) + SYNC_EVERY_S;
  }

  rollDay(time(nullptr));

  switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
      if (rtc.nextAlarm <= time(nullptr) + 2) handleAlarm();
      break;
    case ESP_SLEEP_WAKEUP_GPIO:
      handleButtons();
      break;
    default:  // power-on or restart after the portal: signal if WiFi does not work
      fbBoot();
      if (!cfg.ssid.isEmpty() && rtc.nextSync == 0) {
        const bool ok = syncTime();
        rtc.nextSync = time(nullptr) + (ok ? SYNC_EVERY_S : SYNC_RETRY_S);
        if (!ok) fbError();
      }
      break;
  }

  maybeSync();
  // a button press or the sync may have made the alarm due
  if (rtc.nextAlarm <= time(nullptr) + 1) handleAlarm();

  const time_t now = time(nullptr);
  LOG("next reminder in %ld min\n", (long)(rtc.nextAlarm - now) / 60);
  deepSleep(rtc.nextAlarm - now);
}

void loop() {}
