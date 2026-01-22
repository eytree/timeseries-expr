#pragma once
#include <stdexcept>
#include <string_view>
#include "tsexpr/token.hpp"

namespace tsexpr {

struct ParseError : std::runtime_error { using std::runtime_error::runtime_error; };

class Lexer {
public:
    explicit Lexer(std::string_view s) : s_(s) {}
    Token next();

private:
    void skip_ws();
    bool is_end() const { return i_ >= s_.size(); }

    std::string_view s_;
    std::size_t i_{0};
};

} // namespace tsexpr
