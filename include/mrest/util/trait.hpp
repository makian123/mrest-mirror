#pragma once

#include <string>
#include <type_traits>

namespace mrest {
namespace util {
template <typename T>
concept HasToString = requires(T a) {
	{ std::to_string(a) } -> std::convertible_to<std::string>;
};
}  // namespace trait
}  // namespace mrest