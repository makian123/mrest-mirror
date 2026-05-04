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
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "annotations/controller.hpp"
#include "annotations/parameters.hpp"
#include "annotations/request.hpp"
#include <exceptions/exceptions.hpp>
#include "request.hpp"
#include "util/reflection.hpp"

template <typename T>
concept HasToString = requires(T a) {
	{ std::to_string(a) } -> std::convertible_to<std::string>;
};
class ControllerManager {
   public:
	using RouteHandler = std::function<HttpResponse::BodyInfo(HttpRequest &)>;

	template <typename T, typename... Args>
	// TODO: use reflection to extract all endpoint handlers
	T &AddController(Args &&...args) {
		static_assert(HasAnnotation<RestController>(^^T),
					  "Object needs to have RestController annotation");
		// Use shared pointer for std::any's copy constructible requirement
		auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

		controllers.emplace_back(std::move(ptr));

		std::shared_ptr<T> &stored_ptr = std::any_cast<std::shared_ptr<T> &>(controllers.back());

		routes.emplace_back();

		std::string_view literal;
		template for (constexpr auto annotation : define_static_array(annotations_of(^^T))) {
			if constexpr (SameAnnotation<RestController>(annotation)) {
				literal = [:constant_of(annotation):].route.text;
			}
		}

		template for (constexpr auto func :
					  define_static_array(members_of(^^T, std::meta::access_context::current()))) {
			if constexpr (has_identifier(func) && HasAnnotation<Request>(func)) {
				template for (constexpr auto annotation :
							  define_static_array(annotations_of(func))) {
					if constexpr (SameAnnotation<Request>(annotation)) {
						std::unordered_map<std::string, int> queryIndices;
						std::unordered_map<std::string, int> pathVariables;
						constexpr int bodyIdx =
							AnnotationPos<decltype(RequestBody)>(parameters_of(func));

						constexpr auto params = define_static_array(parameters_of(func));
						template for (constexpr auto idx :
									  make_index_array(std::make_index_sequence<params.size()>{})) {
							if constexpr (HasAnnotation<RequestParam>(params[idx]) || HasAnnotation<PathVariable>(params[idx])) {
								template for (constexpr auto paramAnnotation :
											  define_static_array(annotations_of(params[idx]))) {
									if constexpr (SameAnnotation<RequestParam>(paramAnnotation)) {
										queryIndices.emplace(
											std::string{[:constant_of(paramAnnotation):].name.text},
											idx);
									}
									else if constexpr (SameAnnotation<PathVariable>(paramAnnotation)) {
										pathVariables.emplace(
											std::string{[:constant_of(paramAnnotation):].name.text},
											idx);
									}
								}
							}
						}

						constexpr auto req = [:constant_of(annotation):];
						RouteMetadata metadata =
							std::make_pair(std::string{req.method.text},
										   std::string{literal} + std::string{req.route.text});

						routes.back()[metadata] = CreateHandler(
							metadata.first, metadata.second, MakeFunction(*stored_ptr, &[:func:]),
							queryIndices, pathVariables, bodyIdx);
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
			auto idx = std::distance(controllers.begin(), it);
			routes.erase(routes.begin() + idx);
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
	RouteHandler CreateHandler(std::string_view requestMethod, const std::string &requestPattern,
							   std::function<R(Args...)> method,
							   const std::unordered_map<std::string, int> &queryIndices,
							   const std::unordered_map<std::string, int> &pathVariables,
							   int bodyIdx) {
								std::string pattern{requestPattern};
		return [=](HttpRequest &request) {
			// TODO: remove forced default initialization
			std::tuple<std::remove_cvref_t<Args>...> ref{};

			auto set = [&]<auto I>(auto &e){
				std::get<I>(ref) = std::forward<decltype(e)>(e);
			};

			// Pre-process the request
			if constexpr (sizeof...(Args) > 0) {
				template for (constexpr auto idx :
							  make_index_array(std::make_index_sequence<sizeof...(Args)>{})) {
					if (bodyIdx == idx) {
						auto body =
							glz::read_json<std::remove_cvref_t<Args...[idx]>>(request.body.body)
								.value();
						set.template operator()<idx>(body);
						continue;
					}

					auto queryIt = std::find_if(queryIndices.begin(), queryIndices.end(),
												[](const std::pair<std::string, int> &query) {
													return query.second == idx;
												});
					if (queryIt != queryIndices.end()) {
						if constexpr (std::is_convertible_v<std::remove_cvref_t<Args...[idx]>,
															std::string>) {
							set.template operator()<idx>(request.header.parameters[queryIt->first]);
						} else if constexpr (std::is_fundamental_v<
												 std::remove_cvref_t<Args...[idx]>>) {
							set.template operator()<idx>(
								std::stoull(request.header.parameters[queryIt->first]));
						}
						continue;
					}

					auto pVarIt = std::find_if(pathVariables.begin(), pathVariables.end(),
											   [](const std::pair<std::string, int> &pathSegment) {
												   return pathSegment.second == idx;
											   });
					if (pVarIt != pathVariables.end()) {
						auto pathVarNumber = std::distance(pathVariables.begin(), pVarIt);
						auto extractedPathVar = ExtractPathVariable(pattern, request.header.path, pathVarNumber);
						if(!extractedPathVar){
							throw BadRequestException("Could not extract path variable");
						}
						if constexpr (std::is_convertible_v<std::remove_cvref_t<Args...[idx]>,
															std::string>) {
							set.template operator()<idx>(*extractedPathVar);
						} else if constexpr (std::is_fundamental_v<
												 std::remove_cvref_t<Args...[idx]>>) {
							set.template operator()<idx>(std::stoull(*extractedPathVar));
						}
						continue;
					}
				}
			}

			// Call handler and return
			if constexpr (std::is_same_v<void, R>) {
				if constexpr (sizeof...(Args) > 0)
					std::apply(method, ref);
				else
					method();
			} else {
				R retVal{};
				if constexpr (sizeof...(Args) > 0)
					retVal = std::apply(method, ref);
				else
					retVal = method();

				if constexpr (std::is_convertible_v<R, std::string>) {
					return HttpResponse::BodyInfo{std::string{retVal},
												  HttpResponse::BodyInfo::Type::PlainText};
				} else if constexpr (HasToString<R>) {
					return HttpResponse::BodyInfo{std::to_string(retVal),
												  HttpResponse::BodyInfo::Type::PlainText};
				} else {
					return HttpResponse::BodyInfo{glz::write_json(retVal).value(),
												  HttpResponse::BodyInfo::Type::JSON};
				}
			}

			return HttpResponse::BodyInfo{};
		};
	}
	template <typename C, typename R, typename... Args>
	static std::function<R(Args...)> MakeFunction(C &obj, R (C::*mf)(Args...)) {
		return [&obj, mf](Args... args) -> R { return (obj.*mf)(std::forward<Args>(args)...); };
	}

	template <std::size_t... I>
	static constexpr auto make_index_array(std::index_sequence<I...>) {
		return std::array{I...};
	}
};