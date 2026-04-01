/// @file linq_statistics_by_key_test.cpp
/// @brief variance_*_by, std_dev_*_by, percentile clamping, duplicate values in stats.

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(StatsByKey, VariancePopulationByMatchesValues)
{
    struct row {
        int id;
        double v;
    };
    std::vector<row> const data{{0, 0.0}, {0, 10.0}};
    double const vp = qb::linq::from(data).variance_population_by([](row const& r) { return r.v; });
    EXPECT_NEAR(vp, 25.0, 1e-9);
}

TEST(StatsByKey, StdDevSampleByMatchesSqrtVariance)
{
    struct row {
        double w;
    };
    std::vector<row> const data{{1.0}, {2.0}, {3.0}};
    double const vs = qb::linq::from(data).variance_sample_by([](row const& r) { return r.w; });
    double const sd = qb::linq::from(data).std_dev_sample_by([](row const& r) { return r.w; });
    EXPECT_NEAR(sd, std::sqrt(vs), 1e-9);
}

TEST(StatsByKey, PercentileClampsOutOfRangeP)
{
    std::vector<int> const data{1, 2, 3};
    EXPECT_DOUBLE_EQ(qb::linq::from(data).percentile(-50), qb::linq::from(data).percentile(0));
    EXPECT_DOUBLE_EQ(qb::linq::from(data).percentile(150), qb::linq::from(data).percentile(100));
}

TEST(StatsByKey, MedianByWithDuplicateMiddle)
{
    struct p {
        int tag;
        double x;
    };
    std::vector<p> const data{{0, 1.0}, {1, 2.0}, {2, 2.0}, {3, 5.0}};
    double const m = qb::linq::from(data).median_by([](p const& r) { return r.x; });
    EXPECT_NEAR(m, 2.0, 1e-9);
}
