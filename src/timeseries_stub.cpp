#include <tsexpr/timeseries_stub.hpp>

#include <cmath>

namespace ts::expr {

double sumproduct(const TimeSeries& a, const TimeSeries& b) {
    TimeSeries::require_same_size(a, b);
    double acc = 0.0;
    for (std::size_t i = 0; i < a.v.size(); ++i) {
        acc += a.v[i] * b.v[i];
    }
    return acc;
}

double sumproduct(const TimeSeries& a, double b) {
    double acc = 0.0;
    for (double x : a.v) acc += x * b;
    return acc;
}

double sumproduct(double a, const TimeSeries& b) {
    double acc = 0.0;
    for (double x : b.v) acc += a * x;
    return acc;
}

double sumproduct(double a, double b) {
    return a * b;
}

static TimeSeries binop_ts_ts(const TimeSeries& a, const TimeSeries& b, double (*op)(double,double)) {
    TimeSeries::require_same_size(a,b);
    TimeSeries out;
    out.v.resize(a.v.size());
    for (std::size_t i=0;i<a.v.size();++i) out.v[i] = op(a.v[i], b.v[i]);
    return out;
}

static TimeSeries binop_ts_s(const TimeSeries& a, double b, double (*op)(double,double)) {
    TimeSeries out;
    out.v.resize(a.v.size());
    for (std::size_t i=0;i<a.v.size();++i) out.v[i] = op(a.v[i], b);
    return out;
}

static TimeSeries binop_s_ts(double a, const TimeSeries& b, double (*op)(double,double)) {
    TimeSeries out;
    out.v.resize(b.v.size());
    for (std::size_t i=0;i<b.v.size();++i) out.v[i] = op(a, b.v[i]);
    return out;
}

static double add(double x,double y){return x+y;}
static double sub(double x,double y){return x-y;}
static double mul(double x,double y){return x*y;}
static double divv(double x,double y){return x/y;}

TimeSeries operator+(const TimeSeries& a, const TimeSeries& b){ return binop_ts_ts(a,b,add); }
TimeSeries operator-(const TimeSeries& a, const TimeSeries& b){ return binop_ts_ts(a,b,sub); }
TimeSeries operator*(const TimeSeries& a, const TimeSeries& b){ return binop_ts_ts(a,b,mul); }
TimeSeries operator/(const TimeSeries& a, const TimeSeries& b){ return binop_ts_ts(a,b,divv); }

TimeSeries operator+(const TimeSeries& a, double b){ return binop_ts_s(a,b,add); }
TimeSeries operator-(const TimeSeries& a, double b){ return binop_ts_s(a,b,sub); }
TimeSeries operator*(const TimeSeries& a, double b){ return binop_ts_s(a,b,mul); }
TimeSeries operator/(const TimeSeries& a, double b){ return binop_ts_s(a,b,divv); }

TimeSeries operator+(double a, const TimeSeries& b){ return binop_s_ts(a,b,add); }
TimeSeries operator-(double a, const TimeSeries& b){ return binop_s_ts(a,b,sub); }
TimeSeries operator*(double a, const TimeSeries& b){ return binop_s_ts(a,b,mul); }
TimeSeries operator/(double a, const TimeSeries& b){ return binop_s_ts(a,b,divv); }

TimeSeries operator-(const TimeSeries& a){
    TimeSeries out;
    out.v.resize(a.v.size());
    for (std::size_t i=0;i<a.v.size();++i) out.v[i] = -a.v[i];
    return out;
}

} // namespace ts::expr
