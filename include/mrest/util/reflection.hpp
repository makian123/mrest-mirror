#pragma once

#include <meta>

struct StringLiteral {
	const char *text;

    consteval StringLiteral(): text{nullptr} {}
	consteval explicit StringLiteral(std::string_view inText)
		: text(define_static_string(inText)) {}
};

template<typename A>
consteval bool HasAnnotation(std::meta::info object){
    for(auto annotation: annotations_of(object)){
        if(is_convertible_type(type_of(annotation), ^^A)){
            return true;
        }
    }

    return false;
}
template<typename A>
consteval bool SameAnnotation(std::meta::info annotation){
    return is_convertible_type(type_of(annotation), ^^A);
}