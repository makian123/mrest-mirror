#include <cctype>
#include <chrono>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <request.hpp>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <util/search.hpp>
#include <vector>

namespace {
std::string_view Trim(std::string_view sv) {
	size_t start = 0;
	size_t end = sv.size();

	while (start < end && isspace(sv[start])) ++start;
	while (end > start && isspace(sv[end - 1])) --end;

	return sv.subview(start, end - start);
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
	{BodyType::Bytes, "application/octet-stream"},
	{BodyType::Multipart_FormData, "multipart/form-data"},
	{BodyType::Multipart_Mixed, "multipart/mixed"}};

std::vector<mrest::Resource> ExtractMultiparts(std::string_view boundary,
											   std::span<const char> data) {
	std::vector<mrest::Resource> parts;
	auto boundaryIt = mrest::util::SearchSubstring(data, boundary);
	while (boundaryIt != data.end()) {
		if (*(boundaryIt + 1) == '-' && *(boundaryIt + boundary.length() + 2) == '-') {
			break;
		}
		mrest::Resource part{};

		auto nextBoundary = mrest::util::SearchSubstring(data, boundary, boundaryIt + 1);
		auto partFull = Trim(std::string_view{boundaryIt + boundary.length(), nextBoundary - 2});
		if (partFull.empty()) {
			break;
		}

		auto firstLine = Trim(partFull.subview(0, partFull.find("\n")));
		partFull = partFull.subview(partFull.find("\n") + 1);

		auto secondLine = Trim(partFull.subview(0, partFull.find("\n")));

		auto bodyIt = partFull.find("\r\n\r\n");
		if (bodyIt != partFull.npos) {
			partFull = partFull.subview(bodyIt + 4);
		}

		std::string_view &contentType =
			firstLine.starts_with("Content-Type") ? firstLine : secondLine;
		std::string_view &contentDisposition =
			firstLine.starts_with("Content-Disposition") ? firstLine : secondLine;

		if (contentType.starts_with("Content-Type")) {
			part.contentType = contentType.substr(contentType.find(": ") + 2);
		}

		std::vector<std::string_view> segments =
			SplitString(contentDisposition.subview(contentDisposition.find(": ") + 2), ';');
		for (auto &segment : segments) {
			std::vector<std::string_view> keyValue = SplitString(Trim(segment), '=');
			if (keyValue.size() != 2 || keyValue[1].size() <= 2) {
				continue;
			}

			std::string_view view = keyValue[1];
			// convert \"text\" -> text
			if (view.starts_with('"') && view.ends_with('"')) {
				view = view.subview(1, keyValue[1].size() - 2);
			}

			if (keyValue[0] == "name") {
				part.name = view;
			} else if (keyValue[0] == "filename") {
				part.filename = view;
			}
		}

		const auto len = partFull.length();
		part.data.resize(partFull.length());
		partFull.copy(part.data.data(), partFull.length());

		parts.push_back(part);
		boundaryIt = nextBoundary;
	}

	return parts;
}
}  // namespace

namespace mrest {
HttpHeader::HttpHeader(std::span<const char> rawRequest) {
	std::string headerPart{rawRequest.begin(), util::SearchSubstring(rawRequest, "\r\n\r\n")};
	std::istringstream stream{headerPart};
	std::string line;

	stream >> method >> path >> version;

	// Deal with headers
	while (std::getline(stream, line)) {
		if (line.ends_with("\r\n\r")) {
			break;
		}
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

		// Multipart format: "Content-Type: multipart/XYZ; boundary=..."
		if (key == "Content-Type" && value.starts_with("multipart")) {
			auto boundaryIt = value.find("boundary=");
			boundary = value.substr(boundaryIt + sizeof("boundary=") - 1);
			value.erase(boundaryIt - 2);
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
			auto paramsEnd = std::prev(parameters.end());
			for (auto it = parameters.begin(); it != parameters.end(); ++it) {
				ss << it->first << ' ' << it->second;

				if (it != paramsEnd) {
					ss << '&';
				}
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

	for (auto &cookie : cookies.GetAllCookies()) {
		ss << cookieHeaderKey << ": " << cookie.Stringify() << "\r\n";
	}

	return ss.str();
}

HttpRequest::HttpRequest(std::span<const char> rawRequest) : header{rawRequest} {
	std::println("Request received: {}", rawRequest.data());
	auto bodyIt = util::SearchSubstring(rawRequest, "\r\n\r\n");
	if (bodyIt == rawRequest.end()) {
		return;
	}

	this->body = ParseBody(rawRequest.subspan(std::distance(rawRequest.begin(), bodyIt)));
}
HttpRequest::HttpRequest(HttpSession &session, std::span<const char> rawRequest)
	: HttpRequest(rawRequest) {
	this->session = &session;
}
HttpRequest::HttpRequest(HttpSession &session, const HttpHeader &header,
						 std::span<const char> rawBody)
	: session(&session), header(header), body(ParseBody(rawBody)) {}

std::string HttpRequest::Stringify() const {
	std::stringstream ss;
	ss << header.Stringify();

	const auto bodySize = body.GetSize();
	ss << "content-length: " << bodySize << "\r\n";

	if (bodySize) {
		ss << std::format("content-type: {}\r\n", bodyTypes.at(body.type));
		std::visit([&ss, &bodySize](auto &&arg) {
			using T = std::decay_t<decltype(arg)>;

			if constexpr(std::is_same_v<T, std::string>){
				ss << "\r\n" << arg;
			}
			else if constexpr(std::is_same_v<T, MultipartParam>){
				// Todo: multipart
			}
			else {
				ss << "content-disposition: attachment";
				if(arg.filename){
					ss << "; filename=\"" << *arg.filename << '"';
				}
				ss << "\r\n\r\n";
				ss.write(arg.data.data(), bodySize);
			}
		}, body.value);
	} else {
		ss << "\r\n";
	}

	return ss.str();
}

HttpRequest HttpRequest::BadRequest(std::string_view msg) {
	HttpRequest ret;
	ret.header.statusCode = "400 BAD REQUEST";
	ret.header.headers["Connection"] = "close";
	if (!msg.empty()) {
		ret.body = {BodyType::PlainText, std::string{msg}};
	}

	return ret;
}

HttpRequest HttpRequest::Ok(std::string_view msg) {
	HttpRequest ret;
	ret.header.statusCode = "200 OK";
	ret.header.headers["Connection"] = "close";
	if (!msg.empty()) {
		ret.body = {BodyType::PlainText, std::string{msg}};
	}
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

HttpRequest::BodyInfo HttpRequest::ParseBody(std::span<const char> body) const {
	HttpRequest::BodyInfo retBody;
	if (header.headers.contains("content-type")) {
		auto contentType = header.headers.at("content-type");

		BodyType type;
		for (const auto &[bodyType, value] : bodyTypes) {
			if (value == contentType) {
				type = bodyType;
				break;
			}
		}

		if (type == BodyType::Multipart_FormData || type == BodyType::Multipart_Mixed) {
			auto multiparts = ExtractMultiparts(header.boundary, body);
			retBody = BodyInfo{type, MultipartParam{multiparts}};
		} else {
			retBody = BodyInfo{type, std::string{body.begin(), body.end()}};
		}
	}

	return retBody;
}
}  // namespace mrest