#include <catch2/catch_test_macros.hpp>
#include "beepbox/Generator.h"
#include "beepbox/Params.h"
#include <thread>
#include <vector>

using namespace beepbox;

TEST_CASE("Concurrency: parallel generateBeeps no data race", "[concurrency]") {
  constexpr int kThreads = 8;
  std::vector<std::thread> threads;
  std::vector<GenerateResult> results(kThreads);

  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([i, &results]() {
      Params p;
      p.key = "0abc" + std::string(1, static_cast<char>('0' + i % 10));
      p.mode = (i % 2 == 0) ? Mode::Audible : Mode::Inaudible;
      p.sampleRate = 44100.0f;
      p.duration = 3.0f;
      p.interval = 2.3f;
      p.volumeBeepsdB = -3.0f;
      results[i] = generateBeeps(p);
    });
  }

  for (auto& t : threads) t.join();

  for (int i = 0; i < kThreads; ++i) {
    CHECK(results[i].beepsGenerated >= 1);
    CHECK(!results[i].samples.empty());
  }
}

TEST_CASE("Concurrency: parallel instances produce independent results", "[concurrency]") {
  Params p1;
  p1.key = "aaaaa";
  p1.mode = Mode::Audible;
  p1.sampleRate = 44100.0f;
  p1.duration = 3.0f;
  p1.interval = 2.3f;

  Params p2;
  p2.key = "bbbbb";
  p2.mode = Mode::Inaudible;
  p2.sampleRate = 44100.0f;
  p2.duration = 3.0f;
  p2.interval = 2.3f;

  GenerateResult r1, r2;
  std::thread t1([&]() { r1 = generateBeeps(p1); });
  std::thread t2([&]() { r2 = generateBeeps(p2); });
  t1.join();
  t2.join();

  CHECK(r1.beepsGenerated >= 1);
  CHECK(r2.beepsGenerated >= 1);

  // Different keys + modes → different output
  bool allSame = true;
  size_t len = std::min(r1.samples.size(), r2.samples.size());
  for (size_t i = 0; i < len; ++i) {
    if (r1.samples[i] != r2.samples[i]) { allSame = false; break; }
  }
  CHECK_FALSE(allSame);
}
