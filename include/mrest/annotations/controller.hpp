#pragma once

#include "util/reflection.hpp"

namespace mrest {
namespace annotation {
struct RestController {
	util::StringLiteral route;

	consteval RestController() = default;
	explicit consteval RestController(std::string_view routeText) : route(routeText) {}
};
}  // namespace annotation
}  // namespace mrest