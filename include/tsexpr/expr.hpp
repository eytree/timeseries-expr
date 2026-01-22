#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <tsexpr/timeseries_stub.hpp>

namespace ts::expr {

struct ParseError : std::runtime_error { using std::runtime_error::runtime_error; };
struct EvalError  : std::runtime_error { using std::runtime_error::runtime_error; };

using Env   = std::map<std::string, TimeSeries>;
using Value = std::variant<TimeSeries, double>;

enum class TokKind {
    Ident,
    Number,
    Comma,
    Plus,
    Minus,
    Star,
    Slash,
    LParen,
    RParen,
    Assign,
    End,
    Func, // internal: function call (postfix in RPN)
    Neg, // internal unary minus
};

struct Token {
    TokKind kind{};
    std::string text{}; // for Ident
    double number{};    // for Number
    int arity{0};       // for Func
};

struct Compiled {
    std::string target;     // assignment LHS
    std::vector<Token> rpn; // expression as Reverse Polish Notation
};

/// Compile "z = expr" into a target + RPN token stream.
/// Throws ParseError on malformed input.
Compiled compile(std::string_view input);

/// Evaluate an RPN program into a Value using the provided environment.
/// Throws EvalError on unknown variables or bad operations.
Value eval_rpn(const std::vector<Token>& rpn, const Env& env);

/// Compile + evaluate + assign back into env.
/// Assignment currently requires the expression to reduce to TimeSeries.
void execute_assignment(std::string_view input, Env& env);

} // namespace ts::expr
