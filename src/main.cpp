#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_sleep.h>

#include <ctime>
#include <vector>

#include "AppConfig.h"
#include "DisplayManager.h"
#include "ExeterClient.h"
#include "ScheduleParser.h"

namespace {

constexpr uint64_t kProductionSleepSeconds = 10800;
constexpr uint64_t kDevelopmentSleepSeconds = 20;
constexpr uint64_t kErrorSleepSeconds = 300;

[[noreturn]] void sleepAndRestart(uint64_t seconds) {
  DisplayManager::getInstance().hibernate();
  stopWiFi();
  Serial.flush();
  esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
  esp_deep_sleep_start();
  while (true) {
    // esp_deep_sleep_start() does not return.
  }
}

[[noreturn]] void fail(const char* message) {
  Serial.print("Error: ");
  Serial.println(message);
  DisplayManager::getInstance().showError(message);
  sleepAndRestart(kErrorSleepSeconds);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("bindicexeter boot");

  DisplayManager::getInstance().begin();

  AppConfig config;
  String error;
  if (!loadAppConfig(config, error)) {
    fail(error.c_str());
  }

  if (!connectWiFi(config, error)) {
    fail(error.c_str());
  }

  std::tm now = {};
  const bool haveClock = syncClock(now, error);
  if (!haveClock) {
    Serial.print("NTP failed: ");
    Serial.println(error);
  }

  String html;
  if (!fetchSchedule(config.uprn, html, error)) {
    fail(error.c_str());
  }

  DynamicJsonDocument doc(4096);
  const DeserializationError jsonError = deserializeJson(doc, html);
  if (jsonError || !doc.is<JsonArray>() || doc.size() == 0) {
    fail("Bad response");
  }

  const char* results = doc[0]["Results"] | "";
  if (results == nullptr || results[0] == '\0') {
    fail("Bad response");
  }

  std::vector<Collection> bins = parseCollections(results);
  if (bins.empty()) {
    fail("No collections parsed");
  }
  Serial.print("Parsed ");
  Serial.print(bins.size());
  Serial.println(" collections");

  char updated[16] = {};
  if (haveClock) {
    for (Collection& bin : bins) {
      bin.daysUntil = daysFromToday(bin, now);
    }
    strftime(updated, sizeof(updated), "%H:%M", &now);
  }

  DisplayManager::getInstance().showSchedule(bins, haveClock, haveClock ? updated : "", config.productionMode);

  sleepAndRestart(config.productionMode ? kProductionSleepSeconds : kDevelopmentSleepSeconds);
}

void loop() {
  delay(1000);
}
