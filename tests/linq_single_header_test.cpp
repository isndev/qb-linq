/**
 * @file linq_single_header_test.cpp
 * @brief Smoke test: `single_header/linq.h` is self-contained (no CMake-generated version.h path).
 */

#include <gtest/gtest.h>

#include <vector>

#include <linq.h>

TEST(SingleHeader, VersionMatchesEmbeddedMacros)
{
    EXPECT_EQ(qb::linq::version::major, QB_LINQ_VERSION_MAJOR);
    EXPECT_EQ(qb::linq::version::minor, QB_LINQ_VERSION_MINOR);
    EXPECT_EQ(qb::linq::version::patch, QB_LINQ_VERSION_PATCH);
    EXPECT_STRNE(QB_LINQ_VERSION_STRING, "");
}

TEST(SingleHeader, Pipeline)
{
    std::vector<int> v{1, 2, 3, 4};
    int s = qb::linq::from(v).where([](int x) { return x % 2 == 0; }).sum();
    EXPECT_EQ(s, 6);
}
