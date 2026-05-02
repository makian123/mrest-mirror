#pragma once

#include "util/reflection.hpp"

struct RestController {
	StringLiteral route;
	
	consteval RestController() = default;
	explicit consteval RestController(std::string_view routeText): route(routeText){}
};