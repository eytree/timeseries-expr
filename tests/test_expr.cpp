#include <gtest/gtest.h>

#include <tsexpr/expr.hpp>

using namespace ts::expr;

static void expect_vec_eq(const std::vector<double>& a, const std::vector<double>& b) {
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t i=0;i<a.size();++i) {
        EXPECT_DOUBLE_EQ(a[i], b[i]) << "at i=" << i;
    }
}

TEST(Expr, BasicAssignment) {
    Env env;
    env["a"] = TimeSeries{{1,2,3}};
    env["b"] = TimeSeries{{10,20,30}};
    env["c"] = TimeSeries{{2,4,6}};

    execute_assignment("z = a + b - c / 2", env);

    auto it = env.find("z");
    ASSERT_NE(it, env.end());
    // z = a + b - c/2 = [1+10-1, 2+20-2, 3+30-3] = [10,20,30]
    expect_vec_eq(it->second.v, std::vector<double>({10,20,30}));
}

TEST(Expr, ParenthesesAndUnaryMinus) {
    Env env;
    env["a"] = TimeSeries{{1,2,3}};
    env["b"] = TimeSeries{{10,20,30}};

    execute_assignment("z = -(a + b) * 2", env);

    auto it = env.find("z");
    ASSERT_NE(it, env.end());
    // -(a+b)*2 = -([11,22,33])*2 = [-22,-44,-66]
    expect_vec_eq(it->second.v, std::vector<double>({-22,-44,-66}));
}

TEST(Expr, UnaryMinusTightBinding) {
    Env env;
    env["a"] = TimeSeries{{1,2,3}};
    env["b"] = TimeSeries{{10,20,30}};

    execute_assignment("z = a * -b", env);

    auto it = env.find("z");
    ASSERT_NE(it, env.end());
    // a * -b = [1*-10, 2*-20, 3*-30]
    expect_vec_eq(it->second.v, std::vector<double>({-10,-40,-90}));
}

TEST(Expr, UnknownVariableErrors) {
    Env env;
    env["a"] = TimeSeries{{1,2,3}};

    EXPECT_THROW(execute_assignment("z = a + missing", env), EvalError);
}

TEST(Expr, BacktickIdentifiersWithSpaces) {
    Env env;
    env["total return"] = TimeSeries{{1,2,3}};
    env["carry"] = TimeSeries{{10,20,30}};

    execute_assignment("z = `total return` + carry / 2", env);

    auto it = env.find("z");
    ASSERT_NE(it, env.end());
    // [1+5, 2+10, 3+15] = [6,12,18]
    expect_vec_eq(it->second.v, std::vector<double>({6,12,18}));
}

TEST(Expr, SumproductExcelSemantics) {
    Env env;
    env["a"] = TimeSeries{{1,2,3}};
    env["b"] = TimeSeries{{10,20,30}};

    execute_assignment("z = sumproduct(a, b)", env);

    auto it = env.find("z");
    ASSERT_NE(it, env.end());
    // sumproduct = 1*10 + 2*20 + 3*30 = 140
    expect_vec_eq(it->second.v, std::vector<double>({140}));
}

TEST(Expr, MismatchedParenErrors) {
    Env env;
    env["a"] = TimeSeries{{1,2,3}};

    EXPECT_THROW(execute_assignment("z = (a + 2", env), ParseError);
}
