/// @file linq_statistics_test.cpp
/// @brief Percentiles, median, variance, std-dev: contracts vs naive reference values.

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

namespace {

double naive_variance_pop(std::vector<double> const& v)
{
    double const sum = std::accumulate(v.begin(), v.end(), 0.0);
    double const mean = sum / static_cast<double>(v.size());
    double acc = 0;
    for (double x : v) {
        double const d = x - mean;
        acc += d * d;
    }
    return acc / static_cast<double>(v.size());
}

double naive_variance_sample(std::vector<double> const& v)
{
    if (v.size() < 2)
        throw std::out_of_range("naive");
    double const sum = std::accumulate(v.begin(), v.end(), 0.0);
    double const mean = sum / static_cast<double>(v.size());
    double acc = 0;
    for (double x : v) {
        double const d = x - mean;
        acc += d * d;
    }
    return acc / static_cast<double>(v.size() - 1);
}

} // namespace

TEST(StatisticsPercentile, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).percentile(50), std::out_of_range);
}

TEST(StatisticsPercentile, EndpointsAndInterior)
{
    std::vector<int> const data{10, 20, 30, 40};
    EXPECT_DOUBLE_EQ(qb::linq::from(data).percentile(0), 10.0);
    EXPECT_DOUBLE_EQ(qb::linq::from(data).percentile(100), 40.0);
    double const mid = qb::linq::from(data).percentile(50);
    EXPECT_NEAR(mid, 25.0, 1e-9);
}

TEST(StatisticsMedian, MatchesPercentile50)
{
    std::vector<int> const data{3, 1, 4, 1, 5};
    EXPECT_DOUBLE_EQ(qb::linq::from(data).median(), qb::linq::from(data).percentile(50));
}

TEST(StatisticsMedianBy, ProjectsBeforeOrderStats)
{
    struct row {
        int id;
        double w;
    };
    std::vector<row> const data{{0, 1.0}, {1, 4.0}, {2, 9.0}};
    EXPECT_DOUBLE_EQ(qb::linq::from(data).median_by([](row const& r) { return r.w; }), 4.0);
}

TEST(StatisticsVariancePopulation, MatchesNaive)
{
    std::vector<double> const v{2.0, 4.0, 6.0, 8.0};
    double const ref = naive_variance_pop(v);
    EXPECT_NEAR(qb::linq::from(v).variance_population(), ref, 1e-9);
}

TEST(StatisticsVariancePopulation, EmptyThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).variance_population(), std::out_of_range);
}

TEST(StatisticsVariancePopulation, SingleElementIsZero)
{
    std::vector<double> const one{42.0};
    EXPECT_DOUBLE_EQ(qb::linq::from(one).variance_population(), 0.0);
}

TEST(StatisticsVarianceSample, MatchesNaive)
{
    std::vector<double> const v{1.0, 2.0, 3.0, 4.0};
    double const ref = naive_variance_sample(v);
    EXPECT_NEAR(qb::linq::from(v).variance_sample(), ref, 1e-9);
}

TEST(StatisticsVarianceSample, SingleElementThrows)
{
    std::vector<int> const one{42};
    EXPECT_THROW((void)qb::linq::from(one).variance_sample(), std::out_of_range);
}

TEST(StatisticsStdDev, SqrtOfVariance)
{
    std::vector<int> const data{1, 2, 3};
    double const vp = qb::linq::from(data).variance_population();
    EXPECT_NEAR(qb::linq::from(data).std_dev_population(), std::sqrt(vp), 1e-9);
    double const vs = qb::linq::from(data).variance_sample();
    EXPECT_NEAR(qb::linq::from(data).std_dev_sample(), std::sqrt(vs), 1e-9);
}

TEST(StatisticsAverageIf, EmptyPredicateThrows)
{
    std::vector<int> const data{1, 3, 5};
    EXPECT_THROW((void)qb::linq::from(data).average_if([](int x) { return x % 2 == 0; }), std::out_of_range);
}

TEST(StatisticsAverageIf, EmptySequenceThrows)
{
    std::vector<int> const empty;
    EXPECT_THROW((void)qb::linq::from(empty).average_if([](int) { return true; }), std::out_of_range);
}

TEST(StatisticsPercentileBy, KeyProjection)
{
    struct p {
        int tag;
        double v;
    };
    std::vector<p> const data{{0, 100.0}, {1, 0.0}, {2, 50.0}};
    double const q = qb::linq::from(data).percentile_by(50.0, [](p const& x) { return x.v; });
    EXPECT_NEAR(q, 50.0, 1e-9);
}
