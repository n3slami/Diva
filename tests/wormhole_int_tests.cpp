/**
 * @file wormhole int tests
 * @author ---
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

#include <cstdint>
#include <cstring>
#include <random>
#include <vector>
#include <x86intrin.h>
#include "wh_int.h"

constexpr size_t value_size = 12;

TEST_SUITE("wormhole_int") {
    TEST_CASE("put") {
        wormhole_int *wh = wh_int_create();
        wormref_int *better_tree = wh_int_ref(wh);
        const uint32_t total_puts = 10000;
        const uint32_t rng_seed = 1380;
        const bool check_it_write = false;
        std::mt19937_64 rng(rng_seed);

        uint8_t value[value_size];
        memset(value, 0, sizeof(value));

        std::vector<uint64_t> keys;

        SUBCASE("increasing keys") {
            for (int32_t i = 0; i < total_puts; i++)
                keys.push_back(550 * i);
        }
        SUBCASE("increasing random keys") {
            for (int32_t i = 0; i < total_puts; i++)
                keys.push_back(rng());
            std::sort(keys.begin(), keys.end());
        }
        SUBCASE("random keys") {
            for (int32_t i = 0; i < total_puts; i++)
                keys.push_back(rng());
        }

        for (uint64_t key : keys) {
            const uint64_t key_rev = _bswap64(key);
            memcpy(value + 3, &key, sizeof(key));
            wh_int_put(better_tree, &key_rev, sizeof(key_rev), value, value_size, false, false);
        }

        std::sort(keys.begin(), keys.end());

        const bool it_w = false;
        wormhole_int_iter *it = wh_int_iter_create(better_tree);
        int32_t ind = 0;
        for (wh_int_iter_seek(it, "", 0, it_w); wh_int_iter_valid(it); wh_int_iter_skip1(it, it_w, check_it_write)) {
            const uint8_t *fetched_key;
            uint8_t *fetched_value;
            uint32_t fetched_key_size, fetched_value_size;
            wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&fetched_key), &fetched_key_size,
                                     reinterpret_cast<void **>(&fetched_value), &fetched_value_size);
            const uint64_t recovered_key = _bswap64(*((uint64_t *) fetched_key));
            REQUIRE_EQ(recovered_key, keys[ind++]);
        }
        const uint64_t tmp = _bswap64(keys[keys.size() - 1]);
        for (wh_int_iter_seek(it, &tmp, sizeof(tmp), it_w); wh_int_iter_valid(it); wh_int_iter_skip1_rev(it, it_w, check_it_write)) {
            const uint8_t *fetched_key;
            uint8_t *fetched_value;
            uint32_t fetched_key_size, fetched_value_size;
            wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&fetched_key), &fetched_key_size,
                                     reinterpret_cast<void **>(&fetched_value), &fetched_value_size);
            const uint64_t recovered_key = _bswap64(*((uint64_t *) fetched_key));
            REQUIRE_EQ(recovered_key, keys[--ind]);
        }
        wh_int_iter_destroy(it, check_it_write);
    }
}


