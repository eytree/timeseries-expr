#include "tsexpr/parser.hpp"
#include "tsexpr/lexer.hpp"
#include "tsexpr/token.hpp"
#include <vector>

namespace tsexpr {

static int precedence(TokKind k) {
    switch (k) {
        case TokKind::Neg:   return 4;
        case TokKind::Star:
        case TokKind::Slash: return 3;
        case TokKind::Plus:
        case TokKind::Minus: return 2;
        default:             return 0;
    }
}

static bool is_right_assoc(TokKind k) { return k == TokKind::Neg; }

static bool is_op(TokKind k) {
    return k == TokKind::Plus || k == TokKind::Minus || k == TokKind::Star || k == TokKind::Slash || k == TokKind::Neg;
}

// Shunting-yard with function calls + commas.
// Output RPN tokens. Function calls emit Token{TokKind::Func, name, argc}.
static std::vector<Token> to_rpn(Lexer& lex, Token t0) {
    std::vector<Token> output;
    std::vector<Token> opstack;

    bool expect_operand = true;

    struct FnFrame { std::string name; int argc; bool saw_any_arg; };
    std::vector<FnFrame> fnstack;

    Token t = t0;

    auto next_token = [&]() { return lex.next(); };

    while (t.kind != TokKind::End) {
        if (t.kind == TokKind::Ident) {
            // lookahead for function call
            Token peek = next_token();
            if (peek.kind == TokKind::LParen) {
                opstack.push_back(Token{TokKind::Func, t.text, 0.0});
                opstack.push_back(peek); // '('
                fnstack.push_back(FnFrame{t.text, 0, false});
                expect_operand = true;
                t = next_token();
                continue;
            } else {
                output.push_back(t);
                expect_operand = false;
                if (!fnstack.empty()) fnstack.back().saw_any_arg = true;
                t = peek;
                continue;
            }
        }

        if (t.kind == TokKind::Number) {
            output.push_back(t);
            expect_operand = false;
            if (!fnstack.empty()) fnstack.back().saw_any_arg = true;
            t = next_token();
            continue;
        }

        if (t.kind == TokKind::LParen) {
            opstack.push_back(t);
            expect_operand = true;
            t = next_token();
            continue;
        }

        if (t.kind == TokKind::Comma) {
            while (!opstack.empty() && opstack.back().kind != TokKind::LParen) {
                output.push_back(opstack.back());
                opstack.pop_back();
            }
            if (opstack.empty()) throw ParseError("Comma not within function call");
            if (fnstack.empty()) throw ParseError("Internal error: comma with no function frame");
            fnstack.back().argc += 1;
            expect_operand = true;
            t = next_token();
            continue;
        }

        if (t.kind == TokKind::RParen) {
            while (!opstack.empty() && opstack.back().kind != TokKind::LParen) {
                output.push_back(opstack.back());
                opstack.pop_back();
            }
            if (opstack.empty()) throw ParseError("Mismatched ')'");
            opstack.pop_back(); // pop '('

            if (!opstack.empty() && opstack.back().kind == TokKind::Func) {
                Token fn = opstack.back();
                opstack.pop_back();

                if (fnstack.empty()) throw ParseError("Internal error: function close with no frame");
                auto frame = fnstack.back();
                fnstack.pop_back();

                int argc = frame.saw_any_arg ? (frame.argc + 1) : 0;
                fn.number = static_cast<double>(argc);
                output.push_back(fn);

                if (!fnstack.empty()) fnstack.back().saw_any_arg = true;
            }

            expect_operand = false;
            t = next_token();
            continue;
        }

        if (t.kind == TokKind::Minus && expect_operand) t.kind = TokKind::Neg;

        if (is_op(t.kind)) {
            while (!opstack.empty() && is_op(opstack.back().kind)) {
                TokKind topk = opstack.back().kind;
                TokKind curk = t.kind;
                int ptop = precedence(topk);
                int pcur = precedence(curk);

                bool pop_it = is_right_assoc(curk) ? (ptop > pcur) : (ptop >= pcur);
                if (!pop_it) break;

                output.push_back(opstack.back());
                opstack.pop_back();
            }
            opstack.push_back(t);
            expect_operand = true;
            t = next_token();
            continue;
        }

        throw ParseError("Unexpected token in expression");
    }

    while (!opstack.empty()) {
        if (opstack.back().kind == TokKind::LParen) throw ParseError("Mismatched '('");
        if (opstack.back().kind == TokKind::Func) throw ParseError("Mismatched function call");
        output.push_back(opstack.back());
        opstack.pop_back();
    }
    if (!fnstack.empty()) throw ParseError("Mismatched function call");
    return output;
}

Program compile(std::string_view input) {
    Lexer lex(input);

    Token lhs = lex.next();
    if (lhs.kind != TokKind::Ident) throw ParseError("Expected assignment target identifier at start");
    Token eq = lex.next();
    if (eq.kind != TokKind::Assign) throw ParseError("Expected '=' after assignment target");

    Token first = lex.next();
    if (first.kind == TokKind::End) throw ParseError("Expected expression after '='");

    std::vector<Token> rpn = to_rpn(lex, first);

    Program p;
    p.code.reserve(rpn.size() + 1);

    for (const auto& t : rpn) {
        switch (t.kind) {
            case TokKind::Number:
                p.code.push_back(Instr{Op::PushNum, "", t.number, 0});
                break;
            case TokKind::Ident:
                p.code.push_back(Instr{Op::PushVar, t.text, 0.0, 0});
                break;
            case TokKind::Neg:
                p.code.push_back(Instr{Op::Neg});
                break;
            case TokKind::Plus:
                p.code.push_back(Instr{Op::Add});
                break;
            case TokKind::Minus:
                p.code.push_back(Instr{Op::Sub});
                break;
            case TokKind::Star:
                p.code.push_back(Instr{Op::Mul});
                break;
            case TokKind::Slash:
                p.code.push_back(Instr{Op::Div});
                break;
            case TokKind::Func: {
                Instr ins;
                ins.op = Op::Call;
                ins.text = t.text;
                ins.argc = static_cast<int>(t.number);
                p.code.push_back(std::move(ins));
            } break;
            default:
                throw ParseError("Unsupported token in RPN compilation");
        }
    }

    p.code.push_back(Instr{Op::Store, lhs.text});
    return p;
}

} // namespace tsexpr
