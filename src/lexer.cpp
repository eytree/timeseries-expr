#include "tsexpr/lexer.hpp"
#include <cctype>
#include <cstdlib>

namespace tsexpr {

static bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
static bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

void Lexer::skip_ws() {
    while (!is_end() && std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_;
}

Token Lexer::next() {
    skip_ws();
    if (is_end()) return {TokKind::End};

    char c = s_[i_];

    switch (c) {
        case '+': ++i_; return {TokKind::Plus};
        case '-': ++i_; return {TokKind::Minus};
        case '*': ++i_; return {TokKind::Star};
        case '/': ++i_; return {TokKind::Slash};
        case '(': ++i_; return {TokKind::LParen};
        case ')': ++i_; return {TokKind::RParen};
        case ',': ++i_; return {TokKind::Comma};
        case '=': ++i_; return {TokKind::Assign};
        default: break;
    }

    // backtick-quoted identifier: `anything here` (no escaping in this minimal version)
    if (c == '`') {
        ++i_;
        std::size_t start = i_;
        while (!is_end() && s_[i_] != '`') ++i_;
        if (is_end()) throw ParseError("Unterminated backtick identifier");
        Token t{TokKind::Ident};
        t.text = std::string(s_.substr(start, i_ - start));
        ++i_; // consume closing `
        return t;
    }

    if (is_ident_start(c)) {
        std::size_t start = i_++;
        while (!is_end() && is_ident_char(s_[i_])) ++i_;
        Token t{TokKind::Ident};
        t.text = std::string(s_.substr(start, i_ - start));
        return t;
    }

    if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
        const char* begin = s_.data() + i_;
        char* end = nullptr;
        double v = std::strtod(begin, &end);
        if (end == begin) throw ParseError("Invalid number");
        i_ += static_cast<std::size_t>(end - begin);
        Token t{TokKind::Number};
        t.number = v;
        return t;
    }

    throw ParseError(std::string("Unexpected character: '") + c + "'");
}

} // namespace tsexpr
