#pragma once

#include <algorithm>
#include <any>
#include <concepts>
#include <functional>
#include <memory>
#include <nlohmann/detail/conversions/to_json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "request.hpp"

template <typename T>
concept HasToString = requires(T a) {
	{ std::to_string(a) } -> std::convertible_to<std::string>;
};
class ControllerManager {
   public:
	using RouteHandler = std::function<std::string(HttpRequest &)>;

   private:
	std::vector<std::any> controllers;
	std::unordered_map<std::string, RouteHandler> routes;

	bool MatchRoute(std::string_view pattern, std::string_view path) const;

   public:
	template <typename T, typename... Args>
	T &AddController(Args &&...args) {
		// Use shared pointer for std::any's copy constructible requirement
		auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

		controllers.emplace_back(std::move(ptr));

		std::shared_ptr<T> &stored_ptr =
			std::any_cast<std::shared_ptr<T> &>(controllers.back());
		return *stored_ptr;
	}

	// TODO: use reflection
	template <typename R, typename... Args>
	void AddHandler(std::string_view pattern,
						   std::function<R(Args...)> method) {
		routes[std::string{pattern}] = [method](HttpRequest &request) {
			std::optional<std::tuple<Args &...>> ref;

			auto set = [&]<auto I>(auto &e) {
				ref.emplace([&]<auto... idx>(std::index_sequence<idx...>) {
					return std::tie([&]<auto CurrentIdx>() -> auto & {
						if constexpr (I != CurrentIdx) {
							return std::get<CurrentIdx>(*ref);
						} else {
							return e;
						}
					}.template operator()<idx>()...);
				}(std::index_sequence_for<Args...>()));
			};

			if constexpr (std::is_same_v<void, R>) {
				if constexpr (sizeof...(Args) > 0)
					std::apply(method, *ref);
				else
					method();
			} else {
				R retVal{};
				if constexpr (sizeof...(Args) > 0)
					retVal = std::apply(method, *ref);
				else
					retVal = method();

				if constexpr (std::is_convertible_v<R, std::string>) {
					return std::string{retVal};
				} else if constexpr (HasToString<R>) {
					return std::to_string(retVal);
				}
				else {
					return nlohmann::json(retVal).dump();
				}
			}

			return std::string{};
		};
	}

	template <typename T>
	void RemoveController(T *ptr) {
		auto it =
			std::find_if(controllers.begin(), controllers.end(),
						 [ptr](const std::unique_ptr<std::any> &controller) {
							 return controller.get() == ptr;
						 });

		if (it != controllers.end()) {
			controllers.erase(it);
		}
	}

	std::optional<RouteHandler> GetPathHandler(std::string_view query) const;
};