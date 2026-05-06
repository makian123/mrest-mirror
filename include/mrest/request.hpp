#pragma once

#include <chrono>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cookie.hpp"

namespace mrest {
class HttpSession {
	std::string id;
	std::unordered_map<std::string, std::string> attributes;

	std::chrono::system_clock::time_point creationTime;
	std::chrono::seconds expirationDuration;

   public:
	explicit HttpSession(std::int64_t expirationTimeSeconds = 0)
		: id(' ', 32),
		  creationTime{std::chrono::system_clock::now()},
		  expirationDuration{expirationTimeSeconds} {
		const std::string_view characters = "0123456789ABCDEF";
		std::random_device random_device;
		std::mt19937 generator(random_device());
		std::uniform_int_distribution<> distribution(0, characters.size() - 1);

		for (auto &ch : id) {
			ch = characters[distribution(generator)];
		}
	}
	HttpSession(const HttpSession &other)
		: id{other.id},
		  attributes{other.attributes},
		  creationTime{other.creationTime},
		  expirationDuration{other.expirationDuration} {}

	void Renew() { creationTime = std::chrono::system_clock::now(); }

	void SetAttribute(const std::string &key, std::string_view value) { attributes[key] = value; }
	std::optional<std::string_view> GetAttribute(const std::string key) const {
		if (auto it = attributes.find(key); it != attributes.end()) {
			return it->second;
		}
		return std::nullopt;
	}

	std::string_view GetId() const { return id; }

	operator bool() const {
		if (expirationDuration.count() == 0) {
			return true;
		}

		return std::chrono::system_clock::now() < (creationTime + expirationDuration);
	}
};
struct HttpHeader {
	std::string method;
	std::string path;
	std::string version = "HTTP/1.1";
	std::string statusCode;

	std::unordered_map<std::string, std::string> headers;
	std::unordered_map<std::string, std::string> parameters;
	CookieContainer cookies;

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
	HttpSession *session{nullptr};

	HttpRequest() = default;
	explicit HttpRequest(const std::string &rawRequest);
	HttpRequest(HttpSession &session, const std::string &rawRequest);

	std::string Stringify() const;

	static HttpRequest BadRequest(std::string_view message = "");
};
// Possible better ways to do this
using HttpResponse = HttpRequest;

std::optional<std::string> ExtractPathVariable(std::string_view pattern, std::string_view path,
											   std::size_t idx);
}  // namespace mrest