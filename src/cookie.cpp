#include "cookie.hpp"

#include <chrono>
#include <ctime>
#include <exception>
#include <iomanip>
#include <optional>
#include <sstream>

#include "util/reflection.hpp"

namespace {
std::string ExtractCookievalue(std::string_view str) {
    return std::string{str.subview(0, str.find(';'))};
}
std::optional<std::pair<std::string, std::string>> ExtractCookieKvP(std::string_view str) {
	std::pair<std::string, std::string> keyValue;

	auto keyIt = str.find('=');
	if (keyIt == std::string_view::npos) return std::nullopt;
	keyValue.first = std::string{str.subview(0, keyIt)};

	auto valIt = str.find(';');
	keyValue.second = std::string{str.subview(keyIt + 1, valIt)};

	return keyValue;
}
}  // namespace

namespace mrest {
Cookie::Cookie(std::string_view key, std::string_view val) : key{key}, value{val} {}
Cookie::Cookie(std::string_view rawCookie) {
	auto keyValue = ExtractCookieKvP(rawCookie);
	if (!keyValue) {
		// TODO: do this properly
		throw std::exception();
	}
	this->key = keyValue->first;
	this->value = keyValue->second;
	auto separator = rawCookie.find(';');
	if(separator == std::string_view::npos){
		return;
	}
	
	rawCookie = rawCookie.subview(separator + 1);

	while (rawCookie.size() > 1) {
		keyValue = ExtractCookieKvP(rawCookie);
		if (keyValue) {
            if(keyValue->first == "Max-Age"){
                maxAgeSecs = std::stoi(keyValue->second);
            }
            else if(keyValue->first == "Domain"){
                domain = keyValue->second;
            }
            else if(keyValue->first == "Path"){
                path = keyValue->second;
            }
            else if(keyValue->first == "SameSite"){
                sameSite = *util::StringToEnum<SameSiteType>(keyValue->second);
            }
		} else {
            auto singleValue = ExtractCookievalue(rawCookie);

            if(singleValue == "Secure"){
                secure = true;
            }
            else if(singleValue == "HttpOnly"){
                httpOnly = true;
            }
		}
		rawCookie = rawCookie.subview(rawCookie.find(';') + 1);
	}
}
std::string Cookie::Stringify() const {
	std::stringstream ss;

	ss << key << '=' << value;

	if (expiry) {
		auto time = std::chrono::system_clock::to_time_t(*expiry);
		std::tm *tm = std::localtime(&time);

		ss << "; Expires=" << std::put_time(tm, "%a, %d %b %Y %H:%M:%S GMT");
	}
	if (maxAgeSecs) {
		ss << "; Max-Age=" << *maxAgeSecs;
	}
	if (domain) {
		ss << "; Domain=" << *domain;
	}
	if (path) {
		ss << "; Path=" << *path;
	}
	if (secure) {
		ss << "; Secure";
	}
	if (httpOnly) {
		ss << "; HttpOnly";
	}
	if (sameSite) {
		ss << "; SameSite=" << util::EnumToString(*sameSite);
	}

	return ss.str();
}
}  // namespace mrest