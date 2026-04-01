/// @file linq_property_fuzz_test.cpp
/// @brief Randomized equivalence vs hand-written references (set ops, join, zip, pipeline shapes).

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "qb/linq.h"

namespace {

std::vector<int> iota_vec(int start, int count)
{
    std::vector<int> out(static_cast<std::size_t>(count));
    std::iota(out.begin(), out.end(), start);
    return out;
}

std::vector<int> ref_pipeline_a(std::vector<int> const& in)
{
    std::vector<int> tmp;
    for (int x : in) {
        if (x % 3 != 0)
            tmp.push_back(x * 2);
    }
    std::vector<int> out;
    for (std::size_t i = 2; i < tmp.size() && out.size() < 5; ++i)
        out.push_back(tmp[i]);
    return out;
}

std::vector<int> ref_pipeline_b(std::vector<int> const& in)
{
    std::vector<int> out;
    for (int x : in) {
        if (x >= 0 && (x % 2 == 0))
            out.push_back(x + 7);
    }
    if (out.size() > 8)
        out.resize(8);
    return out;
}

std::vector<int> ref_pipeline_c(std::vector<int> const& in)
{
    std::vector<int> out;
    for (int x : in) {
        if ((x % 2) != 0) {
            int t = x * 5 - 3;
            if (t > 10)
                out.push_back(t);
        }
    }
    if (out.size() > 4)
        out.erase(out.begin(), out.begin() + 4);
    else
        out.clear();
    if (out.size() > 17)
        out.resize(17);
    return out;
}

std::vector<int> ref_deep_pipeline_1(std::vector<int> const& v)
{
    std::vector<int> ref;
    for (int x : v) {
        if (x % 2 == 0) {
            x = x * 3;
            if (x >= 0) {
                x = x / 2;
                ref.push_back(x);
            }
        }
    }
    if (ref.size() > 3)
        ref.erase(ref.begin(), ref.begin() + 3);
    else
        ref.clear();
    if (ref.size() > 10)
        ref.resize(10);
    return ref;
}

std::vector<int> ref_deep_pipeline_2(std::vector<int> const& v)
{
    std::vector<int> ref;
    for (int x : v) {
        if ((x % 3) != 0) {
            x = x * 2 + 1;
            if ((x % 5) != 0) {
                x = x - 7;
                ref.push_back(x);
            }
        }
    }
    if (ref.size() > 100)
        ref.erase(ref.begin(), ref.begin() + 100);
    else
        ref.clear();
    if (ref.size() > 500)
        ref.resize(500);
    return ref;
}

template <class T>
std::vector<T> dotnet_union_ref(std::vector<T> const& a, std::vector<T> const& b)
{
    std::unordered_set<T> seen;
    std::vector<T> out;
    out.reserve(a.size() + b.size());
    for (auto const& x : a) {
        if (seen.insert(x).second)
            out.push_back(x);
    }
    for (auto const& x : b) {
        if (seen.insert(x).second)
            out.push_back(x);
    }
    return out;
}

template <class T>
std::vector<T> dotnet_except_ref(std::vector<T> const& a, std::vector<T> const& b)
{
    std::unordered_set<T> ban(b.begin(), b.end());
    std::unordered_set<T> yielded;
    std::vector<T> out;
    for (auto const& x : a) {
        if (ban.count(x))
            continue;
        if (yielded.insert(x).second)
            out.push_back(x);
    }
    return out;
}

template <class T>
std::vector<T> dotnet_intersect_ref(std::vector<T> const& a, std::vector<T> const& b)
{
    std::unordered_set<T> want(b.begin(), b.end());
    std::unordered_set<T> yielded;
    std::vector<T> out;
    for (auto const& x : a) {
        if (!want.count(x))
            continue;
        if (yielded.insert(x).second)
            out.push_back(x);
    }
    return out;
}

struct fuzz_user {
    int id = 0;
    std::string name;
    int team_id = 0;
    int age = 0;
};

struct fuzz_team {
    int id = 0;
    std::string name;
};

} // namespace

TEST(PropertyFuzz, WhereSelectSkipTakeMatchesReferenceA)
{
    std::mt19937 rng(123456);
    std::uniform_int_distribution<int> size_dist(0, 128);
    std::uniform_int_distribution<int> val_dist(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> v(static_cast<std::size_t>(size_dist(rng)));
        for (auto& x : v)
            x = val_dist(rng);

        auto got = qb::linq::from(v)
                       .where([](int x) { return x % 3 != 0; })
                       .select([](int x) { return x * 2; })
                       .skip(2)
                       .take(5)
                       .to_vector();

        auto want = ref_pipeline_a(v);
        ASSERT_TRUE(qb::linq::from(got).sequence_equal(want)) << "iter " << iter;
    }
}

TEST(PropertyFuzz, DoubleWhereSelectTakeMatchesReferenceB)
{
    std::mt19937 rng(987654);
    std::uniform_int_distribution<int> size_dist(0, 256);
    std::uniform_int_distribution<int> val_dist(-5000, 5000);

    for (int iter = 0; iter < 350; ++iter) {
        std::vector<int> v(static_cast<std::size_t>(size_dist(rng)));
        for (auto& x : v)
            x = val_dist(rng);

        auto got = qb::linq::from(v)
                       .where([](int x) { return x >= 0; })
                       .where([](int x) { return (x % 2) == 0; })
                       .select([](int x) { return x + 7; })
                       .take(8)
                       .to_vector();

        auto want = ref_pipeline_b(v);
        ASSERT_TRUE(qb::linq::from(got).sequence_equal(want)) << "iter " << iter;
    }
}

TEST(PropertyFuzz, WhereSelectWhereSkipTakeMatchesReferenceC)
{
    std::mt19937 rng(314159);
    std::uniform_int_distribution<int> size_dist(0, 900);
    std::uniform_int_distribution<int> val_dist(-50000, 50000);

    for (int iter = 0; iter < 120; ++iter) {
        std::vector<int> v(static_cast<std::size_t>(size_dist(rng)));
        for (auto& x : v)
            x = val_dist(rng);

        auto got = qb::linq::from(v)
                       .where([](int x) { return (x % 2) != 0; })
                       .select([](int x) { return x * 5 - 3; })
                       .where([](int x) { return x > 10; })
                       .skip(4)
                       .take(17)
                       .to_vector();

        auto want = ref_pipeline_c(v);
        ASSERT_TRUE(qb::linq::from(got).sequence_equal(want)) << "iter " << iter;
    }
}

TEST(PropertyFuzz, DeepChainMatchesReferenceLoop)
{
    auto v = iota_vec(-20, 50);
    auto got = qb::linq::from(v)
                   .where([](int x) { return x % 2 == 0; })
                   .select([](int x) { return x * 3; })
                   .where([](int x) { return x >= 0; })
                   .select([](int x) { return x / 2; })
                   .skip(3)
                   .take(10)
                   .to_vector();
    auto want = ref_deep_pipeline_1(v);
    EXPECT_TRUE(qb::linq::from(got).sequence_equal(want));
}

TEST(PropertyFuzz, DeepChainLargeMatchesReferenceLoop)
{
    auto v = iota_vec(0, 5000);
    auto got = qb::linq::from(v)
                   .where([](int x) { return (x % 3) != 0; })
                   .select([](int x) { return x * 2 + 1; })
                   .where([](int x) { return (x % 5) != 0; })
                   .select([](int x) { return x - 7; })
                   .skip(100)
                   .take(500)
                   .to_vector();
    auto want = ref_deep_pipeline_2(v);
    EXPECT_TRUE(qb::linq::from(got).sequence_equal(want));
}

TEST(PropertyFuzz, SetOpsMatchDotNetStyleReference)
{
    std::mt19937 rng(24680);
    std::uniform_int_distribution<int> size_dist(0, 80);
    std::uniform_int_distribution<int> val_dist(-25, 25);

    for (int iter = 0; iter < 200; ++iter) {
        std::vector<int> a(static_cast<std::size_t>(size_dist(rng)));
        std::vector<int> b(static_cast<std::size_t>(size_dist(rng)));
        for (auto& x : a)
            x = val_dist(rng);
        for (auto& x : b)
            x = val_dist(rng);

        auto got_u = qb::linq::from(a).union_with(b).to_vector();
        auto got_e = qb::linq::from(a).except(b).to_vector();
        auto got_i = qb::linq::from(a).intersect(b).to_vector();

        auto want_u = dotnet_union_ref(a, b);
        auto want_e = dotnet_except_ref(a, b);
        auto want_i = dotnet_intersect_ref(a, b);

        EXPECT_TRUE(qb::linq::from(got_u).sequence_equal(want_u)) << "union iter " << iter;
        EXPECT_TRUE(qb::linq::from(got_e).sequence_equal(want_e)) << "except iter " << iter;
        EXPECT_TRUE(qb::linq::from(got_i).sequence_equal(want_i)) << "intersect iter " << iter;
    }
}

TEST(PropertyFuzz, JoinMatchesNestedLoopReference)
{
    std::mt19937 rng(112233);
    std::uniform_int_distribution<int> left_size_dist(0, 24);
    std::uniform_int_distribution<int> right_size_dist(0, 12);
    std::uniform_int_distribution<int> key_dist(0, 8);

    for (int iter = 0; iter < 200; ++iter) {
        std::vector<fuzz_user> users;
        std::vector<fuzz_team> teams;
        int const left_n = left_size_dist(rng);
        int const right_n = right_size_dist(rng);
        for (int i = 0; i < left_n; ++i) {
            users.push_back(
                fuzz_user{i, "u" + std::to_string(i), key_dist(rng), 20 + (i % 10)});
        }
        for (int i = 0; i < right_n; ++i) {
            teams.push_back(fuzz_team{key_dist(rng), "t" + std::to_string(i)});
        }

        auto got = qb::linq::from(users)
                       .join(
                           teams,
                           [](fuzz_user const& u) { return u.team_id; },
                           [](fuzz_team const& t) { return t.id; },
                           [](fuzz_user const& u, fuzz_team const& t) {
                               return u.name + ":" + t.name;
                           })
                       .to_vector();

        std::vector<std::string> want;
        for (auto const& u : users) {
            for (auto const& t : teams) {
                if (u.team_id == t.id)
                    want.push_back(u.name + ":" + t.name);
            }
        }
        std::vector<std::string> gs;
        std::vector<std::string> ws;
        for (auto const& s : got)
            gs.push_back(s);
        for (auto const& s : want)
            ws.push_back(s);
        std::sort(gs.begin(), gs.end());
        std::sort(ws.begin(), ws.end());
        EXPECT_EQ(gs, ws) << "iter " << iter;
    }
}

TEST(PropertyFuzz, ZipPairsMatchLengthsAndValues)
{
    std::mt19937 rng(271828);
    std::uniform_int_distribution<int> size_dist(0, 200);

    for (int iter = 0; iter < 120; ++iter) {
        int const na = size_dist(rng);
        int const nb = size_dist(rng);
        auto a = iota_vec(0, na);
        auto b = iota_vec(1000, nb);

        auto pairs = qb::linq::from(a).zip(b).to_vector();
        std::size_t const expect_n = static_cast<std::size_t>((std::min)(na, nb));
        ASSERT_EQ(pairs.long_count(), expect_n) << "iter " << iter;

        std::size_t i = 0;
        for (auto const& pr : pairs) {
            EXPECT_EQ(pr.first, a[i]) << "iter " << iter << " i " << i;
            EXPECT_EQ(pr.second, b[i]) << "iter " << iter << " i " << i;
            ++i;
        }
    }
}
