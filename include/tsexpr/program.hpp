#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace tsexpr {

struct EvalError : std::runtime_error { using std::runtime_error::runtime_error; };

enum class Op {
    PushVar,
    PushNum,
    Add,
    Sub,
    Mul,
    Div,
    Neg,
    Call,   // fn name + argc
    Store,  // var name
};

struct Instr {
    Op op{Op::PushNum};
    std::string text{}; // var name / function name
    double number{0.0}; // literal
    int argc{0};        // Call arg count
};

struct Program {
    std::vector<Instr> code;

    template <class Backend>
    void execute(Backend& backend) const {
        using Value = decltype(backend.load_var(std::string_view{}));

        std::vector<Value> st;
        st.reserve(code.size());

        auto pop = [&]() -> Value {
            if (st.empty()) throw EvalError("Stack underflow (bad program)");
            Value v = std::move(st.back());
            st.pop_back();
            return v;
        };

        for (const auto& ins : code) {
            switch (ins.op) {
                case Op::PushVar:
                    st.emplace_back(backend.load_var(ins.text));
                    break;

                case Op::PushNum:
                    st.emplace_back(backend.make_number(ins.number));
                    break;

                case Op::Neg: {
                    Value a = pop();
                    st.emplace_back(backend.neg(a));
                } break;

                case Op::Add:
                case Op::Sub:
                case Op::Mul:
                case Op::Div: {
                    Value b = pop();
                    Value a = pop();
                    st.emplace_back(backend.binary(ins.op, a, b));
                } break;

                case Op::Call: {
                    if (ins.argc < 0) throw EvalError("Invalid CALL argc");
                    if (static_cast<std::size_t>(ins.argc) > st.size())
                        throw EvalError("Not enough args for CALL");
                    std::vector<Value> args(static_cast<std::size_t>(ins.argc));
                    for (int i = ins.argc - 1; i >= 0; --i)
                        args[static_cast<std::size_t>(i)] = pop();
                    st.emplace_back(backend.call(ins.text, args));
                } break;

                case Op::Store: {
                    Value v = pop();
                    backend.store_var(ins.text, v);
                } break;
            }
        }
    }
};

} // namespace tsexpr
