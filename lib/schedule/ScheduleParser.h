#pragma once

#include <climits>
#include <ctime>
#include <string>
#include <vector>

struct Collection {
  std::string section;    // "Refuse", "Recycling", "Garden waste", "Food waste"
  std::string binLabel;   // "Black bin", "Green bin", "Brown bin", "Food caddy"
  std::string weekday;    // "Tuesday"
  int day = 0;
  int month = 0;
  int year = 0;
  std::string dateShort;  // "Tue 17 Nov"
  int dateKey = 0;        // year*10000 + month*100 + day
  int rank = 0;           // Refuse 0, Recycling 1, Garden waste 2, Food waste 3, other 4
  int daysUntil = INT_MIN;  // filled by main when the clock is valid
};

// Parses the `Results` HTML fragment returned by the Exeter address finder.
std::vector<Collection> parseCollections(const std::string& html);

// Whole calendar days from the date held in `now` to the collection date.
int daysFromToday(const Collection& collection, const std::tm& now);
