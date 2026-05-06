#pragma once

#include <chrono>
#include <string>

namespace mrest {
struct Cookie {
	enum class SameSiteType { Lax, Strict, None };

	std::string key;
	std::string value;
	std::optional<std::chrono::time_point<std::chrono::system_clock>> expiry;
	std::optional<int> maxAgeSecs;
	std::optional<std::string> domain;
	std::optional<std::string> path;
	bool secure{false};
	bool httpOnly{false};
	std::optional<SameSiteType> sameSite;

	Cookie(std::string_view key, std::string_view val);
	Cookie(std::string_view rawCookie);

	std::string Stringify() const;
};
class CookieContainer {
	std::vector<Cookie> cookies;

   public:
   CookieContainer() = default;
	CookieContainer(const std::vector<Cookie>& cookies) : cookies{cookies} {}

    // Do not store the ptr for long time use
	std::optional<const Cookie*> GetCookie(std::string_view key) const {
		auto it = std::find_if(cookies.begin(), cookies.end(),
							   [key](const Cookie& cookie) { return cookie.key == key; });

		return it != cookies.end() ? std::optional<const Cookie*>(&(*it)) : std::nullopt;
	}
    void AddCookie(Cookie &&cookie) {
        cookies.emplace_back(std::forward<Cookie&&>(cookie));
    }
    void RemoveCookie(std::string_view key){
        cookies.erase(std::remove_if(cookies.begin(), cookies.end(), [key](const Cookie &cookie){ return cookie.key == key; }));
    }

	const std::vector<Cookie> &GetAllCookies() const {
		return cookies;
	}
};
}  // namespace mrest