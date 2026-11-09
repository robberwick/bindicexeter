#pragma once

#include <Arduino.h>
#include <GxEPD2_3C.h>

#include <vector>

#include "ScheduleParser.h"

// Panel and default S3 Mini SPI pins.
static constexpr int EPD_BUSY = 4;
static constexpr int EPD_RST = 5;
static constexpr int EPD_DC = 6;
static constexpr int EPD_CS = 10;
static constexpr int EPD_SCK = 12;
static constexpr int EPD_MOSI = 11;

class DisplayManager {
 public:
  static DisplayManager& getInstance();

  DisplayManager(const DisplayManager&) = delete;
  DisplayManager& operator=(const DisplayManager&) = delete;

  void begin();
  void showSchedule(const std::vector<Collection>& bins, bool haveClock, const char* updated, bool production);
  void showError(const char* message);
  void hibernate();

 private:
  using DisplayType = GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT>;

  DisplayManager() = default;

  String fitText(const String& text, int16_t maxWidth) const;

  DisplayType* display = nullptr;
};
