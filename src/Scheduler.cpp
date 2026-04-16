#include "beepbox/Scheduler.h"
#include <cmath>
#include <algorithm>

namespace beepbox {

int computeBeepCount(float duration, float startTime, float interval) {
  if (duration < kMinBeepWindow || (startTime + kMinBeepWindow) > duration)
    return 0;
  float remaining = duration - startTime - kMinBeepWindow;
  int extra = static_cast<int>(std::floor((remaining + 1e-6f) / interval));
  return 1 + std::max(extra, 0);
}

std::vector<double> computeBeepSchedule(float duration, float startTime, float interval) {
  std::vector<double> times;
  int count = computeBeepCount(duration, startTime, interval);
  double t = startTime;
  while (static_cast<int>(times.size()) < count &&
         (t + kMinBeepWindow) <= duration + 1e-6) {
    times.push_back(t);
    t += interval;
  }
  return times;
}

} // namespace beepbox
