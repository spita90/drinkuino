#pragma once
#include "settings.h"

// Access point + captive portal for configuration. Never returns:
// restarts after saving; after 10 minutes of inactivity it restarts
// (if already configured) or sleeps until a button is pressed.
[[noreturn]] void runPortal(Settings& cfg);
