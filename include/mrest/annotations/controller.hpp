#pragma once

#include "util/reflection.hpp"

namespace mrest {
namespace annotation {
struct RestController {
	util::StringLiteral route;

	consteval RestController() = default;
	explicit consteval RestController(std::string_view routeText) : route(routeText) {}
};

struct Configuration {
	util::StringLiteral sectionName;

	consteval Configuration() = default;
	explicit consteval Configuration(std::string_view section) : sectionName(section) {}
};
}  // namespace annotation
}  // namespace mrest