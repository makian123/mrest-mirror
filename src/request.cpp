#include <cctype>
#include <chrono>
#include <optional>
#include <print>
#include <ranges>
#include <request.hpp>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {
std::string_view Trim(std::string_view sv) {
	size_t start = 0;
	size_t end = sv.size();

	while (start < end && isspace(sv[start])) ++start;
	while (end > start && isspace(sv[end - 1])) --end;

	return sv.substr(start, end - start);
}
std::string &ToLowercase(std::string &str) {
	for (auto &ch : str) {
		ch = std::tolower(ch);
	}

	return str;
}
std::vector<std::string_view> SplitString(std::string_view str, char delim) {
	std::vector<std::string_view> segments;

	if (str.starts_with(delim)) {
		str = str.substr(1);
	}
	auto it = str.find(delim);
	while (it != std::string_view::npos) {
		segments.push_back(str.substr(0, it));
		str = str.substr(it + 1);

		it = str.find(delim);
	}
	if (str.size()) {
		segments.push_back(str);
	}

	return segments;
}

using BodyType = mrest::HttpRequest::BodyInfo::Type;
const std::unordered_map<BodyType, std::string> bodyTypes{
	{BodyType::JSON, "application/json"},
	{BodyType::PlainText, "text/plain"},
	{BodyType::CSS, "text/css"},
	{BodyType::HTML, "text/html"},
	{BodyType::Javascript, "text/javascript"},
	{BodyType::Bytes, "application/octet-stream"}

};
}  // namespace

namespace mrest {
HttpHeader::HttpHeader(const std::string &rawRequest) {
	std::istringstream stream{rawRequest};
	std::string line;

	stream >> method >> path >> version;

	// Deal with headers
	while (std::getline(stream, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}

		if (line.empty()) {
			continue;
		}

		auto delimPos = line.find(':');
		if (delimPos == std::string::npos) {
			continue;
		}

		std::string key{Trim(std::string_view(line.c_str(), delimPos))};
		std::string value{
			Trim(std::string_view(line.c_str() + delimPos + 1, line.size() - delimPos - 1))};

		if (key == "Cookie" || key == "Set-Cookie") {
			cookies.AddCookie(Cookie{value});
			continue;
		}

		headers[ToLowercase(key)] = std::string{value};
	}

	// Parameter trimming
	auto paramStartIt = path.find('?');
	if (paramStartIt == std::string::npos) {
		return;
	}

	auto paramString = path.substr(paramStartIt + 1);
	std::istringstream paramStream{paramString};
	std::string pair;
	while (std::getline(paramStream, pair, '&')) {
		auto eqIt = pair.find('=');
		if (eqIt == std::string::npos) {
			continue;
		}

		std::string key{Trim(std::string_view(pair.c_str(), eqIt))};
		std::string_view value{
			Trim(std::string_view(pair.c_str() + eqIt + 1, pair.size() - eqIt - 1))};
		parameters[ToLowercase(key)] = value;
	}
	path.erase(paramStartIt);
}
std::string HttpHeader::Stringify() const {
	std::stringstream ss;

	std::string cookieHeaderKey = statusCode.empty() ? "Cookie" : "Set-Cookie";
	// Request headers
	if (statusCode.empty()) {
		ss << method << ' ' << path;

		if (parameters.size()) {
			ss << '?';
		}
		auto paramsEnd = std::prev(parameters.end());
		for (auto it = parameters.begin(); it != parameters.end(); ++it) {
			ss << it->first << ' ' << it->second;

			if (it != paramsEnd) {
				ss << '&';
			}
		}

		ss << ' ' << version << "\r\n";
	}
	// Response headers
	else {
		ss << version << ' ' << statusCode << "\r\n";
	}

	for (auto &[key, value] : headers) {
		ss << key << ": " << value << "\r\n";
	}

	for(auto &cookie: cookies.GetAllCookies()) {
		ss << cookieHeaderKey << ": " << cookie.Stringify() << "\r\n";
	}

	return ss.str();
}

HttpRequest::HttpRequest(const std::string &rawRequest) : header{rawRequest} {
	auto bodyIt = rawRequest.find("\r\n\r\n");
	if (bodyIt == std::string::npos) {
		return;
	}

	if (header.headers.contains("content-type")) {
		auto contentType = header.headers["content-type"];
		BodyType type;
		for (const auto &[bodyType, value] : bodyTypes) {
			if (value == contentType) {
				type = bodyType;
				break;
			}
		}

		body = BodyInfo{std::string{rawRequest.begin() + bodyIt + 2, rawRequest.end()}, type};
	}
}
HttpRequest::HttpRequest(HttpSession &session, const std::string &rawRequest)
	: HttpRequest(rawRequest) {
	this->session = &session;
}
std::string HttpRequest::Stringify() const {
	std::stringstream ss;
	ss << header.Stringify();

	ss << "content-length: " << body.body.size() << "\r\n";
	if (!body.body.empty()) {
		ss << std::format("content-type: {}\r\n", bodyTypes.at(body.type));
		ss << "\r\n" << body.body;
	} else {
		ss << "\r\n";
	}

	return ss.str();
}

HttpRequest HttpRequest::BadRequest(std::string_view msg) {
	HttpRequest ret;
	ret.header.statusCode = "400 BAD REQUEST";
	ret.header.headers["Connection"] = "close";
	ret.body = {std::string{msg}, BodyType::PlainText};

	return ret;
}

std::optional<std::string> ExtractPathVariable(std::string_view pattern, std::string_view path,
											   std::size_t idx) {
	auto patternSegments = SplitString(pattern, '/');
	auto pathSegments = SplitString(path, '/');

	if (pathSegments.size() != pathSegments.size()) {
		return std::nullopt;
	}

	for (auto patternIt = patternSegments.begin(), pathIt = pathSegments.begin();
		 patternIt != patternSegments.end() && pathIt != pathSegments.end();
		 ++patternIt, ++pathIt) {
		if ((*patternIt).starts_with('{') && (*patternIt).ends_with('}')) {
			if (idx > 0) {
				idx--;
				continue;
			}
			return std::string{*pathIt};
		}
	}

	return std::nullopt;
}
}  // namespace mrest