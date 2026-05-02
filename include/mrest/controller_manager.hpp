#pragma once

#include <algorithm>
#include <any>
#include <concepts>
#include <functional>
#include <glaze/glaze.hpp>
#include <memory>
#include <meta>
#include <nlohmann/detail/conversions/to_json.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "annotations/controller.hpp"
#include "annotations/request.hpp"
#include "request.hpp"
#include "util/reflection.hpp"

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
		static_assert(HasAnnotation<RestController>(^^T), "Object needs to have RestController annotation");
		// Use shared pointer for std::any's copy constructible requirement
		auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

		controllers.emplace_back(std::move(ptr));

		std::shared_ptr<T> &stored_ptr = std::any_cast<std::shared_ptr<T> &>(controllers.back());

		routes.emplace_back();

		std::string_view literal;
		template for(constexpr auto annotation: define_static_array(annotations_of(^^T))){
			if constexpr(SameAnnotation<RestController>(annotation)){
				literal = [:constant_of(annotation):].route.text;
			}
		}

		template for (constexpr auto func :
					  define_static_array(members_of(^^T, std::meta::access_context::current()))) {
			if constexpr (has_identifier(func) && HasAnnotation<Request>(func)) {
				template for (constexpr auto annotation :
							  define_static_array(annotations_of(func))) {
					if constexpr (SameAnnotation<Request>(annotation)) {
						constexpr auto req = [:constant_of(annotation):];
						RouteMetadata metadata = std::make_pair(std::string{req.method.text},
																std::string{literal} + std::string{req.route.text});

						routes.back()[metadata] = CreateHandler(metadata.first, metadata.second,
																MakeFunction(*stored_ptr, &[:func:]));
					}
				}
			}
		}

		return *stored_ptr;
	}

	template <typename T>
	void RemoveController(T *ptr) {
		auto it = std::find_if(
			controllers.begin(), controllers.end(),
			[ptr](const std::unique_ptr<std::any> &controller) { return controller.get() == ptr; });

		if (it != controllers.end()) {
			controllers.erase(it);
		}
	}

	std::optional<RouteHandler> GetPathHandler(std::string_view method,
											   std::string_view query) const;

   private:
	// {"<METHOD>", "<PATH>"}
	using RouteMetadata = std::pair<std::string, std::string>;
	struct HashPair {
		std::size_t operator()(const RouteMetadata &pair) const {
			const auto hash1{std::hash<std::string>{}(pair.first)};
			const auto hash2{std::hash<std::string>{}(pair.second)};

			// TODO: Better hashing of pairs
			return std::rotl(hash1, 1) ^ hash2;
		}
	};

	std::vector<std::any> controllers;
	std::vector<std::unordered_map<RouteMetadata, RouteHandler, HashPair>> routes;

	bool MatchRoute(std::string_view pattern, std::string_view path) const;

	template <typename R, typename... Args>
	RouteHandler CreateHandler(std::string_view requestMethod, std::string_view requestPattern,
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
					return glz::write_json(retVal);
				}
			}

			return std::string{};
		};
	}
	template <typename C, typename R, typename... Args>
	std::function<R(Args...)> MakeFunction(C &obj, R (C::*mf)(Args...)) {
		return [&obj, mf](Args... args) -> R { return (obj.*mf)(std::forward<Args>(args)...); };
	}
};