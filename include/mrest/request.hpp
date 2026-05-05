#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mrest {
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
	struct BodyInfo {
		enum class Type {
			JSON,
			PlainText,
			CSS,
			HTML,
			Javascript,
			Bytes,
		};

		std::string body;
		Type type = Type::JSON;
	} body;

	HttpRequest() = default;
	explicit HttpRequest(const std::string &rawRequest);

	std::string Stringify() const;

	static HttpRequest BadRequest(std::string_view message = "");
};
// Possible better ways to do this
using HttpResponse = HttpRequest;

std::optional<std::string> ExtractPathVariable(std::string_view pattern, std::string_view path,
											   std::size_t idx);
}  // namespace mrest