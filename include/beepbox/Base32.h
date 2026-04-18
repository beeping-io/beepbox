#ifndef BEEPBOX_BASE32_H
#define BEEPBOX_BASE32_H

#include <string>

namespace beepbox {

/// Convert integer to base-32 string (digits 0-9, a-v).
std::string toBase32(int num);

/// Convert base-32 string to integer.
int fromBase32(const std::string& s);

/// Check if a character is a valid base-32 digit [0-9a-v].
bool isBase32Char(char c);

/// Validate that a key is exactly 5 valid base-32 characters.
bool isValidKey(const std::string& key);

} // namespace beepbox

#endif // BEEPBOX_BASE32_H
