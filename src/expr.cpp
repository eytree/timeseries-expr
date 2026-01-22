#include <tsexpr/expr.hpp>

#include <cctype>
#include <cstdlib>
#include <optional>
#include <utility>

namespace ts::expr {

// -----------------------------
// Lexer
// -----------------------------
static bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
static bool is_ident_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

class Lexer {
public:
    explicit Lexer(std::string_view s) : s_(s) {}

    Token peek() {
        if (!lookahead_) lookahead_ = next_impl();
        return *lookahead_;
    }

    Token next() {
        if (lookahead_) {
            Token t = std::move(*lookahead_);
            lookahead_.reset();
            return t;
        }
        return next_impl();
    }

private:
    Token next_impl() {
        skip_ws();
        if (i_ >= s_.size()) return {TokKind::End};

        char c = s_[i_];

        switch (c) {
            case ',': ++i_; return {TokKind::Comma};
            case '+': ++i_; return {TokKind::Plus};
            case '-': ++i_; return {TokKind::Minus};
            case '*': ++i_; return {TokKind::Star};
            case '/': ++i_; return {TokKind::Slash};
            case '(': ++i_; return {TokKind::LParen};
            case ')': ++i_; return {TokKind::RParen};
            case '=': ++i_; return {TokKind::Assign};
            default: break;
        }

        // backtick-quoted identifier (allows spaces/symbols)
        if (c == '`') {
            ++i_;
            std::size_t start = i_;
            while (i_ < s_.size() && s_[i_] != '`') ++i_;
            if (i_ >= s_.size()) throw ParseError("Unterminated backtick identifier");
            Token t{TokKind::Ident};
            t.text = std::string(s_.substr(start, i_ - start));
            ++i_; // consume closing '`'
            return t;
        }

        if (is_ident_start(c)) {
            std::size_t start = i_++;
            while (i_ < s_.size() && is_ident_char(s_[i_])) ++i_;
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

    void skip_ws() {
        while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_;
    }

    std::string_view s_;
    std::size_t i_{0};
    std::optional<Token> lookahead_{};
};

// -----------------------------
// Shunting-yard helpers
// -----------------------------
static int precedence(TokKind k) {
    switch (k) {
        case TokKind::Neg:   return 3;
        case TokKind::Star:
        case TokKind::Slash: return 2;
        case TokKind::Plus:
        case TokKind::Minus: return 1;
        default:             return 0;
    }
}

static bool is_right_associative(TokKind k) {
    return k == TokKind::Neg;
}

static bool is_operator(TokKind k) {
    return k == TokKind::Plus || k == TokKind::Minus || k == TokKind::Star || k == TokKind::Slash || k == TokKind::Neg;
}

static bool is_function(TokKind k) {
    return k == TokKind::Func;
}

// -----------------------------
// compile
// -----------------------------
Compiled compile(std::string_view input) {
    Lexer lex(input);

    Token lhs = lex.next();
    if (lhs.kind != TokKind::Ident) throw ParseError("Expected assignment target identifier at start");
    Token eq = lex.next();
    if (eq.kind != TokKind::Assign) throw ParseError("Expected '=' after assignment target");

    std::vector<Token> output;
    std::vector<Token> opstack;

    struct FuncCtx {
        int commas = 0;      // number of ',' seen at depth==1
        int depth = 0;       // paren nesting inside function call
        bool saw_expr = false; // saw start of an expression in the call
    };
    std::vector<FuncCtx> func_stack;

    bool expect_operand = true; // for unary '-'

    for (;;) {
        Token t = lex.next();
        if (t.kind == TokKind::End) break;

        // Ident: may be variable, or function name if followed by '('
        if (t.kind == TokKind::Ident) {
            if (lex.peek().kind == TokKind::LParen) {
                // Function call. Push function marker then consume '(' and push it.
                Token f{TokKind::Func};
                f.text = std::move(t.text);
                opstack.push_back(std::move(f));

                Token lp = lex.next(); // consume '('
                opstack.push_back(std::move(lp));
                func_stack.push_back(FuncCtx{0, 1, false});
                expect_operand = true;
                continue;
            }
            output.push_back(std::move(t));
            if (!func_stack.empty() && func_stack.back().depth == 1 && expect_operand) func_stack.back().saw_expr = true;
            expect_operand = false;
            continue;
        }

        if (t.kind == TokKind::Number) {
            output.push_back(std::move(t));
            if (!func_stack.empty() && func_stack.back().depth == 1 && expect_operand) func_stack.back().saw_expr = true;
            expect_operand = false;
            continue;
        }

        if (t.kind == TokKind::Comma) {
            if (func_stack.empty() || func_stack.back().depth != 1) {
                throw ParseError("Unexpected ',' (commas are only valid inside function argument lists)");
            }
            // Pop operators until the matching '('
            while (!opstack.empty() && opstack.back().kind != TokKind::LParen) {
                output.push_back(std::move(opstack.back()));
                opstack.pop_back();
            }
            if (opstack.empty()) throw ParseError("Unexpected ','");
            func_stack.back().commas += 1;
            expect_operand = true;
            continue;
        }

        if (t.kind == TokKind::LParen) {
            opstack.push_back(std::move(t));
            if (!func_stack.empty()) {
                // If we're currently parsing a function call, increase nesting.
                func_stack.back().depth += 1;
                if (func_stack.back().depth == 2 && expect_operand) {
                    // Expression started with '('
                    // (depth was 1 at function start)
                    // Mark that an argument exists.
                    func_stack.back().saw_expr = true;
                }
            }
            expect_operand = true;
            continue;
        }

        if (t.kind == TokKind::RParen) {
            bool closes_function = false;
            if (!func_stack.empty()) {
                func_stack.back().depth -= 1;
                if (func_stack.back().depth < 0) throw ParseError("Mismatched ')'");
                closes_function = (func_stack.back().depth == 0);
            }

            bool found_lparen = false;
            while (!opstack.empty()) {
                Token top = std::move(opstack.back());
                opstack.pop_back();
                if (top.kind == TokKind::LParen) {
                    found_lparen = true;
                    break;
                }
                output.push_back(std::move(top));
            }
            if (!found_lparen) throw ParseError("Mismatched ')'");

            if (closes_function) {
                // After popping '(', there should be a function marker.
                if (opstack.empty() || !is_function(opstack.back().kind)) {
                    throw ParseError("Internal error: function context without function token");
                }
                Token fn = std::move(opstack.back());
                opstack.pop_back();

                FuncCtx ctx = func_stack.back();
                func_stack.pop_back();

                if (!ctx.saw_expr) throw ParseError("Function call has empty argument list");

                fn.arity = ctx.commas + 1;
                output.push_back(std::move(fn));
            }
            expect_operand = false;
            continue;
        }

        if (t.kind == TokKind::Minus && expect_operand) {
            t.kind = TokKind::Neg; // unary
        }

        if (is_operator(t.kind)) {
            while (!opstack.empty() && is_operator(opstack.back().kind)) {
                TokKind topk = opstack.back().kind;
                TokKind curk = t.kind;

                int ptop = precedence(topk);
                int pcur = precedence(curk);

                bool pop_it = false;
                if (is_right_associative(curk)) pop_it = ptop > pcur;
                else pop_it = ptop >= pcur;

                if (!pop_it) break;

                output.push_back(std::move(opstack.back()));
                opstack.pop_back();
            }
            opstack.push_back(std::move(t));
            expect_operand = true;
            continue;
        }

        if (t.kind == TokKind::Assign) {
            throw ParseError("Unexpected '=' inside expression");
        }

        throw ParseError("Unexpected token in expression");
    }

    while (!opstack.empty()) {
        if (opstack.back().kind == TokKind::LParen) throw ParseError("Mismatched '('");
        if (opstack.back().kind == TokKind::Func) throw ParseError("Mismatched function call");
        output.push_back(std::move(opstack.back()));
        opstack.pop_back();
    }

    return Compiled{lhs.text, std::move(output)};
}

// -----------------------------
// evaluation
// -----------------------------
static Value negate_value(const Value& v) {
    if (std::holds_alternative<double>(v)) return -std::get<double>(v);
    return -std::get<TimeSeries>(v);
}

static Value apply_binary(TokKind op, const Value& a, const Value& b) {
    const bool a_ts = std::holds_alternative<TimeSeries>(a);
    const bool b_ts = std::holds_alternative<TimeSeries>(b);

    if (!a_ts && !b_ts) {
        double x = std::get<double>(a);
        double y = std::get<double>(b);
        switch (op) {
            case TokKind::Plus:  return x + y;
            case TokKind::Minus: return x - y;
            case TokKind::Star:  return x * y;
            case TokKind::Slash: return x / y;
            default: break;
        }
        throw EvalError("Unsupported scalar op");
    }

    if (a_ts && b_ts) {
        const auto& x = std::get<TimeSeries>(a);
        const auto& y = std::get<TimeSeries>(b);
        switch (op) {
            case TokKind::Plus:  return x + y;
            case TokKind::Minus: return x - y;
            case TokKind::Star:  return x * y;
            case TokKind::Slash: return x / y;
            default: break;
        }
        throw EvalError("Unsupported TS op");
    }

    if (a_ts && !b_ts) {
        const auto& x = std::get<TimeSeries>(a);
        double y = std::get<double>(b);
        switch (op) {
            case TokKind::Plus:  return x + y;
            case TokKind::Minus: return x - y;
            case TokKind::Star:  return x * y;
            case TokKind::Slash: return x / y;
            default: break;
        }
        throw EvalError("Unsupported TS-scalar op");
    } else {
        double x = std::get<double>(a);
        const auto& y = std::get<TimeSeries>(b);
        switch (op) {
            case TokKind::Plus:  return x + y;
            case TokKind::Minus: return x - y;
            case TokKind::Star:  return x * y;
            case TokKind::Slash: return x / y;
            default: break;
        }
        throw EvalError("Unsupported scalar-TS op");
    }
}

static Value apply_function(const Token& fn, const std::vector<Value>& args) {
    // For now we only ship Excel-like SUMPRODUCT.
    if (fn.text == "sumproduct") {
        if (args.size() != 2) throw EvalError("sumproduct expects 2 arguments");
        const Value& a = args[0];
        const Value& b = args[1];

        const bool a_ts = std::holds_alternative<TimeSeries>(a);
        const bool b_ts = std::holds_alternative<TimeSeries>(b);

        if (a_ts && b_ts) {
            return sumproduct(std::get<TimeSeries>(a), std::get<TimeSeries>(b));
        }
        if (a_ts && !b_ts) {
            return sumproduct(std::get<TimeSeries>(a), std::get<double>(b));
        }
        if (!a_ts && b_ts) {
            return sumproduct(std::get<double>(a), std::get<TimeSeries>(b));
        }
        return sumproduct(std::get<double>(a), std::get<double>(b));
    }

    throw EvalError("Unknown function: " + fn.text);
}

Value eval_rpn(const std::vector<Token>& rpn, const Env& env) {
    std::vector<Value> st;
    st.reserve(rpn.size());

    auto pop = [&]() -> Value {
        if (st.empty()) throw EvalError("Stack underflow (bad expression)");
        Value v = std::move(st.back());
        st.pop_back();
        return v;
    };

    for (const auto& t : rpn) {
        switch (t.kind) {
            case TokKind::Number:
                st.emplace_back(t.number);
                break;

            case TokKind::Ident: {
                auto it = env.find(t.text);
                if (it == env.end()) throw EvalError("Unknown variable: " + t.text);
                st.emplace_back(it->second);
                break;
            }

            case TokKind::Neg: {
                Value v = pop();
                st.emplace_back(negate_value(v));
                break;
            }

            case TokKind::Plus:
            case TokKind::Minus:
            case TokKind::Star:
            case TokKind::Slash: {
                Value b = pop();
                Value a = pop();
                st.emplace_back(apply_binary(t.kind, a, b));
                break;
            }

            case TokKind::Func: {
                if (t.arity <= 0) throw EvalError("Bad function arity");
                if (static_cast<std::size_t>(t.arity) > st.size()) throw EvalError("Stack underflow (function args)");

                std::vector<Value> args;
                args.resize(static_cast<std::size_t>(t.arity));
                // Pop in reverse; args[0] is first argument as written.
                for (int i = t.arity - 1; i >= 0; --i) {
                    args[static_cast<std::size_t>(i)] = pop();
                }
                st.emplace_back(apply_function(t, args));
                break;
            }

            default:
                throw EvalError("Unexpected token during evaluation");
        }
    }

    if (st.size() != 1) throw EvalError("Expression did not reduce to a single value");
    return st.back();
}

void execute_assignment(std::string_view input, Env& env) {
    Compiled c = compile(input);
    Value v = eval_rpn(c.rpn, env);

    // For this starter repo, Env holds TimeSeries only. If a scalar is produced
    // (e.g., by sumproduct), we store it as a length-1 series.
    if (std::holds_alternative<double>(v)) {
        env[c.target] = TimeSeries::from_scalar(std::get<double>(v));
        return;
    }
    env[c.target] = std::get<TimeSeries>(v);
}

} // namespace ts::expr
