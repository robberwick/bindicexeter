#pragma once

#include <Arduino.h>

struct AppConfig {
  String wifiSsid;
  String wifiPassword;
  String uprn;
  bool productionMode = true;
};

// Reads /config.json from LittleFS. Returns false and fills `error` on any failure.
bool loadAppConfig(AppConfig& config, String& error);
