#pragma once
#include <Arduino.h>
#include "schedule.h"

// Configuration in NVS (flash): written only by the portal.
struct Settings {
  String ssid;
  String pass;
  Profile profile{};
};

bool loadSettings(Settings& s);
void saveSettings(const Settings& s);
void clearSettings();
