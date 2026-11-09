#include "DisplayManager.h"

#include <Fonts/FreeMonoBold9pt7b.h>
#include <SPI.h>

#include <algorithm>

namespace {

constexpr int16_t kLeftMargin = 6;
constexpr int16_t kRightMargin = 6;
constexpr int16_t kHeaderAdvance = 17;
constexpr int16_t kRuleGap = 3;
constexpr int16_t kLineAdvance = 14;
constexpr int16_t kGroupGap = 3;
constexpr int16_t kFooterReserve = 14;

int groupKey(const Collection& collection, bool haveClock) {
  return haveClock ? collection.daysUntil : collection.dateKey;
}

String headerText(const Collection& collection, bool haveClock) {
  const String dateShort(collection.dateShort.c_str());
  if (!haveClock) {
    return dateShort;
  }
  const int days = collection.daysUntil;
  if (days < 0) {
    return "OVERDUE - " + dateShort;
  }
  if (days == 0) {
    return "TODAY - " + dateShort;
  }
  if (days == 1) {
    return "TOMORROW - " + dateShort;
  }
  return "IN " + String(days) + " DAYS - " + dateShort;
}

}  // namespace

DisplayManager& DisplayManager::getInstance() {
  static DisplayManager instance;
  return instance;
}

void DisplayManager::begin() {
  if (display != nullptr) {
    return;
  }
  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display = new DisplayType(GxEPD2_290_C90c(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
  display->init(115200, true, 50, false);
  display->setRotation(3);
}

String DisplayManager::fitText(const String& text, int16_t maxWidth) const {
  if (display == nullptr) {
    return text;
  }

  int16_t x1 = 0;
  int16_t y1 = 0;
  uint16_t w = 0;
  uint16_t h = 0;
  display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  if (static_cast<int16_t>(w) <= maxWidth) {
    return text;
  }

  int16_t low = 0;
  int16_t high = static_cast<int16_t>(text.length());
  while (low < high) {
    const int16_t mid = static_cast<int16_t>((low + high + 1) / 2);
    const String candidate = text.substring(0, mid) + "...";
    display->getTextBounds(candidate, 0, 0, &x1, &y1, &w, &h);
    if (static_cast<int16_t>(w) <= maxWidth) {
      low = mid;
    } else {
      high = static_cast<int16_t>(mid - 1);
    }
  }

  return text.substring(0, low) + "...";
}

void DisplayManager::showSchedule(const std::vector<Collection>& bins, bool haveClock, const char* updated,
                                  bool production) {
  if (display == nullptr) {
    return;
  }

  std::vector<Collection> sorted = bins;
  std::stable_sort(sorted.begin(), sorted.end(), [haveClock](const Collection& a, const Collection& b) {
    if (groupKey(a, haveClock) != groupKey(b, haveClock)) {
      return groupKey(a, haveClock) < groupKey(b, haveClock);
    }
    return a.rank < b.rank;
  });

  const int16_t panelWidth = display->width();
  const int16_t panelHeight = display->height();
  const int16_t right = panelWidth - kRightMargin;
  const int16_t usableWidth = right - kLeftMargin;

  display->setFullWindow();
  display->setRotation(3);

  display->firstPage();
  do {
    display->fillScreen(GxEPD_WHITE);
    display->setFont(&FreeMonoBold9pt7b);

    int16_t y = 2;
    size_t index = 0;
    while (index < sorted.size()) {
      const int key = groupKey(sorted[index], haveClock);
      size_t end = index;
      while (end < sorted.size() && groupKey(sorted[end], haveClock) == key) {
        ++end;
      }
      const int16_t groupHeight =
          kHeaderAdvance + kRuleGap + static_cast<int16_t>(end - index) * kLineAdvance + kGroupGap;

      if (y + groupHeight > panelHeight - kFooterReserve) {
        const int remaining = static_cast<int>(sorted.size() - index);
        if (y + 10 <= panelHeight - 18) {
          display->setTextColor(GxEPD_BLACK);
          display->setCursor(kLeftMargin, y + 10);
          display->print(fitText("+" + String(remaining) + " more", usableWidth));
        }
        break;
      }

      const Collection& head = sorted[index];
      const uint16_t headerColour = (haveClock && head.daysUntil <= 1) ? GxEPD_RED : GxEPD_BLACK;
      display->setTextColor(headerColour);
      display->setCursor(kLeftMargin, y + 12);
      display->print(fitText(headerText(head, haveClock), usableWidth));

      y += kHeaderAdvance;
      display->drawLine(kLeftMargin, y, right, y, GxEPD_BLACK);
      y += kRuleGap;

      display->setTextColor(GxEPD_BLACK);
      for (size_t i = index; i < end; ++i) {
        display->setCursor(kLeftMargin, y + 11);
        display->print(fitText(String(sorted[i].binLabel.c_str()), usableWidth));
        y += kLineAdvance;
      }

      y += kGroupGap;
      index = end;
    }

    display->setFont(nullptr);
    display->setTextColor(GxEPD_BLACK);
    display->setCursor(kLeftMargin, panelHeight - 3);
    if (haveClock && updated != nullptr && updated[0] != '\0') {
      String footer = "Updated " + String(updated);
      if (!production) {
        footer += " DEV";
      }
      display->print(fitText(footer, usableWidth));
    } else if (!haveClock) {
      display->print(fitText("Clock not synced", usableWidth));
    }
  } while (display->nextPage());
}

void DisplayManager::showError(const char* message) {
  if (display == nullptr) {
    return;
  }

  const int16_t panelWidth = display->width();
  const int16_t usableWidth = panelWidth - kLeftMargin - kRightMargin;

  display->setFullWindow();
  display->setRotation(3);

  display->firstPage();
  do {
    display->fillScreen(GxEPD_WHITE);
    display->setFont(&FreeMonoBold9pt7b);
    display->setTextColor(GxEPD_BLACK);
    display->setCursor(kLeftMargin, 40);
    display->print("Error");
    display->setCursor(kLeftMargin, 70);
    display->print(fitText(String(message == nullptr ? "" : message), usableWidth));
    display->setCursor(kLeftMargin, 100);
    display->print(fitText("Retrying in 5 min", usableWidth));
  } while (display->nextPage());
}

void DisplayManager::hibernate() {
  if (display != nullptr) {
    display->hibernate();
  }
}
