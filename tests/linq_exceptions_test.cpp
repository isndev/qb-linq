/// @file linq_exceptions_test.cpp
/// @brief User callables may throw; terminals must propagate (no swallowed errors).

#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

TEST(ExceptionsSelect, ProjectionPropagates)
{
    std::vector<int> const src{1, 2, 3, 4, 5};
    EXPECT_THROW(
        (void)qb::linq::from(src)
            .select([](int x) {
                if (x == 4)
                    throw std::runtime_error("select boom");
                return x * 2;
            })
            .to_vector(),
        std::runtime_error);
}

TEST(ExceptionsWhere, PredicatePropagates)
{
    std::vector<int> const src{1, 2, 3, 4, 5, 6};
    EXPECT_THROW(
        (void)qb::linq::from(src)
            .where([](int x) {
                if (x == 5)
                    throw std::runtime_error("where boom");
                return x % 2 == 0;
            })
            .to_vector(),
        std::runtime_error);
}

struct ex_user {
    int id = 0;
    std::string name;
    int team_id = 0;
    int age = 0;
};

struct ex_team {
    int id = 0;
    std::string name;
};

struct heavy_row {
    int id = 0;
    std::string a;
    std::string b;
    std::string c;
};

TEST(ExceptionsSelect, HeavyProjectionPropagates)
{
    std::vector<heavy_row> v;
    for (int i = 0; i < 80; ++i)
        v.push_back(heavy_row{i, "a" + std::to_string(i), "b" + std::to_string(i), "c" + std::to_string(i)});

    EXPECT_THROW(
        (void)qb::linq::from(v)
            .select([](heavy_row const& h) {
                if (h.id == 57)
                    throw std::runtime_error("heavy boom");
                return h.a + h.b + h.c;
            })
            .to_vector(),
        std::runtime_error);
}

TEST(ExceptionsJoin, ResultSelectorPropagates)
{
    std::vector<ex_user> const users{
        {1, "alice", 10, 31},
        {2, "bob", 20, 22},
        {3, "cara", 10, 27},
    };
    std::vector<ex_team> const teams{
        {10, "engine"},
        {20, "design"},
    };

    EXPECT_THROW(
        (void)qb::linq::from(users)
            .join(
                teams,
                [](ex_user const& u) { return u.team_id; },
                [](ex_team const& t) { return t.id; },
                [](ex_user const& u, ex_team const& t) {
                    if (u.name == "cara")
                        throw std::runtime_error("join result boom");
                    return t.name + ":" + u.name;
                })
            .to_vector(),
        std::runtime_error);
}
