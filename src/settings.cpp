#include "settings.h"
#include <Preferences.h>

static const char NS[] = "drink";

bool loadSettings(Settings& s) {
  Preferences p;
  if (!p.begin(NS, true)) return false;
  bool ok = p.getBytesLength("prof") == sizeof s.profile &&
            p.getBytes("prof", &s.profile, sizeof s.profile) == sizeof s.profile;
  s.ssid = p.getString("ssid", "");
  s.pass = p.getString("pass", "");
  p.end();
  return ok;
}

void saveSettings(const Settings& s) {
  Preferences p;
  p.begin(NS, false);
  p.putBytes("prof", &s.profile, sizeof s.profile);
  p.putString("ssid", s.ssid);
  p.putString("pass", s.pass);
  p.end();
}

void clearSettings() {
  Preferences p;
  p.begin(NS, false);
  p.clear();
  p.end();
}
