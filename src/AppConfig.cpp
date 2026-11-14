#include "AppConfig.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {

// A browser install writes no filesystem, so the firmware formats one on first
// boot and then takes the config from USB serial (the flasher page's form).
constexpr uint32_t kSerialConfigWaitMs = 120000;
constexpr size_t kMaxConfigLineBytes = 2048;

bool applyConfig(const JsonDocument& doc, AppConfig& config, String& error) {
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

bool readLine(String& line) {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (!line.isEmpty()) return true;
      continue;
    }
    if (line.length() < kMaxConfigLineBytes) line += c;
  }
  return false;
}

// Waits for one JSON line on USB serial, writes it to /config.json.
bool provisionOverSerial(AppConfig& config, String& error) {
  Serial.println("No config.json: send config JSON on this port (flasher page: Device config).");

  String line;
  const uint32_t deadline = millis() + kSerialConfigWaitMs;
  while (millis() < deadline) {
    if (!readLine(line)) {
      delay(10);
      continue;
    }

    StaticJsonDocument<512> doc;
    String reason;
    AppConfig fromSerial;
    if (deserializeJson(doc, line) != DeserializationError::Ok) {
      Serial.println("Rejected: invalid JSON");
      line = "";
      continue;
    }
    if (!applyConfig(doc, fromSerial, reason)) {
      Serial.print("Rejected: ");
      Serial.println(reason);
      line = "";
      continue;
    }

    File file = LittleFS.open("/config.json", "w");
    if (!file) {
      Serial.println("Failed to write config.json");
      continue;
    }
    file.print(line);
    file.close();

    Serial.println("OK config.json saved");
    config = fromSerial;
    return true;
  }

  error = "config.json missing";
  return false;
}

}  // namespace

bool loadAppConfig(AppConfig& config, String& error) {
  // Auto-format: a board flashed from the browser has no filesystem yet.
  if (!LittleFS.begin(true)) {
    error = "LittleFS mount failed";
    return false;
  }

  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    return provisionOverSerial(config, error);
  }

  StaticJsonDocument<512> doc;
  DeserializationError parseError = deserializeJson(doc, file);
  file.close();

  if (parseError) {
    error = "config.json invalid";
    return false;
  }

  return applyConfig(doc, config, error);
}
