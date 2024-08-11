/**
 * @file art tests
 * @author ---
 *
 * @author Rafael Kallis <rk@rafaelkallis.com>
 */

#include <set>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "art.hpp"
#include "doctest.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>
#include <memory>

using std::array;
using std::hash;
using std::mt19937;
using std::mt19937_64;
using std::random_device;
using std::shuffle;
using std::string;
using std::to_string;

TEST_SUITE("art") {

    TEST_CASE("set") {

        art::art<int *> trie;

        int dummy_value_1;
        int dummy_value_2;

        SUBCASE("insert into empty tree") {
            REQUIRE_EQ(nullptr, trie.get(reinterpret_cast<const uint8_t *>("abc")));
            trie.set(reinterpret_cast<const uint8_t *>("abc"), &dummy_value_1);
            REQUIRE_EQ(&dummy_value_1, trie.get(reinterpret_cast<const uint8_t *>("abc")));
        }

        SUBCASE("insert into empty tree & replace") {
            trie.set(reinterpret_cast<const uint8_t *>("abc"), &dummy_value_1);
            trie.set(reinterpret_cast<const uint8_t *>("abc"), &dummy_value_2);
            REQUIRE_EQ(&dummy_value_2, trie.get(reinterpret_cast<const uint8_t *>("abc")));
        }

        SUBCASE("insert value s.t. existing value is a prefix") {
            const uint8_t * prefix_key = reinterpret_cast<const uint8_t *>("abc");
            const uint8_t * key = reinterpret_cast<const uint8_t *>("abcde");
            trie.set(prefix_key, &dummy_value_1);
            trie.set(key, &dummy_value_2);
            REQUIRE_EQ(&dummy_value_1, trie.get(prefix_key));
            REQUIRE_EQ(&dummy_value_2, trie.get(key));
        }

        SUBCASE("insert value s.t. new value is a prefix") {
            trie.set(reinterpret_cast<const uint8_t *>("abcde"), &dummy_value_1);
            trie.set(reinterpret_cast<const uint8_t *>("abc"), &dummy_value_2);
            REQUIRE_EQ(&dummy_value_1, trie.get(reinterpret_cast<const uint8_t *>("abcde")));
            REQUIRE_EQ(&dummy_value_2, trie.get(reinterpret_cast<const uint8_t *>("abc")));
        }

        SUBCASE("insert key s.t. it mismatches existing key") {
            const uint8_t *key1 = reinterpret_cast<const uint8_t *>("aaaaa");
            const uint8_t *key2 = reinterpret_cast<const uint8_t *>("aabaa");
            trie.set(key1, &dummy_value_1);
            trie.set(key2, &dummy_value_2);
            REQUIRE_EQ(&dummy_value_1, trie.get(key1));
            REQUIRE_EQ(&dummy_value_2, trie.get(key2));
        }

        SUBCASE("monte carlo") {
            const int n = 1000;
            string keys[n];
            int *values[n];
            /* rng */
            mt19937_64 g(0);
            for (int experiment = 0; experiment < 10; experiment += 1) {
                for (int i = 0; i < n; i += 1) {
                    keys[i] = to_string(g());
                    values[i] = new int();
                }

                art::art<int*> m;

                for (int i = 0; i < n; i += 1) {
                    m.set(reinterpret_cast<const uint8_t *>(keys[i].c_str()), values[i]);

                    for (int j = 0; j < i; j += 1) {
                        REQUIRE_EQ(values[j], m.get(reinterpret_cast<const uint8_t *>(keys[j].c_str())));
                    }
                }

                for (int i = 0; i < n; i += 1) {
                    delete values[i];
                }
            }
        }
    }

    TEST_CASE("delete value") {

        auto key0 = reinterpret_cast<const uint8_t *>("aad");
        auto int0 = 0;
        auto key1 = reinterpret_cast<const uint8_t *>("aaaad");
        auto int1 = 1;
        auto key2 = reinterpret_cast<const uint8_t *>("aaaaaaad");
        auto int2 = 2;
        auto key3 = reinterpret_cast<const uint8_t *>("aaaaaaaaaad");
        auto int3 = 3;
        auto key4 = reinterpret_cast<const uint8_t *>("aaaaaaabad");
        auto int4 = 4;
        auto key5 = reinterpret_cast<const uint8_t *>("aaaabaad");
        auto int5 = 5;
        auto key6 = reinterpret_cast<const uint8_t *>("aaaabaaaaad");
        auto int6 = 6;
        auto key7 = reinterpret_cast<const uint8_t *>("aaaaaaaaaaad");
        auto int7 = 7;
        auto key8 = reinterpret_cast<const uint8_t *>("aaaaaaaaaabd");
        auto int8 = 8;
        auto key9 = reinterpret_cast<const uint8_t *>("aaaaaaaaaacd");
        auto int9 = 9;
        auto key10 = reinterpret_cast<const uint8_t *>("aaeaaaaaad");
        auto int10 = 10;
        auto key11 = reinterpret_cast<const uint8_t *>("aaeaad");
        auto int11 = 11;

        auto pref_key0 = reinterpret_cast<const uint8_t *>("aaeaa");
        auto pref_int0 = 20;
        auto pref_key1 = reinterpret_cast<const uint8_t *>("aaeaaaa");
        auto pref_int1 = 21;

        art::art<int*> m;

        m.set(key0, &int0);
        m.set(key1, &int1);
        m.set(key2, &int2);
        m.set(key3, &int3);
        m.set(key4, &int4);
        m.set(key5, &int5);
        m.set(key6, &int6);
        m.set(key7, &int7);
        m.set(key8, &int8);
        m.set(key9, &int9);
        m.set(key10, &int10);
        m.set(key11, &int11);
        m.set(pref_key0, &pref_int0);
        m.set(pref_key1, &pref_int1);

        /* The above statements construct the following tree:
         *        
         *                                    (aa)_________________________e
         *                                     |a \____________d           \
         *                                     |               \          (aa)->20
         *                                    (a)              ()->0     a/ \d
         *                    a______________/|b\___________d            /  ()->11
         *                    /               |              \         (a)->21
         *                  (aa)             (aa)            ()->1      |a
         *          a______/ |b \___d         |a \____d                (ad)->10
         *          /        |      \         |       \
         *        (aa)     (ad)->4 ()->2    (aad)->6  ()->5
         *   a___/ |b \___c                
         *   /     |      \               
         *  (d)->7 (d)->8 ()->9          
         */

        SUBCASE("delete non existing value") {
            REQUIRE_EQ(m.del(reinterpret_cast<const uint8_t *>("aaaaad")), nullptr);
            REQUIRE_EQ(m.del(reinterpret_cast<const uint8_t *>("aaaaaad")), nullptr);
            REQUIRE_EQ(m.del(reinterpret_cast<const uint8_t *>("aaaabd")), nullptr);
            REQUIRE_EQ(m.del(reinterpret_cast<const uint8_t *>("aaaabad")), nullptr);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }

        SUBCASE("n_children == 0 && n_siblings == 0 (6)") {
            REQUIRE_EQ(m.del(key6), &int6);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), nullptr);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }

        SUBCASE("n_children == 0 && n_siblings == 1 (4)") {
            REQUIRE_EQ(m.del(key4), &int4);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), nullptr);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }

        SUBCASE("n_children == 0 && n_siblings > 1 (7)") {
            REQUIRE_EQ(m.del(key7), &int7);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), nullptr);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }

        SUBCASE("n_children == 1 (0),(5)") {
            REQUIRE_EQ(m.del(key0), &int0);
            REQUIRE_EQ(m.get(key0), nullptr);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);

            REQUIRE_EQ(m.del(key5), &int5);
            REQUIRE_EQ(m.get(key0), nullptr);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), nullptr);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }

        SUBCASE("n_children > 1 (3),(2),(1)") {
            REQUIRE_EQ(m.del(key3), &int3);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), nullptr);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);

            REQUIRE_EQ(m.del(key2), &int2);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), nullptr);
            REQUIRE_EQ(m.get(key3), nullptr);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);

            REQUIRE_EQ(m.del(key1), &int1);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), nullptr);
            REQUIRE_EQ(m.get(key2), nullptr);
            REQUIRE_EQ(m.get(key3), nullptr);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }

        SUBCASE("prefix key n_children > 1 (20)") {
            REQUIRE_EQ(m.del(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), nullptr);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }

        SUBCASE("prefix key n_children == 0 (21)") {
            REQUIRE_EQ(m.del(pref_key1), &pref_int1);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), nullptr);
        }


        SUBCASE("leaf child of valued node n_siblings > 0 (11)") {
            REQUIRE_EQ(m.del(key11), &int11);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), &int10);
            REQUIRE_EQ(m.get(key11), nullptr);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }

        SUBCASE("leaf child of valued node n_siblings == 0 (11)") {
            REQUIRE_EQ(m.del(key10), &int10);
            REQUIRE_EQ(m.get(key0), &int0);
            REQUIRE_EQ(m.get(key1), &int1);
            REQUIRE_EQ(m.get(key2), &int2);
            REQUIRE_EQ(m.get(key3), &int3);
            REQUIRE_EQ(m.get(key4), &int4);
            REQUIRE_EQ(m.get(key5), &int5);
            REQUIRE_EQ(m.get(key6), &int6);
            REQUIRE_EQ(m.get(key7), &int7);
            REQUIRE_EQ(m.get(key8), &int8);
            REQUIRE_EQ(m.get(key9), &int9);
            REQUIRE_EQ(m.get(key10), nullptr);
            REQUIRE_EQ(m.get(key11), &int11);
            REQUIRE_EQ(m.get(pref_key0), &pref_int0);
            REQUIRE_EQ(m.get(pref_key1), &pref_int1);
        }
    }

    TEST_CASE("monte carlo delete") {
        const uint32_t n = 1000000;
        uint64_t rng_vals[n];
        mt19937_64 rng(0);
        std::set<uint64_t> dup_checker;
        for (int32_t i = 0; i < n; i++) {
            do {
                rng_vals[i] = rng();
            } while (dup_checker.find(rng_vals[i]) != dup_checker.end());
            dup_checker.insert(rng_vals[i]);
        }

        art::art<int*> m;
        for (int i = 0; i < n; ++i) {
            auto k = to_string(rng_vals[i]);
            auto v = new int();
            *v = i;
            REQUIRE_EQ(m.set(reinterpret_cast<const uint8_t *>(k.c_str()), v), nullptr);
        }
        for (int i = 0; i < n; ++i) {
            auto k = to_string(rng_vals[i]);
            auto get_res = m.get(reinterpret_cast<const uint8_t *>(k.c_str()));
            auto del_res = m.del(reinterpret_cast<const uint8_t *>(k.c_str()));
            REQUIRE_EQ(m.get(reinterpret_cast<const uint8_t *>(k.c_str())), nullptr);
            REQUIRE_EQ(get_res, del_res);
            REQUIRE_NE(del_res, nullptr);
            REQUIRE_EQ(*del_res, i);
            delete del_res;
        }
    }
}

