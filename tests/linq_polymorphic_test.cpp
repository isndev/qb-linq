/// @file linq_polymorphic_test.cpp
/// @brief of_type<U> filtering for polymorphic pointer ranges (dynamic_cast).

#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

namespace {

struct base {
    virtual ~base() = default;
    int tag = 0;
};

struct derived_a : base {
    derived_a() { tag = 1; }
};

struct derived_b : base {
    derived_b() { tag = 2; }
};

} // namespace

TEST(PolymorphicOfType, KeepsMatchingDynamicType)
{
    derived_a a;
    derived_b b;
    std::vector<base*> const ptrs{&a, &b, &a};
    auto filtered = qb::linq::from(ptrs).template of_type<derived_a>().to_vector();
    ASSERT_EQ(filtered.long_count(), 2u);
    EXPECT_EQ(filtered.element_at(0)->tag, 1);
    EXPECT_EQ(filtered.element_at(1)->tag, 1);
}

TEST(PolymorphicOfType, NullPointersExcluded)
{
    derived_a a;
    std::vector<base*> const ptrs{nullptr, &a, nullptr};
    auto filtered = qb::linq::from(ptrs).template of_type<derived_a>().to_vector();
    EXPECT_EQ(filtered.long_count(), 1u);
}

TEST(PolymorphicOfType, NoMatchEmpty)
{
    derived_b b;
    std::vector<base*> const ptrs{&b};
    auto filtered = qb::linq::from(ptrs).template of_type<derived_a>().to_vector();
    EXPECT_EQ(filtered.long_count(), 0u);
}
