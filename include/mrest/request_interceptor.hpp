#pragma once

#include <functional>
#include <memory>

#include "request.hpp"

class FilterChain {
   public:
	/** Requires all the filters to return true to allow a request.
	 * returns:
	 * `true` -> request allowed
	 * `false` -> request denied
	 */
	using FilterMethod = std::function<bool(HttpRequest &)>;

   private:
	FilterMethod filterMethod;
	std::unique_ptr<FilterChain> next{nullptr};

	bool DoNext(HttpRequest &request) const {
		return next ? next->Filter(request) : true;
	}

   public:
	FilterChain() = default;
	FilterChain(FilterMethod method) : filterMethod(method) {}

	bool Filter(HttpRequest &request) const {
		if (!filterMethod || filterMethod(request)) {
			return DoNext(request);
		}

		return false;
	}

	void AddBack(FilterMethod method) {
		if (next) {
			next->AddBack(method);
			return;
		}
		next = std::make_unique<FilterChain>(method);
	}
};