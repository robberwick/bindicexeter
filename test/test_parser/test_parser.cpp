#include <unity.h>

#include <ctime>
#include <string>
#include <vector>

#include "ScheduleParser.h"

namespace {

const char* MARYPOLE_HTML =
    "<h2>Refuse collection</h2>"
    "<p>Your next refuse collection will be on</p>"
    "<h3><img src='https://exeter.gov.uk/img/ECC/binblack.jpg' alt='Black bin' class='binicon'/>Tuesday, 17th "
    "November 2026</h3>"
    "<p>To be collected from at the edge of your address so bin is visible from your street address</p>"
    "<h2>Recycling collection</h2>"
    "<p>Your next recycling collection will be on</p>"
    "<h3><img src='https://exeter.gov.uk/img/ECC/bingreen.jpg' alt='Green bin' class='binicon'/>Tuesday, 10th "
    "November 2026</h3>"
    "<p>To be collected from at the edge of your address so bin is visible from your street address</p>"
    "<h2>Garden waste collection</h2>"
    "<img src='https://exeter.gov.uk/img/ECC/binbrown.jpg' alt='Brown bin' class='binicon'/>"
    "<a href='/bins-and-recycling/garden-waste/sign-up-for-garden-bin-collections/'>Sign up now to receive "
    "fortnightly, charged-for garden waste collections</a>"
    "<h2>Food waste collection</h2>"
    "<p>Your next food waste collection will be on</p>"
    "<h3><img src='https://exeter.gov.uk/img/ECC/foodcaddy.png' alt='Food caddy' class='binicon'/>Tuesday, 10th "
    "November 2026</h3>"
    "<p>To be collected from front edge of address, visible from road</p>";

const char* GARDEN_SUBSCRIBED_HTML =
    "<h2>Refuse collection</h2>"
    "<h3><img src='https://exeter.gov.uk/img/ECC/binblack.jpg' alt='Black bin' class='binicon'/>Tuesday, 17th "
    "November 2026</h3>"
    "<h2>Recycling collection</h2>"
    "<h3><img src='https://exeter.gov.uk/img/ECC/bingreen.jpg' alt='Green bin' class='binicon'/>Tuesday, 10th "
    "November 2026</h3>"
    "<h2>Garden waste collection</h2>"
    "<h3><img src='x' alt='Brown bin'/>Tuesday, 10th November 2026</h3>"
    "<h2>Food waste collection</h2>"
    "<h3><img src='https://exeter.gov.uk/img/ECC/foodcaddy.png' alt='Food caddy' class='binicon'/>Tuesday, 10th "
    "November 2026</h3>";

Collection parseSingleDate(const std::string& date) {
  const std::string html =
      "<h2>Refuse collection</h2><h3><img alt='Black bin'/>" + date + "</h3>";
  const std::vector<Collection> parsed = parseCollections(html);
  TEST_ASSERT_EQUAL_UINT(1, parsed.size());
  return parsed[0];
}

}  // namespace

void test_fixture_a_marypole(void) {
  const std::vector<Collection> bins = parseCollections(MARYPOLE_HTML);
  TEST_ASSERT_EQUAL_UINT(3, bins.size());

  TEST_ASSERT_EQUAL_STRING("Refuse", bins[0].section.c_str());
  TEST_ASSERT_EQUAL_STRING("Black bin", bins[0].binLabel.c_str());
  TEST_ASSERT_EQUAL_INT(17, bins[0].day);
  TEST_ASSERT_EQUAL_INT(11, bins[0].month);
  TEST_ASSERT_EQUAL_INT(2026, bins[0].year);
  TEST_ASSERT_EQUAL_STRING("Tue 17 Nov", bins[0].dateShort.c_str());
  TEST_ASSERT_EQUAL_INT(20261117, bins[0].dateKey);
  TEST_ASSERT_EQUAL_INT(0, bins[0].rank);
  TEST_ASSERT_EQUAL_INT(INT_MIN, bins[0].daysUntil);

  TEST_ASSERT_EQUAL_STRING("Recycling", bins[1].section.c_str());
  TEST_ASSERT_EQUAL_STRING("Green bin", bins[1].binLabel.c_str());
  TEST_ASSERT_EQUAL_INT(10, bins[1].day);
  TEST_ASSERT_EQUAL_INT(11, bins[1].month);
  TEST_ASSERT_EQUAL_INT(2026, bins[1].year);
  TEST_ASSERT_EQUAL_STRING("Tue 10 Nov", bins[1].dateShort.c_str());
  TEST_ASSERT_EQUAL_INT(20261110, bins[1].dateKey);
  TEST_ASSERT_EQUAL_INT(1, bins[1].rank);

  TEST_ASSERT_EQUAL_STRING("Food waste", bins[2].section.c_str());
  TEST_ASSERT_EQUAL_STRING("Food caddy", bins[2].binLabel.c_str());
  TEST_ASSERT_EQUAL_INT(10, bins[2].day);
  TEST_ASSERT_EQUAL_INT(11, bins[2].month);
  TEST_ASSERT_EQUAL_INT(2026, bins[2].year);
  TEST_ASSERT_EQUAL_STRING("Tue 10 Nov", bins[2].dateShort.c_str());
  TEST_ASSERT_EQUAL_INT(20261110, bins[2].dateKey);
  TEST_ASSERT_EQUAL_INT(3, bins[2].rank);
}

void test_fixture_b_garden_subscribed(void) {
  const std::vector<Collection> bins = parseCollections(GARDEN_SUBSCRIBED_HTML);
  TEST_ASSERT_EQUAL_UINT(4, bins.size());

  TEST_ASSERT_EQUAL_STRING("Garden waste", bins[2].section.c_str());
  TEST_ASSERT_EQUAL_STRING("Brown bin", bins[2].binLabel.c_str());
  TEST_ASSERT_EQUAL_INT(2, bins[2].rank);
  TEST_ASSERT_EQUAL_INT(20261110, bins[2].dateKey);
}

void test_fixture_c_unparseable_date(void) {
  const std::vector<Collection> bins = parseCollections("<h2>Refuse collection</h2><h3>not a date</h3>");
  TEST_ASSERT_EQUAL_UINT(0, bins.size());
}

void test_fixture_d_empty(void) {
  const std::vector<Collection> bins = parseCollections("");
  TEST_ASSERT_EQUAL_UINT(0, bins.size());
}

void test_date_variant_first_of_month(void) {
  const Collection bin = parseSingleDate("Monday, 1st January 2027");
  TEST_ASSERT_EQUAL_STRING("Mon 1 Jan", bin.dateShort.c_str());
  TEST_ASSERT_EQUAL_INT(1, bin.day);
  TEST_ASSERT_EQUAL_INT(1, bin.month);
  TEST_ASSERT_EQUAL_INT(2027, bin.year);
}

void test_date_variant_second(void) {
  const Collection bin = parseSingleDate("Wednesday, 22nd December 2027");
  TEST_ASSERT_EQUAL_STRING("Wed 22 Dec", bin.dateShort.c_str());
  TEST_ASSERT_EQUAL_INT(22, bin.day);
  TEST_ASSERT_EQUAL_INT(12, bin.month);
  TEST_ASSERT_EQUAL_INT(2027, bin.year);
}

void test_date_variant_third(void) {
  const Collection bin = parseSingleDate("Friday, 3rd March 2028");
  TEST_ASSERT_EQUAL_STRING("Fri 3 Mar", bin.dateShort.c_str());
  TEST_ASSERT_EQUAL_INT(3, bin.day);
  TEST_ASSERT_EQUAL_INT(3, bin.month);
  TEST_ASSERT_EQUAL_INT(2028, bin.year);
}

void test_days_from_today(void) {
  std::tm now = {};
  now.tm_year = 126;  // 2026
  now.tm_mon = 10;    // November
  now.tm_mday = 9;

  const std::vector<Collection> bins = parseCollections(MARYPOLE_HTML);
  TEST_ASSERT_EQUAL_UINT(3, bins.size());

  TEST_ASSERT_EQUAL_INT(8, daysFromToday(bins[0], now));
  TEST_ASSERT_EQUAL_INT(1, daysFromToday(bins[1], now));
  TEST_ASSERT_EQUAL_INT(1, daysFromToday(bins[2], now));
}

void test_days_from_today_year_boundary(void) {
  Collection bin;
  bin.day = 1;
  bin.month = 1;
  bin.year = 2027;
  bin.dateKey = 20270101;

  std::tm now = {};
  now.tm_year = 126;  // 2026
  now.tm_mon = 11;    // December
  now.tm_mday = 31;

  TEST_ASSERT_EQUAL_INT(1, daysFromToday(bin, now));
}

void setUp(void) {}

void tearDown(void) {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_fixture_a_marypole);
  RUN_TEST(test_fixture_b_garden_subscribed);
  RUN_TEST(test_fixture_c_unparseable_date);
  RUN_TEST(test_fixture_d_empty);
  RUN_TEST(test_date_variant_first_of_month);
  RUN_TEST(test_date_variant_second);
  RUN_TEST(test_date_variant_third);
  RUN_TEST(test_days_from_today);
  RUN_TEST(test_days_from_today_year_boundary);
  return UNITY_END();
}
