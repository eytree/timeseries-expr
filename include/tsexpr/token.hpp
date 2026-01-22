#pragma once
#include <string>

namespace tsexpr {

enum class TokKind {
    Ident,
    Number,

    Plus, Minus, Star, Slash,
    LParen, RParen,
    Comma,
    Assign,
    End,

    // internal
    Neg,   // unary -
    Func,  // function identifier (used during parsing)
};

struct Token {
    TokKind kind{TokKind::End};
    std::string text{}; // Ident / Func name
    double number{0.0}; // Number (or argc stored for Func token during RPN)
};

} // namespace tsexpr
