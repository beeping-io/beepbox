#ifndef BEEPBOX_SCHEDULER_H
#define BEEPBOX_SCHEDULER_H

#include <vector>

namespace beepbox {

/// Minimum duration of one beep window in seconds.
constexpr float kMinBeepWindow = 2.3f;

/// Compute how many beeps fit in the given duration.
int computeBeepCount(float duration, float startTime, float interval);

/// Compute the timestamp (in seconds) of each beep.
std::vector<double> computeBeepSchedule(float duration, float startTime, float interval);

} // namespace beepbox

#endif // BEEPBOX_SCHEDULER_H
