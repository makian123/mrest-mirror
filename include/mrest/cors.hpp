#pragma once

#include <string>
#include <vector>

#include "request.hpp"

class Cors {
   public:
	void SetAllowedMethods(const std::vector<std::string> &methods) {
		allowedMethods = std::accumulate(
			std::next(methods.begin()), methods.end(), methods[0],
			[](const std::string &a, const std::string &b) { return a + ", " + b; });
	}
	void SetAllowedHeaders(const std::vector<std::string> &headers) {
		allowedHeaders = std::accumulate(
			std::next(headers.begin()), headers.end(), headers[0],
			[](const std::string &a, const std::string &b) { return a + ", " + b; });
	}
	void SetAllowedOrigin(const std::string &origin) { allowedOrigin = origin; }

	void AppendToResponse(mrest::HttpResponse &response) {
		response.header.headers.emplace("Access-Control-Allow-Headers", allowedHeaders);
		response.header.headers.emplace("Access-Control-Allow-Methods", allowedMethods);
		response.header.headers.emplace("Access-Control-Allow-Origin", allowedOrigin);
	}

   private:
	std::string allowedMethods = "*";
	std::string allowedHeaders = "*";
	std::string allowedOrigin = "*";
};