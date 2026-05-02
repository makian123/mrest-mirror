#pragma once

#include <algorithm>
#include <any>
#include <concepts>
#include <functional>
#include <memory>
#include <nlohmann/detail/conversions/to_json.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "request.hpp"

template <typename T>
concept HasToString = requires(T a) {
	{ std::to_string(a) } -> std::convertible_to<std::string>;
};
class ControllerManager {
   public:
	using RouteHandler = std::function<std::string(HttpRequest &)>;

	template <typename T, typename... Args>
	// TODO: use reflection to extract all endpoint handlers
	T &AddController(Args &&...args) {
		// Use shared pointer for std::any's copy constructible requirement
		auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

		controllers.emplace_back(std::move(ptr));

		std::shared_ptr<T> &stored_ptr =
			std::any_cast<std::shared_ptr<T> &>(controllers.back());

		routes.emplace_back();

		template for(constexpr auto func: std::define_static_array(members_of(^^T, std::meta::access_context::current()))){
			if constexpr (has_identifier(func) && HasAnnotation<Request>(func)){
				template for(constexpr auto annotation: std::define_static_array(annotations_of(func))){
					if constexpr(SameAnnotation<Request>(annotation)){
						constexpr auto req = constant_of(annotation);
						RouteMetadata metadata{req.method, req.route};

						routes.back()[metadata] = CreateHandler(req.method, req.route, &[:(*stored_ptr)::func:]);
					}
				}
			}
		}

		return *stored_ptr;
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

	private:
	// {"<METHOD>", "<PATH>"}
	using RouteMetadata = std::pair<std::string, std::string>;
	struct HashPair {
		template<typename A, typename B>
		std::size_t operator()(const std::pair<A, B> &pair) const {
			const std::size_t hash1{std::hash<A>(pair.first)};
			const std::size_t hash2{std::hash<B>(pair.second)};

			// TODO: Better hashing of pairs
			return std::rotl(hash1, 1) ^ hash2;
		}
	};

	std::vector<std::any> controllers;
	std::vector<std::unordered_map<RouteMetadata, RouteHandler, HashPair>> routes;

	bool MatchRoute(std::string_view pattern, std::string_view path) const;

	template <typename R, typename... Args>
	RouteHandler CreateHandler(std::string_view requestMethod, 
					std::string_view requestPattern,
					std::function<R(Args...)> method) {
		return [method](HttpRequest &request) {
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
				} else {
					// TODO: use reflection serialization lib 
					return nlohmann::json(retVal).dump();
				}
			}

			return std::string{};
		};
	}
};