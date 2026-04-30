#pragma once

#include <string_view>

struct Request {
	std::string_view route;
	std::string_view method;

	consteval Request(std::string_view route, std::string_view method)
		: route{route}, method{method} {}
};
struct GetRequest : Request {
	consteval GetRequest(std::string_view route): Request(route, "GET"){}
};
struct PostRequest : Request {
	consteval PostRequest(std::string_view route): Request(route, "POST"){}
};
struct PutRequest : Request {
	consteval PutRequest(std::string_view route): Request(route, "PUT"){}
};
struct PatchRequest : Request {
	consteval PatchRequest(std::string_view route): Request(route, "PATCH"){}
};
struct DeleteRequest : Request {
	consteval DeleteRequest(std::string_view route): Request(route, "DELETE"){}
};