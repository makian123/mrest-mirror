#pragma once

#include "util/reflection.hpp"

namespace mrest {
namespace impl {
struct RequestBodyImpl {};
struct RequestCookiesImpl {};
struct RequestSessionImpl {};
}  // namespace impl

namespace annotation {
inline impl::RequestBodyImpl RequestBody;
inline impl::RequestCookiesImpl RequestCookies;
inline impl::RequestSessionImpl RequestSession;

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