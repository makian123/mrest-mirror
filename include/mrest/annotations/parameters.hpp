#pragma once

#include "util/reflection.hpp"

struct RequestBody {};

struct RequestParam {
    StringLiteral name;
};

struct PathVariable {
    StringLiteral name;
};