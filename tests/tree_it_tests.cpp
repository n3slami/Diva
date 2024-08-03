/**
 * @file tree_it tests
 * @author ---
 *
 * @author Rafael Kallis <rk@rafaelkallis.com>
 */

#include <cstdint>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "art.hpp"
#include "doctest.h"
#include <algorithm>
#include <array>
#include <iostream>
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

TEST_SUITE("tree_it") {
  TEST_CASE("full lexicographic traversal") {
    SUBCASE("controlled test") {
      int int0 = 0;
      int int1 = 1;
      int int2 = 2;
      int int3 = 4;
      int int4 = 5;
      int int5 = 5;
      int int6 = 6;

      art::art<int*> m;

      m.set(reinterpret_cast<const uint8_t *>("aa"), &int0);
      m.set(reinterpret_cast<const uint8_t *>("aaaa"), &int1);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaa"), &int2);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaaaa"), &int3);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaba"), &int4);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaa"), &int5);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaaaaa"), &int6);

      /* The above statements construct the following tree:
       *
       *          (aa)
       *   $_____/ |a
       *   /       |
       *  ()->0   (a)
       *   $_____/ |a\____________b
       *   /       |              \
       *  ()->1   (aa)            (aa)
       *   $_____/ |a\___b         |$\____a
       *   /       |     \         |      \
       *  ()->2 (aa$)->3 (a$)->4 ()->5 (aa$)->6
       *
       */

      auto it = m.begin();
      auto it_end = m.end();
      std::string key;
      key.reserve(20);

      // 0
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int0, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 3, "aa"));
      REQUIRE_EQ("aa", it.key());

      ++it;
      // 1
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int1, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 5, "aaaa"));
      REQUIRE_EQ("aaaa", it.key());

      ++it;
      // 2
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int2, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 8, "aaaaaaa"));
      REQUIRE_EQ("aaaaaaa", it.key());

      ++it;
      // 3
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int3, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 11, "aaaaaaaaaa"));
      REQUIRE_EQ("aaaaaaaaaa", it.key());

      ++it;
      // 4
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int4, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 10, "aaaaaaaba"));
      REQUIRE_EQ("aaaaaaaba", it.key());

      ++it;
      // 5
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int5, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 8, "aaaabaa"));
      REQUIRE_EQ("aaaabaa", it.key());

      ++it;
      // 6
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int6, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 11, "aaaabaaaaa"));
      REQUIRE_EQ("aaaabaaaaa", it.key());

      ++it;
      // 7 (overflow)
      REQUIRE(it == it_end);
    }

    SUBCASE("controlled test reverse") {
      int int0 = 0;
      int int1 = 1;
      int int2 = 2;
      int int3 = 4;
      int int4 = 5;
      int int5 = 5;
      int int6 = 6;

      art::art<int*> m;

      m.set(reinterpret_cast<const uint8_t *>("aa"), &int0);
      m.set(reinterpret_cast<const uint8_t *>("aaaa"), &int1);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaa"), &int2);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaaaa"), &int3);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaba"), &int4);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaa"), &int5);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaaaaa"), &int6);

      /* The above statements construct the following tree:
       *
       *          (aa)
       *   $_____/ |a
       *   /       |
       *  ()->0   (a)
       *   $_____/ |a\____________b
       *   /       |              \
       *  ()->1   (aa)            (aa)
       *   $_____/ |a\___b         |$\____a
       *   /       |     \         |      \
       *  ()->2 (aa$)->3 (a$)->4 ()->5 (aa$)->6
       *
       */

      auto it = m.begin();
      for (int i = 0; i < 6; i++)
          it++;
      auto it_begin = m.begin();
      std::string key;
      key.reserve(20);

      // 6
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int6, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 11, "aaaabaaaaa"));
      REQUIRE_EQ("aaaabaaaaa", it.key());

      --it;
      // 5
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int5, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 8, "aaaabaa"));
      REQUIRE_EQ("aaaabaa", it.key());

      --it;
      // 4
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int4, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 10, "aaaaaaaba"));
      REQUIRE_EQ("aaaaaaaba", it.key());

      --it;
      // 3
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int3, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 11, "aaaaaaaaaa"));
      REQUIRE_EQ("aaaaaaaaaa", it.key());

      --it;
      // 2
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int2, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 8, "aaaaaaa"));
      REQUIRE_EQ("aaaaaaa", it.key());


      --it;
      // 1
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int1, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 5, "aaaa"));
      REQUIRE_EQ("aaaa", it.key());

      --it;
      // 0
      REQUIRE(it == it_begin);
      REQUIRE_EQ(&int0, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 3, "aa"));
      REQUIRE_EQ("aa", it.key());
    }

    SUBCASE("tree_len") {
      int n = 0x10000;
      uint8_t key[5];
      int value;
      art::art<int*> m;
      for (int i = 0; i < n; ++i) {
        std::snprintf(reinterpret_cast<char *>(key), 5, "%04X", i);
        m.set(key, &value);
      }

      auto it = m.begin();
      auto it_end = m.end();
      int actual_n = 0;
      for (; it != it_end; ++it) {
          ++actual_n;
      }
      REQUIRE_EQ(n, actual_n);
    }
  }

  TEST_CASE("full lexicographic traversal with prefix keys") {
    SUBCASE("controlled test") {
      int int0 = 0;
      int int1 = 1;
      int int2 = 2;
      int int3 = 4;
      int int4 = 5;
      int int5 = 5;
      int int6 = 6;

      art::art<int*> m;

      m.set(reinterpret_cast<const uint8_t *>("aa"), 2, &int0);
      m.set(reinterpret_cast<const uint8_t *>("aaaa"), 4, &int1);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaa"), 7, &int2);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaaaa"), 10, &int3);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaba"), 9, &int4);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaa"), 7, &int5);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaaaaa"), 10, &int6);

      /* The above statements construct the following tree:
       *
       *    (aa)->0
       *     |a
       *     |
       *    (a)->0
       *     |a\____________b
       *     |              \
       *    (aa)->2         (aa)->5
       *     |a\___b         |a
       *     |     \         |
       *   (aa)->3 (a)->4   (aa)->6
       *
       */

      auto it = m.begin();
      auto it_end = m.end();
      std::string key;
      key.reserve(20);

      // 0
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int0, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 3, "aa"));
      REQUIRE_EQ("aa", it.key());

      ++it;
      // 1
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int1, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 5, "aaaa"));
      REQUIRE_EQ("aaaa", it.key());

      ++it;
      // 2
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int2, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 8, "aaaaaaa"));
      REQUIRE_EQ("aaaaaaa", it.key());

      ++it;
      // 3
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int3, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 11, "aaaaaaaaaa"));
      REQUIRE_EQ("aaaaaaaaaa", it.key());

      ++it;
      // 4
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int4, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 10, "aaaaaaaba"));
      REQUIRE_EQ("aaaaaaaba", it.key());

      ++it;
      // 5
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int5, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 8, "aaaabaa"));
      REQUIRE_EQ("aaaabaa", it.key());

      ++it;
      // 6
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int6, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 11, "aaaabaaaaa"));
      REQUIRE_EQ("aaaabaaaaa", it.key());

      ++it;
      // 7 (overflow)
      REQUIRE(it == it_end);
    }

    SUBCASE("controlled test reverse with prefix keys") {
      int int0 = 0;
      int int1 = 1;
      int int2 = 2;
      int int3 = 4;
      int int4 = 5;
      int int5 = 5;
      int int6 = 6;

      art::art<int*> m;

      m.set(reinterpret_cast<const uint8_t *>("aa"), 2, &int0);
      m.set(reinterpret_cast<const uint8_t *>("aaaa"), 4, &int1);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaa"), 7, &int2);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaaaa"), 10, &int3);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaba"), 9, &int4);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaa"), 7, &int5);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaaaaa"), 10, &int6);

      /* The above statements construct the following tree:
       *
       *    (aa)->0
       *     |a
       *     |
       *    (a)->0
       *     |a\____________b
       *     |              \
       *    (aa)->2         (aa)->5
       *     |a\___b         |a
       *     |     \         |
       *   (aa)->3 (a)->4   (aa)->6
       *
       */

      auto it = m.begin();
      for (int i = 0; i < 6; i++)
          it++;
      auto it_begin = m.begin();
      std::string key;
      key.reserve(20);

      // 6
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int6, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 11, "aaaabaaaaa"));
      REQUIRE_EQ("aaaabaaaaa", it.key());

      --it;
      // 5
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int5, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 8, "aaaabaa"));
      REQUIRE_EQ("aaaabaa", it.key());

      --it;
      // 4
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int4, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 10, "aaaaaaaba"));
      REQUIRE_EQ("aaaaaaaba", it.key());

      --it;
      // 3
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int3, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 11, "aaaaaaaaaa"));
      REQUIRE_EQ("aaaaaaaaaa", it.key());

      --it;
      // 2
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int2, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 8, "aaaaaaa"));
      REQUIRE_EQ("aaaaaaa", it.key());


      --it;
      // 1
      REQUIRE(it != it_begin);
      REQUIRE_EQ(&int1, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 5, "aaaa"));
      REQUIRE_EQ("aaaa", it.key());

      --it;
      // 0
      REQUIRE(it == it_begin);
      REQUIRE_EQ(&int0, *it);
      it.key(key.begin());
      REQUIRE(std::equal(key.begin(), key.begin() + 3, "aa"));
      REQUIRE_EQ("aa", it.key());
    }

    SUBCASE("tree_len with prefix keys") {
      int n = 0x10000;
      uint8_t key[5];
      int value;
      art::art<int*> m;
      for (int i = 0; i < n; ++i) {
        const uint32_t written_len = std::snprintf(reinterpret_cast<char *>(key), 5, "%04X", i);
        m.set(key, written_len, &value);
      }

      auto it = m.begin();
      auto it_end = m.end();
      int actual_n = 0;
      for (; it != it_end; ++it) {
          ++actual_n;
      }
      REQUIRE_EQ(n, actual_n);
    }
  }

  TEST_CASE("range lexicographic traversal") {
    SUBCASE("controlled test") {
      int int0 = 0;
      int int1 = 1;
      int int2 = 2;
      int int3 = 3;
      int int4 = 4;
      int int5 = 5;
      int int6 = 6;

      art::art<int*> m;

      m.set(reinterpret_cast<const uint8_t *>("aa"), &int0);
      m.set(reinterpret_cast<const uint8_t *>("aaaa"), &int1);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaa"), &int2);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaaaa"), &int3);
      m.set(reinterpret_cast<const uint8_t *>("aaaaaaaba"), &int4);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaa"), &int5);
      m.set(reinterpret_cast<const uint8_t *>("aaaabaaaaa"), &int6);

      /* The above statements construct the following tree:
       *
       *          (aa)
       *   $_____/ |a
       *   /       |
       *  ()->0   (a)
       *   $_____/ |a\____________b
       *   /       |              \
       *  ()->1   (aa)            (aa)
       *   $ ____/ |a\___b         |a\____$
       *    /      |     \         |      \
       *  ()->2 (aa$)->3 (a$)->4 ()->5 (aa$)->6
       *
       */

      // iterator on ["aaaaaaaaaa",3]
      auto it = m.begin(reinterpret_cast<const uint8_t *>("aaaaaaaaaa"));

      // iterator on ["aaaabaaaaa",6]
      auto it_end = m.begin(reinterpret_cast<const uint8_t *>("aaaabaaaaa"));

      // 3
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int3, *it);

      ++it;
      // 4
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int4, *it);

      ++it;
      // 5
      REQUIRE(it != it_end);
      REQUIRE_EQ(&int5, *it);

      ++it;
      // 6
      REQUIRE(it == it_end);
    }

    SUBCASE("monte carlo") {
      mt19937_64 rng(0);
      int n_bytes = 4;
      int n = 1 << (n_bytes * 4);
      uint8_t keys[n][n_bytes + 1];
      int value;
      art::art<int*> m;
      for (int i = 0; i < n; ++i) {
        std::snprintf(reinterpret_cast<char *>(keys[i]), n_bytes + 1, "%04X", i); // note: change format if you change n_bytes
        m.set(keys[i], &value);
      }
      for (int experiment = 0; experiment < 1000; ++experiment) {
        int start = rng() % n;
        int end;
        do {
          end = rng() % n;
        } while (end < start);
        auto it = m.begin(keys[start]);
        auto it_end = m.begin(keys[end]);
        int actual_n = 0;
        for (; it != it_end; ++it) {
            ++actual_n;
        }
        REQUIRE_EQ(end - start, actual_n);
      }
    }
  }
}

