#include "portal.h"
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <sys/time.h>
#include "config.h"
#include "hw.h"
#include "page.h"
#include "state.h"

static String jsonEscape(const String& s) {
  String o;
  o.reserve(s.length() + 4);
  for (size_t i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '"' || c == '\\') o += '\\';
    o += (uint8_t)c < 0x20 ? ' ' : c;
  }
  return o;
}

// "HH:MM" -> minutes after midnight, -1 if invalid
static int parseHM(const String& s) {
  int h, m;
  if (sscanf(s.c_str(), "%d:%d", &h, &m) != 2 || h < 0 || h > 23 || m < 0 || m > 59) return -1;
  return h * 60 + m;
}

void runPortal(Settings& cfg) {
  allOff();
  const bool configured = cfg.profile.weightKg > 0;

  // Nearby networks become suggestions for the SSID field
  WiFi.mode(WIFI_AP_STA);
  String nets;
  const int n = WiFi.scanNetworks();
  for (int i = 0; i < n && i < 12; i++) {
    const String s = WiFi.SSID(i);
    if (s.isEmpty()) continue;
    if (nets.length()) nets += ',';
    nets += '"';
    nets += jsonEscape(s);
    nets += '"';
  }
  WiFi.scanDelete();

  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  char apName[24];
  snprintf(apName, sizeof apName, "%s%02X%02X", AP_SSID_PREFIX, mac[4], mac[5]);
  WiFi.softAP(apName, AP_PASSWORD);
  delay(100);
  const IPAddress ip = WiFi.softAPIP();
  LOG("portal: %s on %s\n", apName, ip.toString().c_str());

  DNSServer dns;
  dns.start(53, "*", ip);  // every domain resolves here: the phone opens the page by itself
  WebServer server(80);
  bool saved = false;
  uint32_t lastActivity = millis();

  server.on("/", HTTP_GET, [&] {
    lastActivity = millis();
    server.send_P(200, "text/html; charset=utf-8", PAGE);
  });

  server.on("/cfg.json", HTTP_GET, [&] {
    lastActivity = millis();
    const Profile& p = cfg.profile;
    char buf[256];
    snprintf(buf, sizeof buf,
             "{\"configured\":%s,\"w\":%u,\"h\":%u,\"age\":%u,\"wake\":\"%02u:%02u\",\"sleep\":\"%02u:%02u\","
             "\"glass\":%u,\"ovr\":%u,\"vbat\":%u,\"today\":%u,\"goal\":%u,\"yday\":%u,\"ygoal\":%u,\"ssid\":\"",
             configured ? "true" : "false", (unsigned)p.weightKg, (unsigned)p.heightCm, (unsigned)p.age,
             (unsigned)(p.wakeMin / 60), (unsigned)(p.wakeMin % 60), (unsigned)(p.sleepMin / 60),
             (unsigned)(p.sleepMin % 60), (unsigned)p.glassMl, (unsigned)p.overrideMl, (unsigned)batteryMv(),
             (unsigned)rtc.drunk, (unsigned)rtc.goal, (unsigned)rtc.prevDrunk, (unsigned)rtc.prevGoal);
    String out = buf;
    out += jsonEscape(cfg.ssid);
    out += "\",\"nets\":[";
    out += nets;
    out += "]}";
    server.send(200, "application/json", out);
  });

  server.on("/save", HTTP_POST, [&] {
    lastActivity = millis();
    const int wake = parseHM(server.arg("wake")), sleep = parseHM(server.arg("sleep"));
    const long w = server.arg("w").toInt(), h = server.arg("h").toInt(), age = server.arg("age").toInt(),
               glass = server.arg("glass").toInt(), ovr = server.arg("ovr").toInt();
    if (w < 30 || w > 250 || (h && (h < 100 || h > 230)) || age < 1 || age > 120 || wake < 0 || sleep < 0 ||
        wake == sleep || glass < 100 || glass > 1000 || (ovr && (ovr < 1000 || ovr > 6000))) {
      server.send(400, "text/plain; charset=utf-8",
                  server.arg("l") == "it" ? "Dati non validi: torna indietro e controlla i campi."
                                          : "Invalid data: go back and check the fields.");
      return;
    }

    // The time comes from the phone: works even without WiFi
    const uint64_t ms = strtoull(server.arg("epoch").c_str(), nullptr, 10);
    if (ms > 1735689600000ULL) {
      timeval tv;
      tv.tv_sec = ms / 1000;
      tv.tv_usec = (ms % 1000) * 1000;
      settimeofday(&tv, nullptr);
    }

    Profile p{};
    p.weightKg = w;
    p.heightCm = h;
    p.age = age;
    p.wakeMin = wake;
    p.sleepMin = sleep;
    p.glassMl = glass;
    p.overrideMl = ovr;

    // If only the profile changes, today's count stays valid
    if (timeValid()) {
      const time_t now = time(nullptr);
      if (configured && rtc.dayStart == sched::dayFor(now, cfg.profile).start)
        rtc.dayStart = sched::dayFor(now, p).start;
      rtc.nextAlarm = 0;  // recomputed after the restart
      rtc.nextSync = 0;
    }

    String ssid = server.arg("ssid");
    ssid.trim();
    String pass = server.arg("pass");
    if (pass.isEmpty() && ssid == cfg.ssid) pass = cfg.pass;
    cfg.ssid = ssid;
    cfg.pass = pass;
    cfg.profile = p;
    saveSettings(cfg);

    server.send_P(200, "text/html; charset=utf-8", SAVED_PAGE);
    saved = true;
  });

  server.onNotFound([&] {
    server.sendHeader("Location", String("http://") + ip.toString() + "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  fbPortal();

  while (!saved && millis() - lastActivity < PORTAL_TIMEOUT_MS) {
    dns.processNextRequest();
    server.handleClient();
    delay(2);
  }
  // let the response reach the phone before shutting down the AP
  for (const uint32_t t0 = millis(); saved && millis() - t0 < 1500;) {
    server.handleClient();
    delay(2);
  }
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  if (saved || (configured && timeValid())) ESP.restart();
  LOG("portal timed out without configuration\n");
  sleepUntilButton();
}
