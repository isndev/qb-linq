/// @file linq_property_fuzz_test.cpp
/// @brief Randomized equivalence vs hand-written references (set ops, join, zip, pipeline shapes).

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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

struct fuzz_kv {
    int key = 0;
    int payload = 0;
};

inline bool operator==(fuzz_kv const& a, fuzz_kv const& b) noexcept
{
    return a.key == b.key && a.payload == b.payload;
}

inline bool operator!=(fuzz_kv const& a, fuzz_kv const& b) noexcept
{
    return !(a == b);
}

static std::vector<std::pair<int, int>> ref_count_by_pairs(std::vector<int> const& keys)
{
    std::unordered_map<int, int> counts;
    std::vector<int> order;
    order.reserve(keys.size());
    for (int k : keys) {
        auto const ins = counts.try_emplace(k, 0);
        if (ins.second)
            order.push_back(k);
        ++ins.first->second;
    }
    std::vector<std::pair<int, int>> out;
    out.reserve(order.size());
    for (int k : order)
        out.emplace_back(k, counts.find(k)->second);
    return out;
}

static std::vector<fuzz_kv> ref_except_by(std::vector<fuzz_kv> const& left, std::vector<int> const& right_keys)
{
    std::unordered_set<int> ban(right_keys.begin(), right_keys.end());
    std::unordered_set<int> yielded;
    std::vector<fuzz_kv> out;
    for (auto const& x : left) {
        if (ban.count(x.key))
            continue;
        if (yielded.insert(x.key).second)
            out.push_back(x);
    }
    return out;
}

static std::vector<fuzz_kv> ref_intersect_by(std::vector<fuzz_kv> const& left, std::vector<int> const& right_keys)
{
    std::unordered_set<int> want(right_keys.begin(), right_keys.end());
    std::unordered_set<int> yielded;
    std::vector<fuzz_kv> out;
    for (auto const& x : left) {
        if (!want.count(x.key))
            continue;
        if (yielded.insert(x.key).second)
            out.push_back(x);
    }
    return out;
}

static std::vector<fuzz_kv> ref_union_by(std::vector<fuzz_kv> const& a, std::vector<fuzz_kv> const& b)
{
    std::unordered_set<int> seen;
    std::vector<fuzz_kv> out;
    for (auto const& x : a) {
        if (seen.insert(x.key).second)
            out.push_back(x);
    }
    for (auto const& x : b) {
        if (seen.insert(x.key).second)
            out.push_back(x);
    }
    return out;
}

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

/** Independent row-count check (catches cardinality bugs without relying on string multiset equality). */
TEST(PropertyFuzz, InnerJoinRowCountMatchesNestedLoop)
{
    std::mt19937 rng(778899);
    std::uniform_int_distribution<int> left_size_dist(0, 28);
    std::uniform_int_distribution<int> right_size_dist(0, 15);
    std::uniform_int_distribution<int> key_dist(0, 9);

    for (int iter = 0; iter < 250; ++iter) {
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

        std::size_t want = 0;
        for (auto const& u : users) {
            for (auto const& t : teams) {
                if (u.team_id == t.id)
                    ++want;
            }
        }

        auto const got = qb::linq::from(users)
                             .join(
                                 teams,
                                 [](fuzz_user const& u) { return u.team_id; },
                                 [](fuzz_team const& t) { return t.id; },
                                 [](fuzz_user const&, fuzz_team const&) { return 0; })
                             .long_count();
        EXPECT_EQ(got, want) << "inner join row count iter " << iter;
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

TEST(PropertyFuzz, LeftJoinMatchesNestedLoopReference)
{
    std::mt19937 rng(334455);
    std::uniform_int_distribution<int> left_size_dist(0, 20);
    std::uniform_int_distribution<int> right_size_dist(0, 14);
    std::uniform_int_distribution<int> key_dist(0, 7);

    for (int iter = 0; iter < 220; ++iter) {
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
                       .left_join(
                           teams,
                           [](fuzz_user const& u) { return u.team_id; },
                           [](fuzz_team const& t) { return t.id; },
                           [](fuzz_user const& u, std::optional<fuzz_team> const& ot) {
                               return u.name + ":" + (ot ? ot->name : std::string("<none>"));
                           })
                       .to_vector();

        std::vector<std::string> want;
        for (auto const& u : users) {
            bool any = false;
            for (auto const& t : teams) {
                if (u.team_id == t.id) {
                    want.push_back(u.name + ":" + t.name);
                    any = true;
                }
            }
            if (!any)
                want.push_back(u.name + ":<none>");
        }
        std::vector<std::string> gs;
        std::vector<std::string> ws;
        for (auto const& s : got)
            gs.push_back(s);
        for (auto const& s : want)
            ws.push_back(s);
        std::sort(gs.begin(), gs.end());
        std::sort(ws.begin(), ws.end());
        EXPECT_EQ(gs, ws) << "left_join iter " << iter;
    }
}

TEST(PropertyFuzz, RightJoinMatchesNestedLoopReference)
{
    std::mt19937 rng(556677);
    std::uniform_int_distribution<int> left_size_dist(0, 18);
    std::uniform_int_distribution<int> right_size_dist(0, 16);
    std::uniform_int_distribution<int> key_dist(0, 6);

    for (int iter = 0; iter < 220; ++iter) {
        std::vector<fuzz_user> users;
        std::vector<fuzz_team> teams;
        int const left_n = left_size_dist(rng);
        int const right_n = right_size_dist(rng);
        for (int i = 0; i < left_n; ++i) {
            users.push_back(
                fuzz_user{i, "u" + std::to_string(i), key_dist(rng), 20 + (i % 7)});
        }
        for (int i = 0; i < right_n; ++i) {
            teams.push_back(fuzz_team{key_dist(rng), "t" + std::to_string(i)});
        }

        auto got = qb::linq::from(users)
                       .right_join(
                           teams,
                           [](fuzz_user const& u) { return u.team_id; },
                           [](fuzz_team const& t) { return t.id; },
                           [](std::optional<fuzz_user> const& ou, fuzz_team const& t) {
                               return (ou ? ou->name : std::string("<none>")) + ":" + t.name;
                           })
                       .to_vector();

        std::vector<std::string> want;
        for (auto const& t : teams) {
            bool any = false;
            for (auto const& u : users) {
                if (u.team_id == t.id) {
                    want.push_back(u.name + ":" + t.name);
                    any = true;
                }
            }
            if (!any)
                want.push_back("<none>:" + t.name);
        }
        std::vector<std::string> gs;
        std::vector<std::string> ws;
        for (auto const& s : got)
            gs.push_back(s);
        for (auto const& s : want)
            ws.push_back(s);
        std::sort(gs.begin(), gs.end());
        std::sort(ws.begin(), ws.end());
        EXPECT_EQ(gs, ws) << "right_join iter " << iter;
    }
}

TEST(PropertyFuzz, ZipTripleMatchesManualMinLength)
{
    std::mt19937 rng(998877);
    std::uniform_int_distribution<int> size_dist(0, 120);

    for (int iter = 0; iter < 100; ++iter) {
        int const na = size_dist(rng);
        int const nb = size_dist(rng);
        int const nc = size_dist(rng);
        auto a = iota_vec(10, na);
        auto b = iota_vec(100, nb);
        auto c = iota_vec(1000, nc);
        std::size_t const expect_n = static_cast<std::size_t>((std::min)({na, nb, nc}));

        auto triples = qb::linq::from(a).zip(b, c).to_vector();
        ASSERT_EQ(triples.long_count(), expect_n) << "iter " << iter;

        std::size_t i = 0;
        for (auto const& t : triples) {
            EXPECT_EQ(std::get<0>(t), a[i]) << "iter " << iter << " i " << i;
            EXPECT_EQ(std::get<1>(t), b[i]) << "iter " << iter << " i " << i;
            EXPECT_EQ(std::get<2>(t), c[i]) << "iter " << iter << " i " << i;
            ++i;
        }
    }
}

TEST(PropertyFuzz, CountByOrderAndTotalsMatchReference)
{
    std::mt19937 rng(424242);
    std::uniform_int_distribution<int> len_dist(0, 90);
    std::uniform_int_distribution<int> key_dist(0, 12);

    for (int iter = 0; iter < 160; ++iter) {
        std::vector<int> keys(static_cast<std::size_t>(len_dist(rng)));
        for (auto& k : keys)
            k = key_dist(rng);

        auto got = qb::linq::from(keys).count_by([](int x) { return x; }).to_vector();
        auto want = ref_count_by_pairs(keys);

        ASSERT_EQ(got.long_count(), want.size()) << "iter " << iter;
        std::size_t i = 0;
        for (auto const& pr : got) {
            EXPECT_EQ(pr.first, want[i].first) << "iter " << iter << " i " << i;
            EXPECT_EQ(pr.second, want[i].second) << "iter " << iter << " i " << i;
            ++i;
        }
    }
}

TEST(PropertyFuzz, ExceptByIntersectByUnionByMatchReference)
{
    std::mt19937 rng(135790);
    std::uniform_int_distribution<int> left_n_dist(0, 40);
    std::uniform_int_distribution<int> right_n_dist(0, 25);
    std::uniform_int_distribution<int> key_dist(-5, 15);
    std::uniform_int_distribution<int> pay_dist(-99, 99);

    for (int iter = 0; iter < 140; ++iter) {
        std::vector<fuzz_kv> left(static_cast<std::size_t>(left_n_dist(rng)));
        std::vector<int> right_keys(static_cast<std::size_t>(right_n_dist(rng)));
        for (auto& x : left) {
            x.key = key_dist(rng);
            x.payload = pay_dist(rng);
        }
        for (auto& k : right_keys)
            k = key_dist(rng);

        auto got_e = qb::linq::from(left)
                         .except_by(right_keys, [](fuzz_kv const& x) { return x.key; }, [](int k) { return k; })
                         .to_vector();
        auto got_i = qb::linq::from(left)
                         .intersect_by(right_keys, [](fuzz_kv const& x) { return x.key; }, [](int k) { return k; })
                         .to_vector();

        auto want_e = ref_except_by(left, right_keys);
        auto want_i = ref_intersect_by(left, right_keys);

        auto vec_kv = [](auto const& e) {
            std::vector<fuzz_kv> v;
            for (auto const& x : e)
                v.push_back(x);
            return v;
        };
        auto sort_kv = [](std::vector<fuzz_kv>& v) {
            std::sort(v.begin(), v.end(), [](fuzz_kv const& a, fuzz_kv const& b) {
                if (a.key != b.key)
                    return a.key < b.key;
                return a.payload < b.payload;
            });
        };

        std::vector<fuzz_kv> ge = vec_kv(got_e);
        std::vector<fuzz_kv> gi = vec_kv(got_i);
        sort_kv(ge);
        sort_kv(gi);
        std::vector<fuzz_kv> we = want_e;
        std::vector<fuzz_kv> wi = want_i;
        sort_kv(we);
        sort_kv(wi);
        EXPECT_EQ(ge, we) << "except_by iter " << iter;
        EXPECT_EQ(gi, wi) << "intersect_by iter " << iter;
    }

    for (int iter = 0; iter < 120; ++iter) {
        std::vector<fuzz_kv> a(static_cast<std::size_t>(left_n_dist(rng)));
        std::vector<fuzz_kv> b(static_cast<std::size_t>(right_n_dist(rng)));
        for (auto& x : a) {
            x.key = key_dist(rng);
            x.payload = pay_dist(rng);
        }
        for (auto& x : b) {
            x.key = key_dist(rng);
            x.payload = pay_dist(rng);
        }
        auto got = qb::linq::from(a)
                       .union_by(b, [](fuzz_kv const& x) { return x.key; }, [](fuzz_kv const& x) { return x.key; })
                       .to_vector();
        auto want = ref_union_by(a, b);
        auto vec_kv = [](auto const& e) {
            std::vector<fuzz_kv> v;
            for (auto const& x : e)
                v.push_back(x);
            return v;
        };
        auto sort_kv = [](std::vector<fuzz_kv>& v) {
            std::sort(v.begin(), v.end(), [](fuzz_kv const& x, fuzz_kv const& y) {
                if (x.key != y.key)
                    return x.key < y.key;
                return x.payload < y.payload;
            });
        };
        std::vector<fuzz_kv> gg = vec_kv(got);
        std::vector<fuzz_kv> ww = want;
        sort_kv(gg);
        sort_kv(ww);
        EXPECT_EQ(gg, ww) << "union_by iter " << iter;
    }
}
