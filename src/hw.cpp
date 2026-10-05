#include "hw.h"
#include <esp_sleep.h>

// Arduino core 3.x addresses LEDC by pin, 2.x by channel.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
#define BUZ PIN_BUZZER
#else
#define BUZ 0
#endif

static uint32_t curFreq = 0;

void hwBegin() {
  // internal pull-ups back up the external ones while awake; in deep sleep only the external ones remain
  pinMode(PIN_BTN_OK, INPUT_PULLUP);
  pinMode(PIN_BTN_SNOOZE, INPUT_PULLUP);
  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BUZZER, BUZZER_FREQ, 10);
#else
  ledcSetup(BUZ, BUZZER_FREQ, 10);
  ledcAttachPin(PIN_BUZZER, BUZ);
#endif
  curFreq = BUZZER_FREQ;
  ledcWrite(BUZ, 0);
}

// ---------------------------------------------------------------- buttons

bool okDown() { return digitalRead(PIN_BTN_OK) == LOW; }
bool snoozeDown() { return digitalRead(PIN_BTN_SNOOZE) == LOW; }

void waitRelease(uint32_t timeoutMs) {
  const uint32_t t0 = millis();
  while ((okDown() || snoozeDown()) && millis() - t0 < timeoutMs) delay(10);
  delay(30);
}

Press readGesture(bool sawOk, bool sawSnooze) {
  const uint32_t t0 = millis();
  uint32_t bothSince = 0, releasedSince = 0;
  bool sawBoth = false;
  for (;;) {
    const uint32_t now = millis();
    const bool o = okDown(), s = snoozeDown();
    if (!o && !s) {
      if (!releasedSince) releasedSince = now;
      if (now - releasedSince >= 40) break;  // debounce
      delay(5);
      continue;
    }
    releasedSince = 0;
    sawOk |= o;
    sawSnooze |= s;
    if (o && s) {
      if (!sawBoth || !bothSince) bothSince = now;
      sawBoth = true;
      if (now - bothSince >= RESET_HOLD_MS) return Press::Reset;
    } else {
      bothSince = 0;
      if (s && !sawOk && now - t0 >= PORTAL_HOLD_MS) return Press::Portal;
    }
    delay(10);
  }
  if (sawBoth) return Press::None;  // reset aborted
  if (sawOk) return Press::Ok;
  return sawSnooze ? Press::Snooze : Press::None;
}

// ---------------------------------------------------------------- alarm

AlarmResult runAlarm() {
  const uint32_t t0 = millis();
  for (uint32_t t = 0; t < ALARM_MAX_MS; t = millis() - t0) {
    const uint32_t c = t % 5000;  // vibration: 3 pulses of 300 ms every 5 s
    motor(c < 1500 && (c / 300) % 2 == 0);
    if (t >= ALARM_SILENT_MS) {
      float x = float(t - ALARM_SILENT_MS) / ALARM_RAMP_MS;
      if (x > 1.0f) x = 1.0f;
      // quadratic curve: the rise sounds more even to the ear
      const uint16_t duty = BUZZER_DUTY_MIN + uint16_t((BUZZER_DUTY_MAX - BUZZER_DUTY_MIN) * x * x);
      const uint32_t period = x >= 1.0f ? 500 : 1000;
      buzzer(t % period < 200 ? duty : 0);
    }
    if (okDown() || snoozeDown()) {
      allOff();
      switch (readGesture()) {
        case Press::Ok:     return AlarmResult::Drank;
        case Press::Snooze: return AlarmResult::Snoozed;
        case Press::Portal: return AlarmResult::Portal;
        case Press::Reset:  return AlarmResult::Reset;
        case Press::None:   break;  // reset aborted: keep ringing
      }
    }
    delay(10);
  }
  allOff();
  return AlarmResult::Ignored;
}

// ---------------------------------------------------------------- battery

uint16_t batteryMv() {
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) sum += analogReadMilliVolts(PIN_VBAT);
  return sum / 8 * VBAT_DIVIDER;
}

// ---------------------------------------------------------------- actuators

void buzzer(uint16_t duty, uint32_t freq) {
  if (duty && freq != curFreq) {
    ledcChangeFrequency(BUZ, freq, 10);
    curFreq = freq;
  }
  ledcWrite(BUZ, duty);
}

void motor(bool on) { digitalWrite(PIN_MOTOR, on ? HIGH : LOW); }

void allOff() {
  buzzer(0);
  motor(false);
}

static void beep(uint16_t ms, uint32_t freq, uint16_t duty = 200) {
  buzzer(duty, freq);
  delay(ms);
  buzzer(0);
}

void fbBoot()    { beep(60, 3200); }
void fbAck()     { motor(true); beep(90, 3200); motor(false); }
void fbSnooze()  { beep(60, 2400); delay(90); beep(60, 2400); }
void fbGoal()    { delay(150); beep(90, 2400); beep(90, 2700); beep(180, 3200); }
void fbLowBatt() { for (int i = 0; i < 3; i++) { beep(150, 1800); delay(120); } }
void fbError()   { beep(250, 1600); delay(100); beep(250, 1300); }
void fbPortal()  { beep(100, 2200); delay(60); beep(100, 3200); }
void fbReset()   { beep(700, 2000); }

// ---------------------------------------------------------------- sleep

// A button still held down would wake the chip immediately: leave it out of the wake-up mask.
static bool armButtonWake() {
  waitRelease(3000);
  uint64_t mask = 0;
  if (!okDown()) mask |= 1ULL << PIN_BTN_OK;
  if (!snoozeDown()) mask |= 1ULL << PIN_BTN_SNOOZE;
  if (mask) esp_deep_sleep_enable_gpio_wakeup(mask, ESP_GPIO_WAKEUP_GPIO_LOW);
  return mask != 0;
}

void deepSleep(time_t seconds) {
  allOff();
  armButtonWake();
  if (seconds < 1) seconds = 1;
  esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
  LOG("sleep %lds\n", (long)seconds);
#ifdef DRINK_DEBUG
  Serial.flush();
#endif
  esp_deep_sleep_start();
}

void sleepUntilButton() {
  allOff();
  if (!armButtonWake()) esp_sleep_enable_timer_wakeup(3600ULL * 1000000ULL);  // stuck button: try again in an hour
  esp_deep_sleep_start();
}
