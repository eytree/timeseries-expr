#pragma once

#include <vector>
#include <stdexcept>

namespace ts::expr {

/// A tiny stub TimeSeries used for tests and examples.
/// Replace with your real time series type + alignment semantics.
struct TimeSeries {
    std::vector<double> v;

    TimeSeries() = default;
    explicit TimeSeries(std::vector<double> values) : v(std::move(values)) {}

    /// Convenience for tests: represent a scalar as a length-1 series.
    /// Real implementations might store scalars separately; adapt as needed.
    static TimeSeries from_scalar(double x) { return TimeSeries{{x}}; }

    std::size_t size() const noexcept { return v.size(); }

    static void require_same_size(const TimeSeries& a, const TimeSeries& b) {
        if (a.v.size() != b.v.size()) {
            throw std::runtime_error("TimeSeries size mismatch (stub alignment rule)");
        }
    }
};

/// Excel-like SUMPRODUCT: multiply elementwise then sum.
/// Stub rule: requires same size.
double sumproduct(const TimeSeries& a, const TimeSeries& b);
double sumproduct(const TimeSeries& a, double b);
double sumproduct(double a, const TimeSeries& b);
double sumproduct(double a, double b);

// TS op TS: elementwise; requires same size (stub semantics)
TimeSeries operator+(const TimeSeries& a, const TimeSeries& b);
TimeSeries operator-(const TimeSeries& a, const TimeSeries& b);
TimeSeries operator*(const TimeSeries& a, const TimeSeries& b);
TimeSeries operator/(const TimeSeries& a, const TimeSeries& b);

// TS op scalar
TimeSeries operator+(const TimeSeries& a, double b);
TimeSeries operator-(const TimeSeries& a, double b);
TimeSeries operator*(const TimeSeries& a, double b);
TimeSeries operator/(const TimeSeries& a, double b);

// scalar op TS
TimeSeries operator+(double a, const TimeSeries& b);
TimeSeries operator-(double a, const TimeSeries& b);
TimeSeries operator*(double a, const TimeSeries& b);
TimeSeries operator/(double a, const TimeSeries& b);

// unary
TimeSeries operator-(const TimeSeries& a);

} // namespace ts::expr
