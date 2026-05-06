#pragma once

#include <meta>
#include <string_view>
#include <type_traits>
#include <optional>

namespace mrest {
namespace util {
struct StringLiteral {
	const char *text;

	consteval StringLiteral() : text{nullptr} {}
	consteval explicit StringLiteral(std::string_view inText)
		: text(std::define_static_string(inText)) {}
};

template <typename A>
consteval bool HasAnnotation(std::meta::info object) {
	for (auto annotation : annotations_of(object)) {
		if (is_convertible_type(type_of(annotation), ^^A)) {
			return true;
		}
	}

	return false;
}
template <typename A>
consteval bool SameAnnotation(std::meta::info annotation) {
	return is_convertible_type(type_of(annotation), ^^A);
}
template <typename A>
consteval int AnnotationPos(std::vector<std::meta::info> annotations) {
	for (std::size_t i = 0; i < annotations.size(); ++i) {
		if (HasAnnotation<A>(annotations[i])) {
			return i;
		}
	}
	return -1;
}

template<typename E>
requires std::is_enum_v<E>
std::string_view EnumToString(E val){
	template for(constexpr auto &enumerator: define_static_array(enumerators_of(^^E))){
		if(val == [:enumerator:]){
			return std::meta::display_string_of(enumerator);
		}
	}

	return "<unnamed>";
}
template<typename E>
requires std::is_enum_v<E>
std::optional<E> StringToEnum(std::string_view val){
	template for(constexpr auto &enumerator: define_static_array(enumerators_of(^^E))){
		if(val == display_string_of(enumerator)){
			return [:enumerator:];
		}
	}

	return std::nullopt;
}
}  // namespace reflection
}  // namespace mrest