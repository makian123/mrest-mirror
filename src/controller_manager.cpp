#include "controller_manager.hpp"

#include <optional>
#include <vector>

namespace {
std::vector<std::string> SplitPath(std::string_view path) {
	std::vector<std::string> segments;

	std::size_t start = 0, end = 0;
	while ((end = path.find('/', start)) != std::string::npos) {
		if (end != start) {
			segments.push_back(std::string{path.substr(start, end - start)});
		}

		start = end + 1;
	}

	if (start < path.size()) {
		segments.push_back(std::string{path.substr(start)});
	}

	return segments;
}
}  // namespace

using RouteHandler = ControllerManager::RouteHandler;

bool ControllerManager::MatchRoute(std::string_view pattern,
								   std::string_view path) const {
	auto patternSegments = SplitPath(pattern);
	auto pathSegments = SplitPath(path);

	if (patternSegments.size() != pathSegments.size()) {
		return false;
	}

	for (std::size_t i = 0; i < patternSegments.size(); ++i) {
		const auto &pattern = patternSegments[i];
		const auto &path = pathSegments[i];

		if (!pattern.empty() && pattern.front() == '{' &&
			pattern.back() == '}' && !path.empty()) {
			continue;
		}
		if (pattern != path) {
			return false;
		}
	}

	return true;
}
std::optional<RouteHandler> ControllerManager::GetPathHandler(
	std::string_view query) const {
	auto routeFound = std::find_if(
		routes.begin(), routes.end(),
		[query, this](const std::pair<std::string, RouteHandler> &route) {
			return MatchRoute(query, route.first);
		});

	if (routeFound != routes.end()) {
		return routeFound->second;
	}
	return std::nullopt;
}