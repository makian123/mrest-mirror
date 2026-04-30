#pragma once

#include <string_view>

struct RequestBody {};

struct RequestParam {
    std::string_view name;
};

struct PathVariable {
    std::string_view name;
};