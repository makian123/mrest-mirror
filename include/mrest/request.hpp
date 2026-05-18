#pragma once

#include <algorithm>
#include <chrono>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cookie.hpp"

namespace mrest {
struct Resource {
	std::string name;
	std::vector<char> data;
	std::optional<std::string> filename;
	std::optional<std::string> contentType;
};
struct MultipartParam {
	std::vector<Resource> parts;

	std::optional<const Resource *> GetPart(std::string_view partName) const {
		auto it = std::find_if(parts.begin(), parts.end(),
							   [partName](const Resource &part) { return part.name == partName; });

		return (it != parts.end()) ? std::optional(&(*it)) : std::nullopt;
	}
};

class HttpSession {
	std::string id;
	int connId;
	std::unordered_map<std::string, std::string> attributes;

	std::chrono::system_clock::time_point creationTime;
	std::chrono::seconds expirationDuration;

   public:
	HttpSession() = default;
	HttpSession(int connId, std::int64_t expirationTimeSeconds = 0)
		: id(' ', 32),
		  connId{connId},
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
		  connId{other.connId},
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
	int GetConnectionId() const { return connId; }

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
	std::string boundary{};

	HttpHeader() = default;
	explicit HttpHeader(std::span<const char> rawRequest);

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
			Multipart_FormData,
			Multipart_Mixed
		};

		std::string body;
		Type type = Type::JSON;
		MultipartParam multipartData;
	} body;
	HttpSession *session{nullptr};

	HttpRequest() = default;
	explicit HttpRequest(std::span<const char> rawRequest);
	HttpRequest(HttpSession &session, std::span<const char> rawRequest);
	HttpRequest(HttpSession &session, const HttpHeader &header, std::span<const char> rawBody);
	HttpRequest(const HttpRequest &other) = default;

	std::string Stringify() const;

	static HttpRequest BadRequest(std::string_view message = "");
	static HttpRequest Ok(std::string_view message = "");

   private:
	BodyInfo ParseBody(std::span<const char> body) const;
};
// Possible better ways to do this
using HttpResponse = HttpRequest;

std::optional<std::string> ExtractPathVariable(std::string_view pattern, std::string_view path,
											   std::size_t idx);
}  // namespace mrest