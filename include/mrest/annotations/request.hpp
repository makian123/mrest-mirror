#pragma once

#include <string_view>
#include <util/reflection.hpp>

namespace mrest {
namespace annotation {
struct Request {
	util::StringLiteral route;
	util::StringLiteral method;

	consteval Request() = default;
	consteval Request(std::string_view routeText, std::string_view methodText)
		: route(define_static_string(routeText)), method(std::define_static_string(methodText)) {}
};
struct GetRequest : Request {
	consteval GetRequest() : Request("", "GET") {}
	consteval explicit GetRequest(std::string_view route) : Request(route, "GET") {}
};
struct PostRequest : Request {
	consteval PostRequest() : Request("", "POST") {}
	consteval explicit PostRequest(std::string_view route) : Request(route, "POST") {}
};
struct PutRequest : Request {
	consteval PutRequest() : Request("", "PUT") {}
	consteval explicit PutRequest(std::string_view route) : Request(route, "PUT") {}
};
struct PatchRequest : Request {
	consteval PatchRequest() : Request("", "PATCH") {}
	consteval explicit PatchRequest(std::string_view route) : Request(route, "PATCH") {}
};
struct DeleteRequest : Request {
	consteval DeleteRequest() : Request("", "DELETE") {}
	consteval explicit DeleteRequest(std::string_view route) : Request(route, "DELETE") {}
};
}  // namespace annotation
}  // namespace mrest