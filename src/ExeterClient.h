#pragma once

#include <Arduino.h>

#include <ctime>

#include "AppConfig.h"

bool connectWiFi(const AppConfig& config, String& error);
bool syncClock(std::tm& now, String& error);
bool fetchSchedule(const String& uprn, String& html, String& error);
void stopWiFi();
