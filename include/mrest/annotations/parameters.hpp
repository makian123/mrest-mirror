#pragma once

#include "util/reflection.hpp"

struct RequestBody {};

struct RequestParam {
    StringLiteral name;

    consteval RequestParam() = default;
    consteval explicit RequestParam(std::string_view name): name(name){}
};

struct PathVariable {
    StringLiteral name;

    consteval PathVariable() = default;
    consteval explicit PathVariable(std::string_view name): name(name){}
};