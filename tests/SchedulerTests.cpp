#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "beepbox/Scheduler.h"

using namespace beepbox;

TEST_CASE("Scheduler: zero beeps if duration < 2.3", "[scheduler]") {
  CHECK(computeBeepCount(1.0f, 0.0f, 2.3f) == 0);
  CHECK(computeBeepCount(2.29f, 0.0f, 2.3f) == 0);
}

TEST_CASE("Scheduler: exactly one beep at 2.3s", "[scheduler]") {
  CHECK(computeBeepCount(2.3f, 0.0f, 2.3f) == 1);
}

TEST_CASE("Scheduler: multiple beeps", "[scheduler]") {
  // 5.0s with 2.3s interval: first at 0, second at 2.3 → 2 beeps
  CHECK(computeBeepCount(5.0f, 0.0f, 2.3f) == 2);
  // 10.0s with 2.3s interval: 0, 2.3, 4.6, 6.9 → 4 beeps
  CHECK(computeBeepCount(10.0f, 0.0f, 2.3f) == 4);
}

TEST_CASE("Scheduler: start offset reduces beeps", "[scheduler]") {
  // 5.0s, start at 2.0, interval 2.3: first at 2.0 (needs 4.3) → 1 beep
  CHECK(computeBeepCount(5.0f, 2.0f, 2.3f) == 1);
  // Start so late no beep fits
  CHECK(computeBeepCount(3.0f, 2.0f, 2.3f) == 0);
}

TEST_CASE("Scheduler: schedule timestamps match count", "[scheduler]") {
  auto sched = computeBeepSchedule(10.0f, 0.0f, 2.3f);
  int count = computeBeepCount(10.0f, 0.0f, 2.3f);
  CHECK(static_cast<int>(sched.size()) == count);
}

TEST_CASE("Scheduler: schedule with start offset", "[scheduler]") {
  auto sched = computeBeepSchedule(10.0f, 1.5f, 3.0f);
  REQUIRE(!sched.empty());
  CHECK(sched[0] == 1.5);
  if (sched.size() > 1) {
    CHECK(sched[1] == Catch::Approx(4.5));
  }
}
