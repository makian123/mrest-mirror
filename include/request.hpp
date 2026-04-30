#pragma once

#include <string>
#include <unordered_map>

struct HttpHeader {
	std::string method;
    std::string path;
	std::string version;

	std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> parameters;

    explicit HttpHeader(const std::string &rawRequest);
};
struct HttpRequest {
    HttpHeader header;
    std::string body;

    explicit HttpRequest(const std::string &rawRequest);
};