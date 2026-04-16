#include "beepbox/Base32.h"

namespace beepbox {

static constexpr char kDigits[] = "0123456789abcdefghijklmnopqrstuv";

std::string toBase32(int num) {
  if (num == 0) return "0";

  std::string result;
  bool negative = num < 0;
  if (negative) num = -num;

  while (num > 0) {
    result = kDigits[num % 32] + result;
    num /= 32;
  }

  if (negative) result = "-" + result;
  return result;
}

int fromBase32(const std::string& s) {
  int result = 0;
  int factor = 1;
  for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
    int val = -1;
    for (int j = 0; j < 32; ++j) {
      if (s[i] == kDigits[j]) {
        val = j;
        break;
      }
    }
    if (val < 0) return -1;
    result += factor * val;
    factor *= 32;
  }
  return result;
}

bool isBase32Char(char c) {
  for (int i = 0; i < 32; ++i) {
    if (c == kDigits[i]) return true;
  }
  return false;
}

bool isValidKey(const std::string& key) {
  if (key.size() != 5) return false;
  for (char c : key) {
    if (!isBase32Char(c)) return false;
  }
  return true;
}

} // namespace beepbox
