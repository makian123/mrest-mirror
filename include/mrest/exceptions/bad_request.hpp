#pragma once

#include <exception>
#include <string>
#include <string_view>

struct BadRequestException: std::exception {
    public:
    BadRequestException(std::string_view msg = ""): message{msg}{}
    ~BadRequestException() = default;

    virtual const char *what() const noexcept {
        return message.c_str();
    }

    private:
    std::string message;
};