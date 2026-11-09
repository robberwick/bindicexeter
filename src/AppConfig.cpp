#include "AppConfig.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

bool loadAppConfig(AppConfig& config, String& error) {
  if (!LittleFS.begin(false)) {
    // No auto-format: a missing filesystem means `uploadfs` was never run.
    error = "LittleFS mount failed";
    return false;
  }

  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    error = "config.json missing";
    return false;
  }

  StaticJsonDocument<512> doc;
  DeserializationError parseError = deserializeJson(doc, file);
  file.close();

  if (parseError) {
    error = "config.json invalid";
    return false;
  }

  const String wifiSsid = doc["wifi_ssid"].as<String>();
  const String wifiPassword = doc["wifi_password"].as<String>();
  const String uprn = doc["uprn"].as<String>();
  const bool productionMode = doc["production_mode"] | true;

  if (wifiSsid.isEmpty()) {
    error = "WiFi SSID missing";
    return false;
  }

  if (uprn.length() < 10 || uprn.length() > 15) {
    error = "UPRN must be 10-15 digits";
    return false;
  }

  for (size_t i = 0; i < uprn.length(); ++i) {
    if (!isDigit(uprn[i])) {
      error = "UPRN must be 10-15 digits";
      return false;
    }
  }

  config.wifiSsid = wifiSsid;
  config.wifiPassword = wifiPassword;
  config.uprn = uprn;
  config.productionMode = productionMode;
  return true;
}
