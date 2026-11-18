#include "ExeterClient.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

namespace {

constexpr uint32_t kWifiConnectTimeoutMs = 30000;
constexpr uint32_t kNtpTimeoutMs = 15000;
constexpr time_t kMinValidTimestamp = 1704067200;  // 2024-01-01T00:00:00Z

}  // namespace

bool connectWiFi(const AppConfig& config, String& error) {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start >= kWifiConnectTimeoutMs) {
      error = "WiFi connect failed";
      return false;
    }
    delay(250);
  }

  Serial.print("WiFi connected, IP ");
  Serial.println(WiFi.localIP());
  return true;
}

bool syncClock(std::tm& now, String& error) {
  configTzTime("GMT0BST,M3.5.0/1,M10.5.0", "pool.ntp.org", "time.cloudflare.com");

  const uint32_t start = millis();
  while (time(nullptr) < kMinValidTimestamp) {
    if (millis() - start >= kNtpTimeoutMs) {
      error = "NTP failed";
      return false;
    }
    delay(250);
  }

  const time_t timestamp = time(nullptr);
  localtime_r(&timestamp, &now);
  Serial.println("Clock synced");
  return true;
}

bool fetchSchedule(const String& uprn, String& html, String& error) {
  const String url =
      "https://exeter.gov.uk/repositories/hidden-pages/address-finder/?qsource=UPRN&qtype=bins&term=" + uprn;

  WiFiClientSecure client;
  // Deliberate: public read-only data, and with no OTA a pinned CA would strand the device on rotation.
  client.setInsecure();
  client.setHandshakeTimeout(15);

  HTTPClient http;
  http.setTimeout(15000);
  http.setUserAgent("bindicexeter/1.0");

  if (!http.begin(client, url)) {
    error = "HTTP begin failed";
    return false;
  }

  http.addHeader("Accept", "application/json");
  const int code = http.GET();
  const String payload = http.getString();
  Serial.print("HTTP ");
  Serial.print(code);
  Serial.print(", ");
  Serial.print(payload.length());
  Serial.println(" bytes");
  http.end();

  if (code != HTTP_CODE_OK) {
    error = "HTTP " + String(code);
    return false;
  }
  if (payload.isEmpty()) {
    error = "Empty response";
    return false;
  }

  html = payload;
  return true;
}

void stopWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}
