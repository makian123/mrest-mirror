#pragma once

#include <span>
#include <string_view>
#include <algorithm>

namespace mrest {
namespace util {
inline auto SearchSubstring(std::span<const char> source, std::string_view substring, int startPos = 0) {
	return std::search(source.begin() + startPos, source.end(), substring.begin(), substring.end());
}
inline auto SearchSubstring(std::span<const char> source, std::string_view substring,
					 std::span<const char>::iterator startPos) {
	return std::search(startPos, source.end(), substring.cbegin(), substring.cend());
}
}  // namespace util
}  // namespace mrest