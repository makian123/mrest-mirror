#pragma once

#include "util/reflection.hpp"

namespace mrest {
namespace impl {
struct RequestBodyImpl {};
}  // namespace impl

namespace annotation {
inline mrest::impl::RequestBodyImpl RequestBody;
struct RequestParam {
	util::StringLiteral name;

	consteval RequestParam() = default;
	consteval explicit RequestParam(std::string_view name) : name(name) {}
};

struct PathVariable {
	util::StringLiteral name;

	consteval PathVariable() = default;
	consteval explicit PathVariable(std::string_view name) : name(name) {}
};
}  // namespace annotation
}  // namespace mrest