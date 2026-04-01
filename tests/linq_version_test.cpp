/**
 * @file linq_version_test.cpp
 * @brief Generated `qb/linq/version.h` matches CMake project version (via configure_file).
 */

#include <gtest/gtest.h>

#include <qb/linq.h>

TEST(Version, MacrosMatchStruct)
{
    EXPECT_EQ(qb::linq::version::major, QB_LINQ_VERSION_MAJOR);
    EXPECT_EQ(qb::linq::version::minor, QB_LINQ_VERSION_MINOR);
    EXPECT_EQ(qb::linq::version::patch, QB_LINQ_VERSION_PATCH);
    EXPECT_EQ(qb::linq::version::integer, QB_LINQ_VERSION_INTEGER);
}

TEST(Version, StringNonEmpty)
{
    EXPECT_STRNE(QB_LINQ_VERSION_STRING, "");
}

TEST(Version, IntegerFormula)
{
    EXPECT_EQ(QB_LINQ_VERSION_INTEGER,
        QB_LINQ_VERSION_MAJOR * 1000000 + QB_LINQ_VERSION_MINOR * 1000 + QB_LINQ_VERSION_PATCH);
}
