#include <gtest/gtest.h>
#include <tsexpr/parser.hpp>

#include <map>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

namespace {

struct Series { std::vector<double> v; };
using Value = std::variant<Series, double>;

static Series ew(const Series& a, const Series& b, double (*op)(double,double)) {
    if (a.v.size() != b.v.size()) throw std::runtime_error("len mismatch");
    Series o; o.v.resize(a.v.size());
    for (std::size_t i=0;i<a.v.size();++i) o.v[i]=op(a.v[i], b.v[i]);
    return o;
}
static Series ew_scalar(const Series& a, double s, double (*op)(double,double)) {
    Series o; o.v.resize(a.v.size());
    for (std::size_t i=0;i<a.v.size();++i) o.v[i]=op(a.v[i], s);
    return o;
}
static double sumproduct_series(const Series& a, const Series& b) {
    if (a.v.size()!=b.v.size()) throw std::runtime_error("len mismatch");
    double acc=0;
    for (std::size_t i=0;i<a.v.size();++i) acc += a.v[i]*b.v[i];
    return acc;
}

struct Backend {
    std::map<std::string, Value> vars;

    Value load_var(std::string_view name) const {
        auto it = vars.find(std::string(name));
        if (it == vars.end()) throw std::runtime_error("unknown var: " + std::string(name));
        return it->second;
    }
    void store_var(std::string_view name, const Value& v) { vars[std::string(name)] = v; }
    Value make_number(double x) const { return x; }

    Value neg(const Value& a) const {
        if (std::holds_alternative<double>(a)) return -std::get<double>(a);
        const auto& s = std::get<Series>(a);
        Series o; o.v.resize(s.v.size());
        for (std::size_t i=0;i<s.v.size();++i) o.v[i] = -s.v[i];
        return o;
    }

    Value binary(tsexpr::Op op, const Value& a, const Value& b) const {
        auto add=[](double x,double y){return x+y;};
        auto sub=[](double x,double y){return x-y;};
        auto mul=[](double x,double y){return x*y;};
        auto div=[](double x,double y){return x/y;};

        auto pick=[&](tsexpr::Op o)->double(*)(double,double){
            switch(o){
                case tsexpr::Op::Add: return add;
                case tsexpr::Op::Sub: return sub;
                case tsexpr::Op::Mul: return mul;
                case tsexpr::Op::Div: return div;
                default: throw std::runtime_error("bad op");
            }
        };
        auto f=pick(op);

        bool aS=std::holds_alternative<Series>(a);
        bool bS=std::holds_alternative<Series>(b);
        if(!aS && !bS) return f(std::get<double>(a), std::get<double>(b));
        if(aS && bS)   return ew(std::get<Series>(a), std::get<Series>(b), f);
        if(aS && !bS)  return ew_scalar(std::get<Series>(a), std::get<double>(b), f);

        // scalar op series
        double s = std::get<double>(a);
        const auto& x = std::get<Series>(b);
        Series o; o.v.resize(x.v.size());
        for (std::size_t i=0;i<x.v.size();++i) o.v[i] = f(s, x.v[i]);
        return o;
    }

    Value call(std::string_view fn, const std::vector<Value>& args) const {
        if (fn == "sumproduct") {
            if (args.size()!=2) throw std::runtime_error("argc");
            const auto& a=args[0]; const auto& b=args[1];
            bool aS=std::holds_alternative<Series>(a);
            bool bS=std::holds_alternative<Series>(b);
            if(aS && bS) return sumproduct_series(std::get<Series>(a), std::get<Series>(b));
            if(aS && !bS){
                const auto& s=std::get<Series>(a); double k=std::get<double>(b);
                return k*std::accumulate(s.v.begin(), s.v.end(), 0.0);
            }
            if(!aS && bS){
                double k=std::get<double>(a); const auto& s=std::get<Series>(b);
                return k*std::accumulate(s.v.begin(), s.v.end(), 0.0);
            }
            return std::get<double>(a)*std::get<double>(b);
        }
        throw std::runtime_error("unknown fn");
    }
};

TEST(Expr, BackticksMixedWithUnquoted) {
    Backend be;
    be.vars["total return"] = Series{{5,6,7}};
    be.vars["carry"] = Series{{2,2,2}};

    auto p = tsexpr::compile("z = `total return` + carry / 2");
    p.execute(be);

    ASSERT_TRUE(std::holds_alternative<Series>(be.vars["z"]));
    auto z = std::get<Series>(be.vars["z"]).v;
    ASSERT_EQ(z.size(), 3u);
    EXPECT_DOUBLE_EQ(z[0], 6.0);
    EXPECT_DOUBLE_EQ(z[1], 7.0);
    EXPECT_DOUBLE_EQ(z[2], 8.0);
}

TEST(Expr, SumproductReducesToScalar) {
    Backend be;
    be.vars["a"] = Series{{1,2,3}};
    be.vars["b"] = Series{{10,20,30}};

    auto p = tsexpr::compile("s = sumproduct(a, b)");
    p.execute(be);

    ASSERT_TRUE(std::holds_alternative<double>(be.vars["s"]));
    EXPECT_DOUBLE_EQ(std::get<double>(be.vars["s"]), 140.0);
}

TEST(Expr, ScalarsOnly) {
    Backend be;
    be.vars["x"] = 10.0;

    auto p = tsexpr::compile("y = x * 3 - 4");
    p.execute(be);

    ASSERT_TRUE(std::holds_alternative<double>(be.vars["y"]));
    EXPECT_DOUBLE_EQ(std::get<double>(be.vars["y"]), 26.0);
}

} // namespace
