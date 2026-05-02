#pragma once

#include <string>
#include <unordered_map>

struct HttpHeader {
	std::string method;
    std::string path;
	std::string version = "HTTP/1.1";
    std::string statusCode;

	std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> parameters;

    HttpHeader() = default;
    explicit HttpHeader(const std::string &rawRequest);

    std::string Stringify() const;
};
struct HttpRequest {
    HttpHeader header;
    std::string body;

    HttpRequest() = default;
    explicit HttpRequest(const std::string &rawRequest);

    std::string Stringify() const;

    static HttpRequest BadRequest();
};
// Possible better ways to do this
using HttpResponse = HttpRequest;