#include <tsexpr/parser.hpp>

#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

namespace toy {

// Toy "time series": vector of doubles.
struct Series { std::vector<double> v; };

// Values are either series or scalar.
using Value = std::variant<Series, double>;

static Series elementwise(const Series& a, const Series& b, double (*op)(double,double)) {
    if (a.v.size() != b.v.size()) throw std::runtime_error("Series length mismatch");
    Series out; out.v.resize(a.v.size());
    for (std::size_t i = 0; i < a.v.size(); ++i) out.v[i] = op(a.v[i], b.v[i]);
    return out;
}

static Series elementwise_scalar(const Series& a, double s, double (*op)(double,double)) {
    Series out; out.v.resize(a.v.size());
    for (std::size_t i = 0; i < a.v.size(); ++i) out.v[i] = op(a.v[i], s);
    return out;
}

static double sumproduct_series(const Series& a, const Series& b) {
    if (a.v.size() != b.v.size()) throw std::runtime_error("Series length mismatch");
    double acc = 0.0;
    for (std::size_t i = 0; i < a.v.size(); ++i) acc += a.v[i] * b.v[i];
    return acc;
}

// Backend required by Program::execute
struct Backend {
    std::map<std::string, Value> vars;

    Value load_var(std::string_view name) const {
        auto it = vars.find(std::string(name));
        if (it == vars.end()) throw std::runtime_error("Unknown variable: " + std::string(name));
        return it->second;
    }

    void store_var(std::string_view name, const Value& v) {
        vars[std::string(name)] = v;
    }

    Value make_number(double x) const { return x; }

    Value neg(const Value& a) const {
        if (std::holds_alternative<double>(a)) return -std::get<double>(a);
        const auto& s = std::get<Series>(a);
        Series out; out.v.resize(s.v.size());
        for (std::size_t i = 0; i < s.v.size(); ++i) out.v[i] = -s.v[i];
        return out;
    }

    Value binary(tsexpr::Op op, const Value& a, const Value& b) const {
        auto add = [](double x,double y){ return x+y; };
        auto sub = [](double x,double y){ return x-y; };
        auto mul = [](double x,double y){ return x*y; };
        auto div = [](double x,double y){ return x/y; };

        auto pick = [&](tsexpr::Op o)->double(*)(double,double){
            switch (o) {
                case tsexpr::Op::Add: return add;
                case tsexpr::Op::Sub: return sub;
                case tsexpr::Op::Mul: return mul;
                case tsexpr::Op::Div: return div;
                default: throw std::runtime_error("Unsupported binary op");
            }
        };
        auto f = pick(op);

        const bool aS = std::holds_alternative<Series>(a);
        const bool bS = std::holds_alternative<Series>(b);

        if (!aS && !bS) return f(std::get<double>(a), std::get<double>(b));
        if (aS && bS)   return elementwise(std::get<Series>(a), std::get<Series>(b), f);
        if (aS && !bS)  return elementwise_scalar(std::get<Series>(a), std::get<double>(b), f);

        // scalar op series
        double s = std::get<double>(a);
        const auto& x = std::get<Series>(b);
        Series out; out.v.resize(x.v.size());
        for (std::size_t i = 0; i < x.v.size(); ++i) out.v[i] = f(s, x.v[i]);
        return out;
    }

    Value call(std::string_view fn, const std::vector<Value>& args) const {
        if (fn == "sumproduct") {
            if (args.size() != 2) throw std::runtime_error("sumproduct expects 2 args");
            const auto& a = args[0];
            const auto& b = args[1];

            const bool aS = std::holds_alternative<Series>(a);
            const bool bS = std::holds_alternative<Series>(b);

            if (aS && bS) return sumproduct_series(std::get<Series>(a), std::get<Series>(b));

            if (aS && !bS) {
                const auto& s = std::get<Series>(a);
                double k = std::get<double>(b);
                return k * std::accumulate(s.v.begin(), s.v.end(), 0.0);
            }

            if (!aS && bS) {
                double k = std::get<double>(a);
                const auto& s = std::get<Series>(b);
                return k * std::accumulate(s.v.begin(), s.v.end(), 0.0);
            }

            return std::get<double>(a) * std::get<double>(b);
        }

        throw std::runtime_error("Unknown function: " + std::string(fn));
    }
};

} // namespace toy

static void print_value(const toy::Value& v) {
    if (std::holds_alternative<double>(v)) {
        std::cout << std::get<double>(v) << "\n";
        return;
    }
    const auto& s = std::get<toy::Series>(v).v;
    std::cout << "[";
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (i) std::cout << ", ";
        std::cout << s[i];
    }
    std::cout << "]\n";
}

int main() {
    toy::Backend backend;

    backend.vars["carry"] = toy::Series{{2,2,2}};
    backend.vars["total return"] = toy::Series{{5,6,7}};
    backend.vars["a"] = toy::Series{{1,2,3}};
    backend.vars["b"] = toy::Series{{10,20,30}};
    backend.vars["x"] = 10.0;

    // 1) Toy time series example + scalar literal
    auto p1 = tsexpr::compile("z = `total return` + carry / 2");
    p1.execute(backend);
    std::cout << "z = ";
    print_value(backend.vars["z"]); // [6, 7, 8]

    // 2) sumproduct reduces to scalar
    auto p2 = tsexpr::compile("s = sumproduct(a, b)");
    p2.execute(backend);
    std::cout << "s = ";
    print_value(backend.vars["s"]); // 140

    // 3) pure scalar expression
    auto p3 = tsexpr::compile("y = x * 3 - 4");
    p3.execute(backend);
    std::cout << "y = ";
    print_value(backend.vars["y"]); // 26

    return 0;
}
