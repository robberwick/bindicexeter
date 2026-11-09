#include "ScheduleParser.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>

namespace {

const char* const kMonthNames[12] = {"January", "February", "March",     "April",   "May",      "June",
                                     "July",    "August",   "September", "October", "November", "December"};

bool isSpaceChar(char ch) {
  return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

bool isDigitChar(char ch) {
  return std::isdigit(static_cast<unsigned char>(ch)) != 0;
}

bool isAlphaChar(char ch) {
  return std::isalpha(static_cast<unsigned char>(ch)) != 0;
}

std::string stripTags(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  bool inTag = false;
  for (char ch : input) {
    if (ch == '<') {
      inTag = true;
    } else if (ch == '>') {
      inTag = false;
    } else if (!inTag) {
      out.push_back(ch);
    }
  }
  return out;
}

std::string collapseWhitespace(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  bool pendingSpace = false;
  for (char ch : input) {
    if (isSpaceChar(ch)) {
      if (!out.empty()) {
        pendingSpace = true;
      }
      continue;
    }
    if (pendingSpace) {
      out.push_back(' ');
      pendingSpace = false;
    }
    out.push_back(ch);
  }
  return out;
}

std::string cleanText(const std::string& input) {
  return collapseWhitespace(stripTags(input));
}

void skipSpaces(const std::string& text, size_t& pos) {
  while (pos < text.size() && isSpaceChar(text[pos])) {
    ++pos;
  }
}

// Finds the content between the first `openTag` occurrence at/after `from` and the next `closeTag`.
// Returns false when either marker is missing.
bool readElement(const std::string& html, const char* openTag, const char* closeTag, size_t from, size_t& contentStart,
                 size_t& contentEnd, size_t& elementEnd) {
  const size_t open = html.find(openTag, from);
  if (open == std::string::npos) {
    return false;
  }
  const size_t openEnd = html.find('>', open);
  if (openEnd == std::string::npos) {
    return false;
  }
  const size_t close = html.find(closeTag, openEnd + 1);
  if (close == std::string::npos) {
    return false;
  }
  contentStart = openEnd + 1;
  contentEnd = close;
  elementEnd = close + std::char_traits<char>::length(closeTag);
  return true;
}

// Extracts alt='...' or alt="..." from a raw tag fragment.
std::string findAltText(const std::string& raw) {
  const size_t alt = raw.find("alt=");
  if (alt == std::string::npos) {
    return std::string();
  }
  size_t pos = alt + 4;
  if (pos >= raw.size()) {
    return std::string();
  }
  const char quote = raw[pos];
  if (quote != '\'' && quote != '"') {
    return std::string();
  }
  const size_t end = raw.find(quote, pos + 1);
  if (end == std::string::npos) {
    return std::string();
  }
  return cleanText(raw.substr(pos + 1, end - pos - 1));
}

int monthIndexFromName(const std::string& text, size_t pos) {
  if (pos + 3 > text.size()) {
    return 0;
  }
  std::string abbrev;
  for (size_t i = 0; i < 3; ++i) {
    abbrev.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(text[pos + i]))));
  }
  for (int month = 0; month < 12; ++month) {
    std::string candidate;
    for (size_t i = 0; i < 3; ++i) {
      candidate.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(kMonthNames[month][i]))));
    }
    if (candidate == abbrev) {
      return month + 1;
    }
  }
  return 0;
}

int rankForSection(const std::string& section) {
  if (section == "Refuse") return 0;
  if (section == "Recycling") return 1;
  if (section == "Garden waste") return 2;
  if (section == "Food waste") return 3;
  return 4;
}

std::string defaultBinLabel(const std::string& section) {
  if (section == "Refuse") return "Black bin";
  if (section == "Recycling") return "Green bin";
  if (section == "Garden waste") return "Brown bin";
  if (section == "Food waste") return "Food caddy";
  return section;
}

std::string normaliseSectionTitle(const std::string& title) {
  static const std::string kSuffix = " collection";
  if (title.size() >= kSuffix.size() &&
      title.compare(title.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0) {
    return title.substr(0, title.size() - kSuffix.size());
  }
  return title;
}

// Parses "Tuesday, 17th November 2026". Returns false when the text is not a usable date.
bool parseDateText(const std::string& text, Collection& collection) {
  std::string weekday;
  size_t pos = text.find(',');
  if (pos != std::string::npos) {
    weekday = collapseWhitespace(text.substr(0, pos));
    pos += 1;
  } else {
    pos = 0;
  }

  skipSpaces(text, pos);

  const size_t dayStart = pos;
  while (pos < text.size() && isDigitChar(text[pos])) {
    ++pos;
  }
  if (pos == dayStart) {
    return false;
  }
  const int day = std::atoi(text.substr(dayStart, pos - dayStart).c_str());

  while (pos < text.size() && isAlphaChar(text[pos])) {
    ++pos;
  }
  skipSpaces(text, pos);

  const int month = monthIndexFromName(text, pos);
  if (month == 0) {
    return false;
  }
  pos += 3;
  while (pos < text.size() && isAlphaChar(text[pos])) {
    ++pos;
  }
  skipSpaces(text, pos);

  const size_t yearStart = pos;
  while (pos < text.size() && isDigitChar(text[pos])) {
    ++pos;
  }
  if (pos == yearStart) {
    return false;
  }
  const int year = std::atoi(text.substr(yearStart, pos - yearStart).c_str());

  if (day < 1 || day > 31) {
    return false;
  }
  if (year < 2020 || year > 2100) {
    return false;
  }

  collection.weekday = weekday;
  collection.day = day;
  collection.month = month;
  collection.year = year;
  collection.dateKey = year * 10000 + month * 100 + day;

  std::string dateShort;
  if (weekday.size() >= 3) {
    dateShort = weekday.substr(0, 3);
    dateShort.push_back(' ');
  }
  dateShort += std::to_string(day);
  dateShort.push_back(' ');
  dateShort += std::string(kMonthNames[month - 1], 3);
  collection.dateShort = dateShort;
  return true;
}

}  // namespace

std::vector<Collection> parseCollections(const std::string& html) {
  std::vector<Collection> collections;
  size_t searchFrom = 0;

  while (searchFrom < html.size()) {
    size_t titleStart = 0;
    size_t titleEnd = 0;
    size_t sectionStart = 0;
    if (!readElement(html, "<h2", "</h2>", searchFrom, titleStart, titleEnd, sectionStart)) {
      break;
    }

    const std::string section = normaliseSectionTitle(cleanText(html.substr(titleStart, titleEnd - titleStart)));

    size_t nextSection = html.find("<h2", sectionStart);
    const size_t sectionEnd = (nextSection == std::string::npos) ? html.size() : nextSection;
    searchFrom = sectionEnd;

    size_t contentStart = 0;
    size_t contentEnd = 0;
    size_t elementEnd = 0;
    if (!readElement(html, "<h3", "</h3>", sectionStart, contentStart, contentEnd, elementEnd) ||
        contentStart > sectionEnd) {
      continue;  // Garden-waste sign-up block has no date.
    }

    const std::string raw = html.substr(contentStart, contentEnd - contentStart);
    Collection collection;
    collection.section = section;
    if (!parseDateText(cleanText(raw), collection)) {
      continue;
    }

    collection.binLabel = findAltText(raw);
    if (collection.binLabel.empty()) {
      collection.binLabel = defaultBinLabel(section);
    }
    collection.rank = rankForSection(section);
    collection.daysUntil = INT_MIN;
    collections.push_back(collection);
  }

  return collections;
}

namespace {

// Howard Hinnant's days_from_civil.
int64_t daysFromCivil(int64_t year, unsigned month, unsigned day) {
  year -= month <= 2 ? 1 : 0;
  const int64_t era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
  const unsigned dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
  const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
  return era * 146097 + static_cast<int64_t>(dayOfEra) - 719468;
}

}  // namespace

int daysFromToday(const Collection& collection, const std::tm& now) {
  const int64_t target = daysFromCivil(collection.year, static_cast<unsigned>(collection.month),
                                       static_cast<unsigned>(collection.day));
  const int64_t today =
      daysFromCivil(now.tm_year + 1900, static_cast<unsigned>(now.tm_mon + 1), static_cast<unsigned>(now.tm_mday));
  return static_cast<int>(target - today);
}
