/**
 * @file diva tests
 * @author ---
 */

#include "wormhole/wh.h"
#include "wormhole/wh_int.h"
#include <atomic>
#include <endian.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <string_view>
#include <thread>
#include <x86intrin.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "diva.hpp"
#include "util.hpp"

namespace diva {

class DivaTests {
public:
    template <DivaType diva_type>
    static void Insert() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const bool check_it_write = false;
        const bool check_it_unlock = true;
        Diva<diva_type> s(infix_size, seed, load_factor);

        uint64_t value = to_big_endian_order(0x0000000011111111UL);
        s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        value = to_big_endian_order(0x0000000022222222UL);
        s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        value = to_big_endian_order(0x0000000033333333UL);
        s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        value = to_big_endian_order(0x0000000044444444UL);
        s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        value = to_big_endian_order(0x0000000020000000UL);
        s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        value = to_big_endian_order(0x0000000040007777UL);
        s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));

        for (int32_t i = 1; i < 100; i++) {
            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * i + r * (100 - i)) / 100;
            value = to_big_endian_order(interp);
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        }
        SUBCASE("interpolated inserts") {
            const auto [occupieds_pos, checks] = 
                ReadStoreContentsFromFile("insert/interpolated");
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore *store;

            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);
                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);
                wh_iter_destroy(it, check_it_write);
            }
        }

        for (int32_t i = 90; i >= 70; i -= 2) {
            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * i + r * (100 - i)) / 100;
            value = to_big_endian_order(interp);
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        }
        SUBCASE("overlapping interpolated reversed inserts with a stride of 2") {
            const auto [occupieds_pos, checks] = 
                ReadStoreContentsFromFile("insert/overlapping_interpolated_stride_2");
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore *store;

            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);
                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);
                wh_iter_destroy(it, check_it_write);
            }
        }

        const uint32_t shamt = 16;
        for (int32_t i = 1; i < 50; i++) {
            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
            value = to_big_endian_order(interp);
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        }
        SUBCASE("overlapping interpolated consecutive inserts") {
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("insert/overlapping_interpolated_consecutive");
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore *store;

            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);
                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);
                wh_iter_destroy(it, check_it_write);
            }
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value = to_big_endian_order(value);
        s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)});

        SUBCASE("split infix store: left half") {
            const auto [occupieds_pos, checks] = 
                ReadStoreContentsFromFile("insert/split/left");
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore *store;

            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);
                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);
                wh_iter_destroy(it, check_it_write);
            }
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value &= ~BITMASK(shamt);
        value = to_big_endian_order(value);
        SUBCASE("split infix store: right half") {
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("insert/split/right");
            uint8_t res_key[sizeof(uint64_t)];
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore store;

            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8, check_it_write);
                wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size, 
                                 reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type>::InfixStore), &dummy);
                AssertStoreContents(s, store, occupieds_pos, checks);
                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8, check_it_write);
                wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size, 
                             reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type>::InfixStore), &dummy);
                AssertStoreContents(s, store, occupieds_pos, checks);
                wh_iter_destroy(it, check_it_write);
            }
        }

        // Fetch old boundary key
        uint8_t old_boundary [sizeof(uint64_t)];
        uint32_t old_boundary_size;
        if constexpr (diva_type == DivaType::Int) {
            wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
            wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8, check_it_write);
            uint32_t dummy;
            typename Diva<diva_type>::InfixStore store;
            wh_int_iter_peek(it, reinterpret_cast<void *>(old_boundary), sizeof(old_boundary), &old_boundary_size, 
                             reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type>::InfixStore), &dummy);
            wh_int_iter_destroy(it, check_it_write);
        }
        else {
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8, check_it_write);
            uint32_t dummy;
            typename Diva<diva_type>::InfixStore store;
            wh_iter_peek(it, reinterpret_cast<void *>(old_boundary), sizeof(old_boundary), &old_boundary_size, 
                         reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type>::InfixStore), &dummy);
            wh_iter_destroy(it, check_it_write);
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
        value = to_big_endian_order(value);
        s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)});
        SUBCASE("split infix store, create void infixes") {
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("insert/split/create_void_infixes");
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type >::InfixStore *store;

            uint64_t value = to_big_endian_order(0b0000000000000000000000000000000000011101000100110000000000000000UL);
            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - 2, check_it_write);
                wh_int_iter_skip1_rev(it, check_it_write, check_it_unlock);

                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                REQUIRE_EQ(old_boundary_size, res_size);
                REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
                wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                REQUIRE_EQ(sizeof(value), res_size);
                REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value) - 2), 0);

                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - 2, check_it_write);
                wh_iter_skip1_rev(it, check_it_write, check_it_unlock);

                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                REQUIRE_EQ(old_boundary_size, res_size);
                REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
                wh_iter_skip1(it, check_it_write, check_it_unlock);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                REQUIRE_EQ(sizeof(value), res_size);
                REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value) - 2), 0);

                wh_iter_destroy(it, check_it_write);
            }
        }

        if constexpr (diva_type == DivaType::Int)
            return;

        uint8_t new_extended_key[12];
        uint32_t new_extended_key_len;
        {
            uint64_t value = to_big_endian_order(0x0000000033333333UL);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value), check_it_write);
            typename Diva<diva_type>::InfixStore *store;
            uint32_t dummy;
            wh_iter_peek(it, reinterpret_cast<void *>(new_extended_key), sizeof(new_extended_key), &new_extended_key_len, 
                         reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type>::InfixStore), &dummy);
            wh_iter_destroy(it, check_it_write);
        }
        memset(new_extended_key + new_extended_key_len, 0, 3);
        new_extended_key_len += 3;
        new_extended_key[new_extended_key_len - 1] = 1;
        s.InsertSplit({new_extended_key, new_extended_key_len});

        SUBCASE("split infix store using an extension of a full boundary key: left half") {
            const std::vector<uint32_t> occupieds_pos = {};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore *store;

            const uint64_t value = to_big_endian_order(0x0000000033333333UL);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                             reinterpret_cast<void **>(&store), &dummy);
            AssertStoreContents(s, *store, occupieds_pos, checks);
            REQUIRE_EQ(sizeof(value), res_size);
            REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
            wh_iter_destroy(it, check_it_write);
        }

        SUBCASE("split infix store using an extension of a full boundary key: right half") {
            const std::vector<uint32_t> occupieds_pos = {410};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{ 48,1,0b00001}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore *store;

            const uint64_t value = to_big_endian_order(0x0000000033333333UL);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
            wh_iter_skip1(it, check_it_write, check_it_unlock);
            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                             reinterpret_cast<void **>(&store), &dummy);
            AssertStoreContents(s, *store, occupieds_pos, checks);
            REQUIRE_EQ(new_extended_key_len, res_size);
            REQUIRE_EQ(memcmp(new_extended_key, res_key, new_extended_key_len), 0);
            wh_iter_destroy(it, check_it_write);
        }
    }


    template <DivaType diva_type>
    static void RandomInsert() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;

        const uint32_t rng_seed = 10;
        std::mt19937_64 rng(rng_seed);
        const uint32_t init_n = 10000;
        std::vector<uint64_t> init_keys;
        for (int32_t i = 0; i < init_n; i++)
            init_keys.push_back(rng());
        std::sort(init_keys.begin(), init_keys.end());
        if constexpr (diva_type != DivaType::Int) {
            for (int32_t i = 0; i < init_n; i++)
                init_keys[i] = to_big_endian_order(init_keys[i]);
        }
        Diva<diva_type> s(infix_size, init_keys.begin(), init_keys.end(), sizeof(uint64_t),
                          seed, load_factor);

        const uint32_t extra_n = 1400000;
        for (int32_t i = 0; i < extra_n; i++) {
            const uint64_t key = to_big_endian_order(rng());
            s.Insert(reinterpret_cast<const uint8_t *>(&key), sizeof(key));
        }
    }


    template <DivaType diva_type>
    static void PointQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<diva_type> s(infix_size, seed, load_factor);

        std::set<uint64_t> keys = {std::numeric_limits<uint64_t>::min(),
                                   std::numeric_limits<uint64_t>::max()};
        for (int32_t i = 0; i < 10; i++)
            keys.insert((i + 1) * 0x0000000011111111UL);
        for (uint64_t key : keys) {
            const uint64_t conv_key = to_big_endian_order(key);
            s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key));
        }
        std::set<uint64_t> partial_keys;

        for (int32_t i = 1; i < 100; i++) {
            const uint32_t shared = 34;
            const uint32_t ignore = 1;
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * i + r * (100 - i)) / 100;
            const uint64_t value = to_big_endian_order(interp);
            s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
            partial_keys.insert((rev_value & (~BITMASK(bits_to_zero_out)) | (1ULL << bits_to_zero_out)));
        }

        for (int32_t i = 90; i >= 70; i -= 2) {
            const uint32_t shared = 34;
            const uint32_t ignore = 1;
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * i + r * (100 - i)) / 100;
            const uint64_t value = to_big_endian_order(interp);
            s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
            partial_keys.insert((rev_value & (~BITMASK(bits_to_zero_out)) | (1ULL << bits_to_zero_out)));
        }

        const uint32_t shamt = 16;
        for (int32_t i = 1; i < 50; i++) {
            const uint32_t shared = 34;
            const uint32_t ignore = 1;
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
            const uint64_t value = to_big_endian_order(interp);
            s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
            partial_keys.insert((rev_value & (~BITMASK(bits_to_zero_out)) | (1ULL << bits_to_zero_out)));
        }

        {
            uint64_t value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
            value = to_big_endian_order(value);
            s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            keys.insert(__bswap_64(value));

            value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
            value = to_big_endian_order(value);
            s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            keys.insert(__bswap_64(value));
        }

        SUBCASE("no false negatives") {
            for (uint64_t key : keys) {
                const uint64_t conv_key = to_big_endian_order(key);
                REQUIRE(s.PointQuery(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key)));
            }
        }

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);
        const uint32_t n_queries_per_partial_key = 100;
        SUBCASE("extensions of partial keys") {
            for (uint64_t partial_key : partial_keys) {
                const uint64_t l = partial_key - (partial_key & -partial_key);
                const uint64_t r = partial_key | (partial_key - 1);
                for (int32_t i = 0; i < n_queries_per_partial_key; i++) {
                    const uint64_t key = l + rng() % (r - l + 1);
                    const uint64_t conv_key = to_big_endian_order(key);
                    REQUIRE(s.PointQuery(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key)));
                }
            }
        }

        SUBCASE("negatives") {
            const uint32_t n_queries = 100000;
            std::set<uint64_t> queries;
            while (queries.size() < n_queries) {
                const uint64_t key = rng() % (5 * 0x0000000011111111UL);
                bool skip = false;
                if (keys.find(key) != keys.end())
                    skip = true;
                for (uint64_t partial_key : partial_keys) {
                    const uint64_t mask = ((partial_key & -partial_key) << 1) - 1;
                    if ((partial_key | mask) == (key | mask)) {
                        skip = true;
                        break;
                    }
                }
                if (!skip)
                    queries.insert(key);
            }
            for (uint64_t query : queries) {
                const uint64_t conv_query = to_big_endian_order(query);
                REQUIRE_FALSE(s.PointQuery(reinterpret_cast<const uint8_t *>(&conv_query), sizeof(conv_query)));
            }

            queries.clear();
            while (queries.size() < n_queries) {
                const uint64_t key = rng();
                bool skip = false;
                if (keys.find(key) != keys.end())
                    skip = true;
                for (uint64_t partial_key : partial_keys) {
                    const uint64_t mask = ((partial_key & -partial_key) << 1) - 1;
                    if ((partial_key | mask) == (key | mask)) {
                        skip = true;
                        break;
                    }
                }
                if (!skip)
                    queries.insert(key);
            }
            for (uint64_t query : queries) {
                const uint64_t conv_query = to_big_endian_order(query);
                REQUIRE_FALSE(s.PointQuery(reinterpret_cast<const uint8_t *>(&conv_query), sizeof(conv_query)));
            }
        }
    }


    template <DivaType diva_type>
    static void RangeQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<diva_type> s(infix_size, seed, load_factor);

        std::set<uint64_t> keys = {std::numeric_limits<uint64_t>::min(),
                                   std::numeric_limits<uint64_t>::max()};
        for (int32_t i = 0; i < 10; i++)
            keys.insert((i + 1) * 0x0000000011111111UL);
        for (uint64_t key : keys) {
            const uint64_t conv_key = to_big_endian_order(key);
            s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key));
        }
        std::set<uint64_t> partial_keys;

        for (int32_t i = 1; i < 100; i++) {
            const uint32_t shared = 34;
            const uint32_t ignore = 1;
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * i + r * (100 - i)) / 100;
            const uint64_t value = to_big_endian_order(interp);
            s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
            partial_keys.insert((rev_value & (~BITMASK(bits_to_zero_out)) | (1ULL << bits_to_zero_out)));
        }

        for (int32_t i = 90; i >= 70; i -= 2) {
            const uint32_t shared = 34;
            const uint32_t ignore = 1;
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * i + r * (100 - i)) / 100;
            const uint64_t value = to_big_endian_order(interp);
            s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
            partial_keys.insert((rev_value & (~BITMASK(bits_to_zero_out)) | (1ULL << bits_to_zero_out)));
        }

        const uint32_t shamt = 16;
        for (int32_t i = 1; i < 50; i++) {
            const uint32_t shared = 34;
            const uint32_t ignore = 1;
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
            const uint64_t value = to_big_endian_order(interp);
            s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
            partial_keys.insert((rev_value & (~BITMASK(bits_to_zero_out)) | (1ULL << bits_to_zero_out)));
        }

        {
            uint64_t value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
            value = to_big_endian_order(value);
            s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            keys.insert(__bswap_64(value));

            value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
            value = to_big_endian_order(value);
            s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            keys.insert(__bswap_64(value));
        }

        SUBCASE("no false negatives: random end-points") {
            const uint32_t rng_seed = 2;
            std::mt19937_64 rng(rng_seed);
            const uint32_t n_queries = 100000;
            std::set<std::pair<uint64_t, uint64_t>> queries;
            while (queries.size() < n_queries) {
                uint64_t l = rng() % (4 * 0x0000000011111111UL);
                uint64_t r = rng() % (4 * 0x0000000011111111UL);
                if (l > r)
                    std::swap(l, r);
                bool skip = *keys.lower_bound(l) > r;
                if (!skip)
                    queries.insert({l, r});
            }
            for (auto [l, r] : queries) {
                const uint64_t conv_l = to_big_endian_order(l);
                const uint64_t conv_r = to_big_endian_order(r);
                REQUIRE(s.RangeQuery(reinterpret_cast<const uint8_t *>(&conv_l), sizeof(conv_l),
                                     reinterpret_cast<const uint8_t *>(&conv_r), sizeof(conv_r)));
            }
        }

        SUBCASE("no false negatives: bounded-length ranges") {
            const uint32_t rng_seed = 3;
            std::mt19937_64 rng(rng_seed);
            const uint32_t n_queries = 100000;
            const uint64_t length_modulo = 1ULL << 17;
            std::set<std::pair<uint64_t, uint64_t>> queries;
            while (queries.size() < n_queries) {
                uint64_t l = rng() % (4 * 0x0000000011111111UL);
                uint64_t r = l + rng() % length_modulo;
                bool skip = *keys.lower_bound(l) > r;
                if (!skip)
                    queries.insert({l, r});
            }
            for (auto [l, r] : queries) {
                const uint64_t conv_l = to_big_endian_order(l);
                const uint64_t conv_r = to_big_endian_order(r);
                REQUIRE(s.RangeQuery(reinterpret_cast<const uint8_t *>(&conv_l), sizeof(conv_l),
                                     reinterpret_cast<const uint8_t *>(&conv_r), sizeof(conv_r)));
            }
        }

        SUBCASE("intersections with partial keys") {
            const uint32_t rng_seed = 4;
            std::mt19937_64 rng(rng_seed);
            const uint32_t n_queries = 100000;
            std::set<std::pair<uint64_t, uint64_t>> queries;
            while (queries.size() < n_queries) {
                uint64_t l = rng() % (4 * 0x0000000011111111UL);
                uint64_t next_key = *keys.lower_bound(l);
                uint64_t r = l + rng() % (next_key - l);
                bool skip = true;
                for (uint64_t partial_key : partial_keys) {
                    const uint64_t partial_l = partial_key - (partial_key & -partial_key);
                    const uint64_t partial_r = partial_key | (partial_key - 1);
                    if (std::max(l, partial_l) <= std::min(r, partial_r)) {
                        skip = false;
                        break;
                    }
                }
                if (!skip)
                    queries.insert({l, r});
            }
            for (auto [l, r] : queries) {
                const uint64_t conv_l = to_big_endian_order(l);
                const uint64_t conv_r = to_big_endian_order(r);
                REQUIRE(s.RangeQuery(reinterpret_cast<const uint8_t *>(&conv_l), sizeof(conv_l),
                                     reinterpret_cast<const uint8_t *>(&conv_r), sizeof(conv_r)));
            }
        }

        SUBCASE("negatives: close to keys from the left") {
            const uint32_t rng_seed = 5;
            std::mt19937_64 rng(rng_seed);
            const uint32_t n_queries = 100000;
            std::set<std::pair<uint64_t, uint64_t>> queries;
            while (queries.size() < n_queries) {
                uint64_t l = rng() % (4 * 0x0000000011111111UL);
                uint64_t next_key = *keys.lower_bound(l);
                uint64_t r = l + rng() % (next_key - l);
                bool skip = false;
                for (uint64_t partial_key : partial_keys) {
                    const uint64_t partial_l = partial_key - (partial_key & -partial_key);
                    const uint64_t partial_r = partial_key | (partial_key - 1);
                    if (std::max(l, partial_l) <= std::min(r, partial_r)) {
                        skip = true;
                        break;
                    }
                }
                if (!skip)
                    queries.insert({l, r});
            }
            for (auto [l, r] : queries) {
                const uint64_t conv_l = to_big_endian_order(l);
                const uint64_t conv_r = to_big_endian_order(r);
                REQUIRE_FALSE(s.RangeQuery(reinterpret_cast<const uint8_t *>(&conv_l), sizeof(conv_l),
                                           reinterpret_cast<const uint8_t *>(&conv_r), sizeof(conv_r)));
            }
        }

        SUBCASE("negatives: close to keys from the right") {
            const uint32_t rng_seed = 6;
            std::mt19937_64 rng(rng_seed);
            const uint32_t n_queries = 100000;
            std::set<std::pair<uint64_t, uint64_t>> queries;
            while (queries.size() < n_queries) {
                uint64_t r = rng() % (4 * 0x0000000011111111UL);
                auto it = keys.lower_bound(r);
                --it;
                uint64_t prev_key = *it;
                uint64_t l = r - rng() % (r - prev_key);
                bool skip = false;
                for (uint64_t partial_key : partial_keys) {
                    const uint64_t partial_l = partial_key - (partial_key & -partial_key);
                    const uint64_t partial_r = partial_key | (partial_key - 1);
                    if (std::max(l, partial_l) <= std::min(r, partial_r)) {
                        skip = true;
                        break;
                    }
                }
                if (!skip)
                    queries.insert({l, r});
            }
            for (auto [l, r] : queries) {
                const uint64_t conv_l = to_big_endian_order(l);
                const uint64_t conv_r = to_big_endian_order(r);
                REQUIRE_FALSE(s.RangeQuery(reinterpret_cast<const uint8_t *>(&conv_l), sizeof(conv_l),
                                           reinterpret_cast<const uint8_t *>(&conv_r), sizeof(conv_r)));
            }
        }
    }


    template <DivaType diva_type>
    static void Delete() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const bool check_it_write = false;
        const bool check_it_unlock = true;
        Diva<diva_type> s(infix_size, seed, load_factor);

        SUBCASE("merge") {
            SUBCASE("1") {
                std::vector<uint64_t> boundary_keys = 
                    {0b00001111'11111111'00000000'00000000UL,
                        0b00010000'00000000'11111111'11111111UL,
                        0b00010111'11111111'00000000'00000000UL,
                        0b11111111'11111111'11111111'11111111UL};
                for (uint64_t key : boundary_keys) {
                    const uint64_t conv_key = to_big_endian_order(key);
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key));
                }

                const uint8_t *key_ptr, *res_key;
                uint32_t key_len;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                typename Diva<diva_type>::InfixStore *store;
                uint32_t res_size, dummy;

                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'001010101'01011,
                    0b0'011011011'00100, 0b0'011011011'00001,
                    0b1'001010101'01000, 0b1'001010101'01011,
                    0b1'011011011'00001};
                std::vector<uint64_t> right_store_infixes {0b1'001010101'10000,
                    0b1'001010101'01011, 0b1'001111111'11001,
                    0b1'011111100'01000, 0b1'011111100'00111};
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("delete/merge/1");
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);
                    wh_int_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);
                    wh_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_iter_destroy(it, check_it_write);
                }
            }

            SUBCASE("2") {
                std::vector<uint64_t> boundary_keys = 
                    {0b00001111'11111111'00000000'00000000UL,
                        0b00010000'01111111'00001010'00000000UL,
                        0b00010000'10000000'11111111'11111111UL,
                        0b11111111'11111111'11111111'11111111UL};
                for (uint64_t key : boundary_keys) {
                    const uint64_t conv_key = to_big_endian_order(key);
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key));
                }

                const uint8_t *key_ptr, *res_key;
                uint32_t key_len;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                typename Diva<diva_type>::InfixStore *store;
                uint32_t res_size, dummy;

                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'000000000'10100,
                    0b0'000000000'10101, 0b0'000000001'10111,
                    0b0'000000010'10000, 0b0'000000010'00001,
                    0b0'000000011'10111, 0b1'000000000'00001};
                std::vector<uint64_t> right_store_infixes {0b0'010100000'11111,
                    0b0'011110101'01000, 0b0'011110101'00001,
                    0b1'001001011'01011};
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("delete/merge/2");
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);
                    wh_int_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);
                    wh_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_iter_destroy(it, check_it_write);
                }
            }

            SUBCASE("3") {
                std::vector<uint64_t> boundary_keys = 
                    {0b00000000'00000000'00000000'00000000UL,
                        0b00000000'00000000'11111111'11111111UL,
                        0b11111111'11111111'11111111'11111111UL};
                for (uint64_t key : boundary_keys) {
                    const uint64_t conv_key = to_big_endian_order(key);
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key));
                }

                const uint8_t *key_ptr, *res_key;
                uint32_t key_len;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                typename Diva<diva_type>::InfixStore *store;
                uint32_t res_size, dummy;

                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'000000000'10100,
                    0b0'000000000'10101, 0b0'000000001'10111,
                    0b0'000000010'10000, 0b0'000000010'00001,
                    0b0'000000011'10111, 0b1'000000000'00001};
                std::vector<uint64_t> right_store_infixes {0b0'010100000'11111,
                    0b0'011110101'01000, 0b0'011110101'00001,
                    0b1'001001011'01011};
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("delete/merge/3");
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);
                    wh_int_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);
                    wh_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_iter_destroy(it, check_it_write);
                }
            }
        }

        SUBCASE("delete all") {
            std::vector<std::pair<uint64_t, uint64_t>> keys;
            std::set<uint64_t> boundary_keys;
            for (int32_t i = 0; i < 10; i++)
                keys.emplace_back((i + 1) * 0x0000000011111111UL, 0);
            {
                const uint64_t min_key = std::numeric_limits<uint64_t>::min();
                s.AddTreeKey(reinterpret_cast<const uint8_t *>(&min_key), sizeof(min_key));
                boundary_keys.insert(min_key);
                const uint64_t max_key = std::numeric_limits<uint64_t>::max();
                s.AddTreeKey(reinterpret_cast<const uint8_t *>(&max_key), sizeof(max_key));
                boundary_keys.insert(max_key);
            }
            for (auto [key, bits_to_zero_out] : keys) {
                const uint64_t conv_key = to_big_endian_order(key);
                s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key));
                boundary_keys.insert(key);
            }

            for (int32_t i = 1; i < 100; i++) {
                const uint32_t shared = 34;
                const uint32_t ignore = 1;
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * i + r * (100 - i)) / 100;
                const uint64_t value = to_big_endian_order(interp);
                s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
                keys.emplace_back(interp, bits_to_zero_out);
            }

            for (int32_t i = 90; i >= 70; i -= 2) {
                const uint32_t shared = 34;
                const uint32_t ignore = 1;
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * i + r * (100 - i)) / 100;
                const uint64_t value = to_big_endian_order(interp);
                s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
                keys.emplace_back(interp, bits_to_zero_out);
            }

            const uint32_t shamt = 16;
            for (int32_t i = 1; i < 50; i++) {
                const uint32_t shared = 34;
                const uint32_t ignore = 1;
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
                const uint64_t value = to_big_endian_order(interp);
                s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
                keys.emplace_back(interp, bits_to_zero_out);
            }

            {
                uint64_t value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
                value = to_big_endian_order(value);
                s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
                boundary_keys.insert(__bswap_64(value));
            }

            for (int32_t i = 0; i < keys.size(); i++) {
                const uint64_t query = to_big_endian_order(keys[i].first);
                REQUIRE(s.PointQuery(reinterpret_cast<const uint8_t *>(&query), sizeof(query)));
            }

            const uint32_t shuffle_seed = 10;
            std::mt19937 shuffle_gen(shuffle_seed);
            std::shuffle(keys.begin(), keys.end(), shuffle_gen);
            // Since we duplicate this infix, we're going to return true anyways, so account for this
            keys.emplace_back(0b00011101000010110000000000000000UL, 16);

            for (int32_t i = 0; i < keys.size() - 1; i++) {
                if (boundary_keys.find(keys[i].first) != boundary_keys.end())
                    boundary_keys.erase(keys[i].first);

                const uint64_t del_value = to_big_endian_order(keys[i].first);
                s.Delete(reinterpret_cast<const uint8_t *>(&del_value), sizeof(del_value));

                for (int32_t j = 0; j < keys.size(); j++) {
                    const uint64_t query = to_big_endian_order(keys[j].first);
                    auto it = boundary_keys.lower_bound(keys[j].first);
                    bool expected_res = true;
                    if (*it != keys[j].first) {
                        const uint64_t next_key = to_big_endian_order(*it);
                        --it;
                        const uint64_t prev_key = to_big_endian_order(*it);
                        auto [shared, ignore, implicit_size] = s.GetSharedIgnoreImplicitLengths(
                                {reinterpret_cast<const uint8_t *>(&prev_key), sizeof(prev_key)},
                                {reinterpret_cast<const uint8_t *>(&next_key), sizeof(next_key)});
                        const uint32_t bits_to_zero_out = std::max(sizeof(uint64_t) * 8 - shared - ignore - implicit_size - s.infix_size_,
                                                                   keys[j].second);
                        const uint64_t l = keys[j].first & (~BITMASK(bits_to_zero_out + 1));
                        const uint64_t r = keys[j].first | BITMASK(bits_to_zero_out + 1);
                        expected_res = std::find_if(keys.begin() + i + 1, keys.end(),
                                                    [&](std::pair<uint64_t, uint64_t> key) { return *it < key.first
                                                                                                        && (l <= key.first && key.first <= r); })
                                                != keys.end();
                    }
                    REQUIRE_EQ(s.PointQuery(reinterpret_cast<const uint8_t *>(&query), sizeof(query)), expected_res);
                }
            }
        }

        SUBCASE("monte carlo") {
            const uint32_t n_keys = 2000000;
            const uint32_t rng_seed = 3;
            std::mt19937_64 rng(rng_seed);

            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++)
                keys.push_back(rng());
            std::sort(keys.begin(), keys.end());
            std::vector<std::string> string_keys;
            for (int32_t i = 0; i < n_keys; i++) {
                size_t str_length;
                if constexpr (diva_type == DivaType::Int)
                    str_length = 8;
                else
                    str_length = 6 + rng() % 3;
                const uint64_t value = to_big_endian_order(keys[i]);
                string_keys.emplace_back(reinterpret_cast<const char *>(&value), str_length);
            }
            std::shuffle(string_keys.begin(), string_keys.end(), rng);
            std::sort(string_keys.begin(), string_keys.begin() + n_keys / 32);
            Diva<diva_type> s(infix_size, string_keys.begin(), string_keys.begin() + n_keys / 32, seed, load_factor);

            std::vector<bool> deleted(n_keys, false);
            uint32_t i = n_keys / 32;
            while (i < n_keys) {
                if (rng() % 2 == 0) {
                    s.Insert(string_keys[i]);
                    REQUIRE(s.PointQuery(string_keys[i]));
                    i++;
                }
                else {
                    const uint32_t pos = rng() % i;
                    if (!deleted[pos]) {
                        s.Delete(string_keys[pos]);
                        deleted[pos] = true;
                    }
                }
            }
        }
    }


    template <DivaType diva_type>
    static void BulkLoad() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t n_keys = 2600;
        const bool check_it_write = false;
        const bool check_it_unlock = true;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);
        SUBCASE("fixed length") {
            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++)
                keys.push_back(rng());
            std::sort(keys.begin(), keys.end());
            if constexpr (diva_type != DivaType::Int) {
                for (int32_t i = 0; i < n_keys; i++)
                    keys[i] = to_big_endian_order(keys[i]);
            }

            Diva<diva_type> s(infix_size, keys.begin(), keys.end(), sizeof(uint64_t), seed, load_factor);

            // Setup the expected boundary keys and the Infix Stores we should check
            const uint32_t num_boundaries = 6;
            const uint8_t expected_boundaries[num_boundaries][8] = {{0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000},
                                                                    {0b00000000, 0b00010000, 0b00001100, 0b00111101, 0b11110111, 0b11101011, 0b00011100, 0b10100010},
                                                                    {0b01100010, 0b00101001, 0b11111000, 0b10110111, 0b10010000, 0b00101011, 0b01101111, 0b10111011},
                                                                    {0b11000111, 0b10011111, 0b10110001, 0b01100101, 0b10001101, 0b10001110, 0b10111111, 0b01000111},
                                                                    {0b11111111, 0b11000110, 0b10010011, 0b00101100, 0b00100001, 0b01010100, 0b01111011, 0b11000010},
                                                                    {0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111}};
            std::vector<uint32_t> occupieds_pos_vec[6];
            std::vector<std::tuple<uint32_t, bool, uint64_t>> checks_vec[6];
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("bulk_load/fixed_length/2");
                occupieds_pos_vec[1] = occupieds_pos;
                checks_vec[1] = checks;
            }
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("bulk_load/fixed_length/3");
                occupieds_pos_vec[2] = occupieds_pos;
                checks_vec[2] = checks;
            }
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("bulk_load/fixed_length/4");
                occupieds_pos_vec[3] = occupieds_pos;
                checks_vec[3] = checks;
            }

            // Check the boundary keys and the Infix Stores
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore *store;
            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, nullptr, 0, check_it_write);
                for (int32_t i = 0; i < num_boundaries; i++) {
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_EQ(memcmp(expected_boundaries[i], res_key, sizeof(keys[0])), 0);
                    AssertStoreContents(s, *store, occupieds_pos_vec[i], checks_vec[i]);
                    wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                }
                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, nullptr, 0, check_it_write);
                for (int32_t i = 0; i < num_boundaries; i++) {
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_EQ(memcmp(expected_boundaries[i], res_key, sizeof(keys[0])), 0);
                    AssertStoreContents(s, *store, occupieds_pos_vec[i], checks_vec[i]);
                    wh_iter_skip1(it, check_it_write, check_it_unlock);
                }
                wh_iter_destroy(it, check_it_write);
            }
        }

        SUBCASE("variable length") {
            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++)
                keys.push_back(rng());
            std::sort(keys.begin(), keys.end());
            std::vector<std::string> string_keys;
            for (int32_t i = 0; i < n_keys; i++) {
                const size_t str_length = 6 + rng() % 3;
                const uint64_t value = to_big_endian_order(keys[i]);
                string_keys.emplace_back(reinterpret_cast<const char *>(&value), str_length);
            }

            Diva<diva_type> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor);

            // Setup the expected boundary keys and the Infix Stores we should check
            const uint32_t num_boundaries = 6;
            const uint32_t expected_boundary_lengths[num_boundaries] = {8, 6, 6, 6, 8, 8};
            const uint8_t expected_boundaries[num_boundaries][8] = {{0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000},
                                                                    {0b00000000, 0b00010000, 0b00001100, 0b00111101, 0b11110111, 0b11101011},
                                                                    {0b01100010, 0b00101001, 0b11111000, 0b10110111, 0b10010000, 0b00101011},
                                                                    {0b11000111, 0b10011111, 0b10110001, 0b01100101, 0b10001101, 0b10001110},
                                                                    {0b11111111, 0b11000110, 0b10010011, 0b00101100, 0b00100001, 0b01010100, 0b01111011, 0b11000010},
                                                                    {0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111}};
            std::vector<uint32_t> occupieds_pos_vec[6];
            std::vector<std::tuple<uint32_t, bool, uint64_t>> checks_vec[6];
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("bulk_load/variable_length/2");
                occupieds_pos_vec[1] = occupieds_pos;
                checks_vec[1] = checks;
            }
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("bulk_load/variable_length/3");
                occupieds_pos_vec[2] = occupieds_pos;
                checks_vec[2] = checks;
            }
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("bulk_load/variable_length/4");
                occupieds_pos_vec[3] = occupieds_pos;
                checks_vec[3] = checks;
            }


            // Check the boundary keys and the Infix Stores
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<diva_type>::InfixStore *store;
            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, nullptr, 0, check_it_write);
                for (int32_t i = 0; i < num_boundaries; i++) {
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_EQ(memcmp(expected_boundaries[i], res_key, expected_boundary_lengths[i]), 0);
                    AssertStoreContents(s, *store, occupieds_pos_vec[i], checks_vec[i]);
                    wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                }
                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, nullptr, 0, check_it_write);
                for (int32_t i = 0; i < num_boundaries; i++) {
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_EQ(memcmp(expected_boundaries[i], res_key, expected_boundary_lengths[i]), 0);
                    AssertStoreContents(s, *store, occupieds_pos_vec[i], checks_vec[i]);
                    wh_iter_skip1(it, check_it_write, check_it_unlock);
                }
                wh_iter_destroy(it, check_it_write);
            }
        }
    }


    template <DivaType diva_type>
    static void SerializeDeserialize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t n_keys = 1000000;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        std::vector<uint64_t> keys;
        for (int32_t i = 0; i < n_keys; i++)
            keys.push_back(rng());
        std::sort(keys.begin(), keys.end());
        std::vector<std::string> string_keys;
        for (int32_t i = 0; i < n_keys; i++) {
            size_t str_length;
            if constexpr (diva_type == DivaType::Int)
                str_length = 8;
            else
                str_length = 6 + rng() % 3;
            const uint64_t value = to_big_endian_order(keys[i]);
            string_keys.emplace_back(reinterpret_cast<const char *>(&value), str_length);
        }

        SUBCASE("bulk loaded") {
            Diva<diva_type> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor);

            const uint32_t buf_size = s.Size() + 20;
            char *buf = new char[buf_size];
            const uint32_t serialized_size = s.Serialize(buf);
            REQUIRE_EQ(s.Size(), serialized_size);

            Diva<diva_type> reconstructed_s(buf);
            AssertDivas(s, reconstructed_s);

            delete[] buf;
        }

        Diva<diva_type> s(infix_size, seed, load_factor, 0, true);
        const uint32_t sample_gap = n_keys / 20;
        for (uint32_t i = 0; i < n_keys; i += sample_gap)
            s.AddTreeKey(reinterpret_cast<const uint8_t *>(string_keys[i].data()), string_keys[i].size());

        SUBCASE("samples only") {
            const uint32_t buf_size = s.Size() + 20;
            char *buf = new char[buf_size];
            const uint32_t serialized_size = s.Serialize(buf);
            REQUIRE_EQ(s.Size(), serialized_size);

            Diva<diva_type> reconstructed_s(buf);
            AssertDivas(s, reconstructed_s);

            delete[] buf;
        }

        uint32_t *perm = new uint32_t[n_keys];
        for (uint32_t i = 0; i < n_keys; i++)
            perm[i] = i;
        std::shuffle(perm, perm + n_keys, rng);
        for (uint32_t i = 0; i < n_keys; i++) {
            if (perm[i] % sample_gap == 0)
                continue;
            s.Insert(string_keys[perm[i]]);
        }
        delete[] perm;

        SUBCASE("incremental insertions") {
            const uint32_t buf_size = s.Size() + 20;
            char *buf = new char[buf_size];
            const uint32_t serialized_size = s.Serialize(buf);
            REQUIRE_EQ(s.Size(), serialized_size);

            Diva<diva_type> reconstructed_s(buf);
            AssertDivas(s, reconstructed_s);

            delete[] buf;
        }
    }


    template <DivaType diva_type>
    static void BulkLoadStreaming() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t n_keys = 40000;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        std::vector<uint64_t> keys;
        for (int32_t i = 0; i < n_keys; i++)
            keys.push_back(rng());
        std::sort(keys.begin(), keys.end());
        std::vector<std::string> string_keys;
        for (int32_t i = 0; i < n_keys; i++) {
            size_t str_length;
            if constexpr (diva_type == DivaType::Int)
                str_length = 8;
            else
                str_length = 6 + rng() % 3;
            const uint64_t value = to_big_endian_order(keys[i]);
            string_keys.emplace_back(reinterpret_cast<const char *>(&value), str_length);
        }

        SUBCASE("streaming") {
            Diva<diva_type> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor);
            Diva<diva_type> streamed_s(infix_size, seed, load_factor);
            for (int32_t i = 0; i < n_keys; i++)
                streamed_s.BulkLoadStreaming(string_keys[i]);
            streamed_s.BulkLoadStreamingFinish();
            AssertDivas(s, streamed_s);
        }
    }


    template <DivaType diva_type>
    static void Concurrency() {
        const uint32_t infix_size = 10;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t n_keys = 20000000;
        const uint32_t n_threads = 16;
        const uint32_t n_bulk = n_keys / n_threads;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        std::vector<uint64_t> keys;
        for (int32_t i = 0; i < n_keys; i++)
            keys.push_back(rng());
        std::sort(keys.begin(), keys.end());
        std::vector<std::string> string_keys;
        for (int32_t i = 0; i < n_keys; i++) {
            size_t str_length;
            if constexpr (diva_type == DivaType::Int)
                str_length = 8;
            else
                str_length = 6 + rng() % 3;
            const uint64_t value = to_big_endian_order(keys[i]);
            string_keys.emplace_back(reinterpret_cast<const char *>(&value), str_length);
        }
        std::shuffle(string_keys.begin(), string_keys.end(), rng);
        std::sort(string_keys.begin(), string_keys.begin() + n_bulk);
        Diva<diva_type> s(infix_size, string_keys.begin(), string_keys.begin() + n_bulk, seed, load_factor);
        for (uint32_t i = 0; i < n_bulk; i++)
            REQUIRE_EQ(s.PointQuery(string_keys[i]), true);

        std::vector<std::thread> threads;

        SUBCASE("inserts") {
            for (uint32_t i = 0; i < n_threads; i++) {
                threads.emplace_back([&, i] {
                            std::mt19937_64 rng(rng_seed + i + 1);
                            for (uint32_t j = n_bulk + i; j < n_keys; j += n_threads) {
                                s.Insert(string_keys[j], nullptr, rng());
                                REQUIRE(s.PointQuery(string_keys[j]));
                            }
                        });
            }
            for (auto& t : threads)
                t.join();
        }

        SUBCASE("inserts and deletes") {
            std::vector<std::atomic<bool>> deleted(string_keys.size());
            for (uint32_t i = 0; i < string_keys.size(); i++)
                deleted[i].store(false, std::memory_order_release);
            for (uint32_t i = 0; i < n_threads; i++) {
                threads.emplace_back([&, i] {
                            std::mt19937_64 rng(rng_seed + i + 1);
                            uint32_t ti = n_bulk + i;
                            while (ti < n_keys) {
                                const bool insert = (rng() % 2) > 0;
                                if (insert) {
                                    s.Insert(string_keys[ti], nullptr, rng());
                                    REQUIRE(s.PointQuery(string_keys[ti]));
                                    ti += n_threads;
                                }
                                else {
                                    const uint32_t offset = rng() % ((ti - n_threads) / n_threads) + 1;
                                    const uint32_t pos = ti - offset * n_threads;
                                    assert(pos < ti && (ti - pos) % n_threads == 0);
                                    if (!deleted[pos].load(std::memory_order_acquire)) {
                                        s.Delete(string_keys[pos]);
                                        deleted[pos].store(true, std::memory_order_release);
                                    }
                                }
                            }
                        });
            }
            for (auto& t : threads)
                t.join();
        }
    }


    template <DivaType diva_type>
    static void Payloads() {
        const uint32_t infix_size = 5;
        const uint32_t payload_size = 100;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t n_threads = 32;
        const uint32_t n_keys = 10 * 1024;
        const uint32_t infix_store_target_size = Diva<diva_type, PayloadType::FixedLength>::infix_store_target_size;
        const bool check_it_write = false;
        const bool check_it_unlock = true;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        std::vector<uint64_t> keys;
        for (int32_t i = 0; i < n_keys; i++)
            keys.push_back(rng());
        std::sort(keys.begin(), keys.end());
        std::vector<std::string> fixed_length_string_keys, string_keys;
        for (int32_t i = 0; i < n_keys; i++) {
            size_t str_length;
            const size_t length_offset = rng() % 3;
            if constexpr (diva_type == DivaType::Int)
                str_length = 8;
            else
                str_length = 6 + length_offset;
            const uint64_t value = to_big_endian_order(keys[i]);
            fixed_length_string_keys.emplace_back(reinterpret_cast<const char *>(&value), 8);
            string_keys.emplace_back(reinterpret_cast<const char *>(&value), str_length);
        }
        std::sort(string_keys.begin(), string_keys.end());
        uint64_t payloads_contents[n_keys][payload_size / 64 + 2];
        uint64_t *payloads[n_keys];
        for (uint32_t i = 0; i < n_keys; i++) {
            for (uint32_t j = 0; j < payload_size / 64 + 2; j++)
                payloads_contents[i][j] = rng();
            payloads[i] = &(payloads_contents[i][0]);
        }
        std::sort(keys.begin(), keys.end());
        fixed_length_string_keys.clear();
        for (uint32_t i = 0; i < keys.size(); i++) {
            const uint64_t value = to_big_endian_order(keys[i]);
            if constexpr (diva_type != DivaType::Int)
                keys[i] = value;
            fixed_length_string_keys.emplace_back(reinterpret_cast<const char *>(&value), 8);
        }


        SUBCASE("bulk load") {
            SUBCASE("normal") {
                Diva<diva_type, PayloadType::FixedLength> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor,
                                                    payload_size, reinterpret_cast<const uint64_t **>(&payloads));
                CheckDivaPayloads(s, string_keys, reinterpret_cast<const uint64_t **>(&payloads));
            }

            SUBCASE("fixed length") {
                Diva<diva_type, PayloadType::FixedLength> s(infix_size, keys.begin(), keys.end(), 8, seed, load_factor,
                                                    payload_size, reinterpret_cast<const uint64_t **>(&payloads));
                CheckDivaPayloads(s, fixed_length_string_keys, reinterpret_cast<const uint64_t **>(&payloads));
            }

            SUBCASE("streaming") {
                Diva<diva_type, PayloadType::FixedLength> s(infix_size, seed, load_factor, payload_size);
                for (uint32_t i = 0; i < n_keys; i++) {
                    if constexpr (diva_type == DivaType::Int)
                        s.BulkLoadStreaming(keys[i], payloads[i]);
                    else
                        s.BulkLoadStreaming(string_keys[i], payloads[i]);
                }
                s.BulkLoadStreamingFinish();
                if constexpr (diva_type == DivaType::Int)
                    CheckDivaPayloads(s, fixed_length_string_keys, reinterpret_cast<const uint64_t **>(&payloads));
                else 
                    CheckDivaPayloads(s, string_keys, reinterpret_cast<const uint64_t **>(&payloads));
            }
        }

        SUBCASE("empty filter") {
            Diva<diva_type, PayloadType::FixedLength> s(infix_size, seed, load_factor, payload_size, true);

            uint32_t perm[string_keys.size()];
            for (uint32_t i = 0; i < string_keys.size(); i++)
                perm[i] = i;
            std::shuffle(perm, perm + string_keys.size(), rng);
            
            for (auto it = s.GetIterator(string_keys[0]); it.IsValid(); it++) {
                auto [key, key_len_bits] = *it;
                if constexpr (diva_type == DivaType::Int)
                    REQUIRE_EQ(key, std::numeric_limits<typeof(key)>::max());
                else
                    REQUIRE_EQ(key[0], 0xFF);
                REQUIRE_EQ(key_len_bits, 800);
            }

            if constexpr (diva_type == DivaType::Int) {
                for (uint32_t i = 0; i < string_keys.size(); i++) {
                    const uint32_t ind = perm[i];
                    bool found = false;
                    for (auto it = s.GetIterator(fixed_length_string_keys[ind]); it.IsValid(); it++) {
                        uint64_t it_payload[payload_size / 64 + 2];
                        it.GetPayload(it_payload);
                        auto [val, val_len_bits] = *it;
                        val = __builtin_bswap64(val);
                        char *str = reinterpret_cast<char *>(&val);
                        if (memcmp(str, fixed_length_string_keys[ind].c_str(), val_len_bits / 8) > 0)
                            break;
                        if (val_len_bits % 8 > 0 
                                && ((str[val_len_bits / 8] >> (8 - val_len_bits % 8)) != (fixed_length_string_keys[ind][val_len_bits / 8] >> (8 - val_len_bits % 8))))
                            continue;
                        found |= compare_bitmap_to_bitmap(payloads_contents[ind], 0, it_payload, 0, payload_size);
                    }
                    REQUIRE(!found);

                    s.Insert(fixed_length_string_keys[ind], payloads_contents[ind]);
                    found = false;
                    for (auto it = s.GetIterator(fixed_length_string_keys[ind]); it.IsValid(); it++) {
                        uint64_t it_payload[payload_size / 64 + 2];
                        it.GetPayload(it_payload);
                        auto [val, val_len_bits] = *it;
                        val = __builtin_bswap64(val);
                        char *str = reinterpret_cast<char *>(&val);
                        if (memcmp(str, fixed_length_string_keys[ind].c_str(), val_len_bits / 8) > 0)
                            break;
                        if (val_len_bits % 8 > 0 
                                && ((str[val_len_bits / 8] >> (8 - val_len_bits % 8)) != (fixed_length_string_keys[ind][val_len_bits / 8] >> (8 - val_len_bits % 8))))
                            continue;
                        found |= compare_bitmap_to_bitmap(payloads_contents[ind], 0, it_payload, 0, payload_size);
                    }
                    REQUIRE(found);
                }
            }
            else {
                for (uint32_t i = 0; i < string_keys.size(); i++) {
                    const uint32_t ind = perm[i];
                    bool found = false;
                    for (auto it = s.GetIterator(fixed_length_string_keys[ind]); it.IsValid(); it++) {
                        uint64_t it_payload[payload_size / 64 + 2];
                        it.GetPayload(it_payload);
                        auto [str, str_len_bits] = *it;
                        if (memcmp(str.c_str(), string_keys[ind].c_str(), str_len_bits / 8) > 0)
                            break;
                        if (str_len_bits % 8 > 0 
                                && ((str[str_len_bits / 8] >> (8 - str_len_bits % 8)) != (string_keys[ind][str_len_bits / 8] >> (8 - str_len_bits % 8))))
                            continue;
                        found |= compare_bitmap_to_bitmap(payloads_contents[ind], 0, it_payload, 0, payload_size);
                    }
                    REQUIRE(!found);

                    s.Insert(string_keys[ind], payloads_contents[ind]);
                    found = false;
                    for (auto it = s.GetIterator(string_keys[ind]); it.IsValid(); it++) {
                        uint64_t it_payload[payload_size / 64 + 2];
                        it.GetPayload(it_payload);
                        auto [str, str_len_bits] = *it;
                        if (memcmp(str.c_str(), string_keys[ind].c_str(), str_len_bits / 8) > 0)
                            break;
                        if (str_len_bits % 8 > 0 
                                && ((str[str_len_bits / 8] >> (8 - str_len_bits % 8)) != (string_keys[ind][str_len_bits / 8] >> (8 - str_len_bits % 8))))
                            continue;
                        found |= compare_bitmap_to_bitmap(payloads_contents[ind], 0, it_payload, 0, payload_size);
                    }
                    REQUIRE(found);
                }
            }
        }

        Diva<diva_type, PayloadType::FixedLength> s(infix_size, seed, load_factor, payload_size);

        SUBCASE("insert") {
            uint64_t payload[payload_size / 64 + 2];
            uint64_t value;
            uint8_t buf[9];
            memset(buf, 0, sizeof(buf));

            value = to_big_endian_order(0x0000000011111111UL);
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 1;
            s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);

            value = to_big_endian_order(0x0000000022222222UL);
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 2;
            s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);

            value = to_big_endian_order(0x0000000033333333UL);
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 3;
            s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);

            value = to_big_endian_order(0x0000000044444444UL);
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 4;
            s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);

            value = to_big_endian_order(0x0000000020000000UL);
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 5;
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);

            value = to_big_endian_order(0x0000000040007777UL);
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 6;
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);


            for (int32_t i = 1; i < 100; i++) {
                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * i + r * (100 - i)) / 100;
                value = to_big_endian_order(interp);
                for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                    payload[i] = rng();
                s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);
            }
            SUBCASE("interpolated inserts") {
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("payloads/insert/interpolated");
                ReadStorePayloadsFromFile("payloads/insert/interpolated", payload_size, check_payloads);

                uint8_t res_key[sizeof(uint64_t)];
                uint32_t res_size, dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                    wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                         reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 1);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                    wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                     reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 1);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                    wh_iter_destroy(it, check_it_write);
                }
            }

            for (int32_t i = 90; i >= 70; i -= 2) {
                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * i + r * (100 - i)) / 100;
                value = to_big_endian_order(interp);
                for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                    payload[i] = rng();
                s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);
            }
            SUBCASE("overlapping interpolated reversed inserts with a stride of 2") {
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/insert/overlapping_interpolated_stride_2");
                ReadStorePayloadsFromFile("payloads/insert/overlapping_interpolated_stride_2", payload_size, check_payloads);

                uint8_t res_key[sizeof(uint64_t)];
                uint32_t res_size, dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                    wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                         reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 1);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                    wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                     reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 1);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                    wh_iter_destroy(it, check_it_write);
                }
            }

            const uint32_t shamt = 16;
            for (int32_t i = 1; i < 50; i++) {
                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
                value = to_big_endian_order(interp);
                for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                    payload[i] = rng();
                s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value), payload);
            }
            SUBCASE("overlapping interpolated consecutive inserts") {
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/insert/overlapping_interpolated_consecutive");
                ReadStorePayloadsFromFile("payloads/insert/overlapping_interpolated_consecutive", payload_size, check_payloads);

                uint8_t res_key[sizeof(uint64_t)];
                uint32_t res_size, dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                    wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                         reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 1);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                    wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                     reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 1);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                    wh_iter_destroy(it, check_it_write);
                }
            }

            value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
            value = to_big_endian_order(value);
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 7;
            s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)}, payload);

            SUBCASE("split infix store: left half") {
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/insert/split/left");
                ReadStorePayloadsFromFile("payloads/insert/split/left", payload_size, check_payloads);

                uint8_t res_key[sizeof(uint64_t)];
                uint32_t res_size, dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                    wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                         reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 1);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                    wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                     reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 1);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                    wh_iter_destroy(it, check_it_write);
                }
            }

            value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
            value &= ~BITMASK(shamt);
            value = to_big_endian_order(value);
            SUBCASE("split infix store: right half") {
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/insert/split/right");
                ReadStorePayloadsFromFile("payloads/insert/split/right", payload_size, check_payloads);

                uint8_t res_key[sizeof(uint64_t)];
                uint32_t res_size, dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);

                    wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8, check_it_write);

                    wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size, 
                                     reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type, PayloadType::FixedLength>::InfixStore), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 0x7);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 0x7);

                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8, check_it_write);

                    wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size, 
                                 reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 0x7);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 0x7);

                    wh_iter_destroy(it, check_it_write);
                }
            }

            // Fetch old boundary key
            uint8_t old_boundary [sizeof(uint64_t)];
            uint32_t old_boundary_size;
            if constexpr (diva_type == DivaType::Int) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8, check_it_write);
                uint32_t dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;
                wh_int_iter_peek(it, reinterpret_cast<void *>(old_boundary), sizeof(old_boundary), &old_boundary_size, 
                                 reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type, PayloadType::FixedLength>::InfixStore), &dummy);
                wh_int_iter_destroy(it, check_it_write);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8, check_it_write);
                uint32_t dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;
                wh_iter_peek(it, reinterpret_cast<void *>(old_boundary), sizeof(old_boundary), &old_boundary_size, 
                             reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type, PayloadType::FixedLength>::InfixStore), &dummy);
                wh_iter_destroy(it, check_it_write);
            }

            value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
            value = to_big_endian_order(value);
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 9;
            s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)}, payload);
            SUBCASE("split infix store, create void infixes") {
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/insert/split/create_void_infixes");
                ReadStorePayloadsFromFile("payloads/insert/split/create_void_infixes", payload_size, check_payloads);

                uint8_t res_key[sizeof(uint64_t)];
                uint32_t res_size, dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;
                uint64_t value = to_big_endian_order(0b0000000000000000000000000000000000011101000100110000000000000000UL);
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - 2, check_it_write);
                    wh_int_iter_skip1_rev(it, check_it_write, check_it_unlock);

                    wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                         reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 0x7);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 0x7);

                    REQUIRE_EQ(old_boundary_size, res_size);
                    REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
                    wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                         reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    REQUIRE_EQ(sizeof(value), res_size);
                    REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value) - 2), 0);

                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - 2, check_it_write);
                    wh_iter_skip1_rev(it, check_it_write, check_it_unlock);

                    wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                     reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                    s.GetSamplePayload(store, 0, payload);
                    REQUIRE_EQ(payload[0], 0x7);
                    REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 0x7);

                    REQUIRE_EQ(old_boundary_size, res_size);
                    REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
                    wh_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                     reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                    REQUIRE_EQ(sizeof(value), res_size);
                    REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value) - 2), 0);

                    wh_iter_destroy(it, check_it_write);
                }
            }

            if constexpr (diva_type == DivaType::Int)
                return;

            uint8_t new_extended_key[12];
            uint32_t new_extended_key_len;
            {
                uint64_t value = to_big_endian_order(0x0000000033333333UL);
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value), check_it_write);
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore *store;
                uint32_t dummy;
                wh_iter_peek(it, reinterpret_cast<void *>(new_extended_key), sizeof(new_extended_key), &new_extended_key_len, 
                             reinterpret_cast<void *>(&store), sizeof(typename Diva<diva_type, PayloadType::FixedLength>::InfixStore), &dummy);
                wh_iter_destroy(it, check_it_write);
            }
            memset(new_extended_key + new_extended_key_len, 0, 3);
            new_extended_key_len += 3;
            new_extended_key[new_extended_key_len - 1] = 1;
            for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                payload[i] = 10;
            s.InsertSplit({new_extended_key, new_extended_key_len}, payload);
            SUBCASE("split infix store using an extension of a full boundary key: left half") {
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
                const uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                const uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);

                uint8_t res_key[12];
                uint32_t res_size, dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;

                const uint64_t value = to_big_endian_order(0x0000000033333333UL);
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);

                wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                 reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                s.GetSamplePayload(store, 0, payload);
                REQUIRE_EQ(payload[0], 3);
                REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 3);

                REQUIRE_EQ(sizeof(value), res_size);
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);

                wh_iter_destroy(it, check_it_write);
            }

            SUBCASE("split infix store using an extension of a full boundary key: right half") {
                const std::vector<uint32_t> occupieds_pos = {410};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{ 48,1,0b00001}};
                const uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {{0x6, 0x6, }};
                const uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);

                uint8_t res_key[12];
                uint32_t res_size, dummy;
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore store;

                const uint64_t value = to_big_endian_order(0x0000000033333333UL);
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                wh_iter_skip1(it, check_it_write, check_it_unlock);

                wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size,
                                 reinterpret_cast<void *>(&store), sizeof(store), &dummy);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
                s.GetSamplePayload(store, 0, payload);
                REQUIRE_EQ(payload[0], 10);
                REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 10);

                REQUIRE_EQ(new_extended_key_len, res_size);
                REQUIRE_EQ(memcmp(new_extended_key, res_key, new_extended_key_len), 0);

                wh_iter_destroy(it, check_it_write);
            }
        }

        SUBCASE("delete") {
            uint64_t payload[payload_size / 64 + 2] = {};
            SUBCASE("1") {
                std::vector<uint64_t> boundary_keys = 
                    {0b00001111'11111111'00000000'00000000UL,
                        0b00010000'00000000'11111111'11111111UL,
                        0b00010111'11111111'00000000'00000000UL,
                        0b11111111'11111111'11111111'11111111UL};
                for (uint64_t key : boundary_keys) {
                    const uint64_t conv_key = to_big_endian_order(key);
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        payload[i]++;
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key), payload);
                }

                const uint8_t *key_ptr, *res_key;
                uint32_t key_len, res_size;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore *store;
                uint32_t dummy;

                // Setup check data
                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/merge/1");
                std::vector<uint64_t> left_store_infixes {0b0'001010101'01011,
                    0b0'011011011'00100, 0b0'011011011'00001,
                    0b1'001010101'01000, 0b1'001010101'01011,
                    0b1'011011011'00001};
                uint64_t left_store_payloads[left_store_infixes.size() * ((payload_size + 63) / 64) + 1];
                for (uint32_t i = 0; i < left_store_infixes.size() * ((payload_size + 63) / 64) + 1; i++)
                    left_store_payloads[i] = rng();
                std::vector<uint64_t> right_store_infixes {0b1'001010101'10000,
                    0b1'001010101'01011, 0b1'001111111'11001,
                    0b1'011111100'01000, 0b1'011111100'00111};
                uint64_t right_store_payloads[right_store_infixes.size() * ((payload_size + 63) / 64) + 1];
                for (uint32_t i = 0; i < right_store_infixes.size() * ((payload_size + 63) / 64) + 1; i++)
                    right_store_payloads[i] = rng();
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                for (uint32_t i = 0; i < left_store_infixes.size(); i++) {
                    copy_bitmap_to_bitmap(left_store_payloads, payload_size * i,
                                            check_payloads_contents[i], 0, payload_size);
                }
                for (uint32_t i = 0; i < right_store_infixes.size(); i++) {
                    copy_bitmap_to_bitmap(right_store_payloads, payload_size * i,
                                            check_payloads_contents[left_store_infixes.size() + i], 0, payload_size);
                }
                const uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);

                // Do the check
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit,
                                           false, left_store_payloads);

                    wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit,
                                           false, right_store_payloads);
                    wh_int_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks, check_payloads);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 1);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 3);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 3);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit,
                                           false, left_store_payloads);

                    wh_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit,
                                           false, right_store_payloads);
                    wh_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks, check_payloads);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 1);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 3);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 3);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_iter_destroy(it, check_it_write);
                }
            }

            SUBCASE("3") {
                std::vector<uint64_t> boundary_keys = 
                    {0b00000000'00000000'00000000'00000000UL,
                        0b00000000'00000000'11111111'11111111UL,
                        0b11111111'11111111'11111111'11111111UL};
                for (uint64_t key : boundary_keys) {
                    const uint64_t conv_key = to_big_endian_order(key);
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        payload[i]++;
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key), payload);
                }

                const uint8_t *key_ptr, *res_key;
                uint32_t key_len, res_size;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore *store;
                uint32_t dummy;

                // Setup check data
                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/merge/3");
                std::vector<uint64_t> left_store_infixes {0b0'000000000'10100,
                    0b0'000000000'10101, 0b0'000000001'10111,
                    0b0'000000010'10000, 0b0'000000010'00001,
                    0b0'000000011'10111, 0b1'000000000'00001};
                uint64_t left_store_payloads[left_store_infixes.size() * ((payload_size + 63) / 64) + 1];
                for (uint32_t i = 0; i < left_store_infixes.size() * ((payload_size + 63) / 64) + 1; i++)
                    left_store_payloads[i] = rng();
                std::vector<uint64_t> right_store_infixes {0b0'010100000'11111,
                    0b0'011110101'01000, 0b0'011110101'00001,
                    0b1'001001011'01011};
                uint64_t right_store_payloads[right_store_infixes.size() * ((payload_size + 63) / 64) + 1];
                for (uint32_t i = 0; i < right_store_infixes.size() * ((payload_size + 63) / 64) + 1; i++)
                    right_store_payloads[i] = rng();
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                for (uint32_t i = 0; i < left_store_infixes.size(); i++) {
                    copy_bitmap_to_bitmap(left_store_payloads, payload_size * i,
                                            check_payloads_contents[i], 0, payload_size);
                }
                for (uint32_t i = 0; i < right_store_infixes.size(); i++) {
                    copy_bitmap_to_bitmap(right_store_payloads, payload_size * i,
                                            check_payloads_contents[left_store_infixes.size() + i], 0, payload_size);
                }
                const uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);

                // Do the check
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit,
                                           false, left_store_payloads);

                    wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit,
                                           false, right_store_payloads);
                    wh_int_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks, check_payloads);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 1);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_int_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 3);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 3);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit,
                                           false, left_store_payloads);

                    wh_iter_skip1(it, check_it_write, check_it_unlock);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit,
                                           false, right_store_payloads);
                    wh_iter_destroy(it, check_it_write);

                    s.DeleteMerge({key_ptr, key_len});

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks, check_payloads);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 1);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_iter_skip1(it, check_it_write, check_it_unlock);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 3);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 3);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_iter_destroy(it, check_it_write);
                }
            }

            SUBCASE("delete all") {
                std::vector<std::pair<uint64_t, uint64_t>> keys;
                std::set<std::pair<uint64_t, uint64_t *>> boundary_keys;
                uint64_t *payloads_contents = new uint64_t[n_keys * (payload_size / 64 + 2)];
                std::vector<uint64_t *> payloads;
                for (uint32_t i = 0; i < 10; i++) {
                    keys.emplace_back((i + 1) * 0x0000000011111111UL, 0);
                    for (uint32_t j = 0; j < payload_size / 64 + 2; j++)
                        payloads_contents[i * (payload_size / 64 + 2) + j] = rng();
                    payloads.push_back(payloads_contents + i * (payload_size / 64 + 2));
                }
                {
                    const uint64_t min_key = std::numeric_limits<uint64_t>::min();
                    uint64_t *new_payload = new uint64_t[payload_size / 64 + 2];
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        new_payload[i] = rng();
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&min_key), sizeof(min_key), new_payload);
                    boundary_keys.insert({min_key, new_payload});

                    new_payload = new uint64_t[payload_size / 64 + 2];
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        new_payload[i] = rng();
                    const uint64_t max_key = std::numeric_limits<uint64_t>::max();
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&max_key), sizeof(max_key), new_payload);
                    boundary_keys.insert({max_key, new_payload});
                }
                for (uint32_t i = 0; i < keys.size(); i++) {
                    auto [key, bits_to_zero_out] = keys[i];
                    const uint64_t conv_key = to_big_endian_order(key);
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key), payloads[i]);
                    boundary_keys.insert({key, payloads[i]});
                }

                for (int32_t i = 1; i < 100; i++) {
                    const uint32_t shared = 34;
                    const uint32_t ignore = 1;
                    const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

                    const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                    const uint64_t interp = (l * i + r * (100 - i)) / 100;
                    const uint64_t value = to_big_endian_order(interp);
                    uint64_t *new_payload = new uint64_t[payload_size / 64 + 2];
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        new_payload[i] = rng();
                    s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)}, new_payload);
                    keys.emplace_back(interp, bits_to_zero_out);
                    payloads.push_back(new_payload);
                }

                for (int32_t i = 90; i >= 70; i -= 2) {
                    const uint32_t shared = 34;
                    const uint32_t ignore = 1;
                    const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

                    const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                    const uint64_t interp = (l * i + r * (100 - i)) / 100;
                    const uint64_t value = to_big_endian_order(interp);
                    uint64_t *new_payload = new uint64_t[payload_size / 64 + 2];
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        new_payload[i] = rng();
                    s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)}, new_payload);
                    keys.emplace_back(interp, bits_to_zero_out);
                    payloads.push_back(new_payload);
                }

                const uint32_t shamt = 16;
                for (int32_t i = 1; i < 50; i++) {
                    const uint32_t shared = 34;
                    const uint32_t ignore = 1;
                    const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<diva_type>::base_implicit_size - s.infix_size_;

                    const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                    const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
                    const uint64_t value = to_big_endian_order(interp);
                    uint64_t *new_payload = new uint64_t[payload_size / 64 + 2];
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        new_payload[i] = rng();
                    s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)}, new_payload);
                    keys.emplace_back(interp, bits_to_zero_out);
                    payloads.push_back(new_payload);
                }

                {
                    uint64_t value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
                    value = to_big_endian_order(value);
                    uint64_t *new_payload = new uint64_t[payload_size / 64 + 2];
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        new_payload[i] = 0xDEADBEEF;
                    s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)}, new_payload);
                    keys.emplace_back(__bswap_64(value), 0);
                    payloads.push_back(new_payload);
                    boundary_keys.insert({__bswap_64(value), new_payload});
                }

                for (int32_t i = 0; i < keys.size(); i++) {
                    const uint64_t query = to_big_endian_order(keys[i].first);
                    REQUIRE(s.PointQuery(reinterpret_cast<const uint8_t *>(&query), sizeof(query)));
                }

                const uint32_t shuffle_seed = 10;
                std::mt19937 shuffle_gen(shuffle_seed);
                uint32_t *perm = new uint32_t[keys.size()];
                for (uint32_t i = 0; i < keys.size(); i++)
                    perm[i] = i;
                std::shuffle(perm, perm + keys.size(), shuffle_gen);
                std::vector<std::pair<uint64_t, uint64_t>> keys_copy(keys.size());
                std::vector<uint64_t *> payloads_copy(keys.size());
                for (uint32_t i = 0; i < keys.size(); i++) {
                    keys_copy[i] = keys[perm[i]];
                    payloads_copy[i] = payloads[perm[i]];
                }
                keys = keys_copy;
                payloads = payloads_copy;
                delete[] perm;
                // Since we duplicate this infix, we're going to return true anyways, so account for this
                keys.emplace_back(0b00011101000010110110100111010000UL, 16);
                payloads.push_back(nullptr);

                for (int32_t i = 0; i < keys.size() - 1; i++) {
                    if (boundary_keys.find({keys[i].first, payloads[i]}) != boundary_keys.end())
                        boundary_keys.erase({keys[i].first, payloads[i]});

                    const uint64_t del_value = to_big_endian_order(keys[i].first);
                    s.Delete(reinterpret_cast<const uint8_t *>(&del_value), sizeof(del_value),
                             [&](const uint64_t *payload) { return compare_bitmap_to_bitmap(payload, 0, payloads[i], 0, payload_size); });

                    for (int32_t j = 0; j < keys.size(); j++) {
                        const uint64_t query = to_big_endian_order(keys[j].first);
                        auto it = boundary_keys.lower_bound({keys[j].first, payloads[j]});
                        bool expected_res = true;
                        if (it->first != keys[j].first) {
                            uint64_t next_key = to_big_endian_order(it->first);
                            --it;
                            uint64_t prev_key = to_big_endian_order(it->first);
                            auto [shared, ignore, implicit_size] = s.GetSharedIgnoreImplicitLengths(
                                    {reinterpret_cast<const uint8_t *>(&prev_key), sizeof(prev_key)},
                                    {reinterpret_cast<const uint8_t *>(&next_key), sizeof(next_key)});
                            const uint32_t bits_to_zero_out = std::max(sizeof(uint64_t) * 8 - shared - ignore - implicit_size - s.infix_size_,
                                                                       keys[j].second);
                            const uint64_t l = keys[j].first & (~BITMASK(bits_to_zero_out + 1));
                            const uint64_t r = keys[j].first | BITMASK(bits_to_zero_out + 1);
                            expected_res = std::find_if(keys.begin() + i + 1, keys.end(),
                                                        [&](std::pair<uint64_t, uint64_t> key) { return it->first < key.first
                                                                                                            && (l <= key.first && key.first <= r); })
                                                    != keys.end();
                        }
                        REQUIRE_EQ(s.PointQuery(reinterpret_cast<const uint8_t *>(&query), sizeof(query)), expected_res);
                    }
                }
            }

            SUBCASE("monte carlo") {
                const uint32_t n_keys = 2000000;
                const uint32_t rng_seed = 2;
                std::mt19937_64 rng(rng_seed);

                std::vector<std::pair<uint64_t, uint64_t *>> keys;
                uint64_t *payloads_contents = new uint64_t[n_keys * (payload_size / 64 + 2)];
                for (int32_t i = 0; i < n_keys; i++) {
                    for (uint32_t j = 0; j < payload_size / 64 + 2; j++)
                        payloads_contents[i * (payload_size / 64 + 2) + j] = rng();
                    keys.emplace_back(rng(), payloads_contents + i * (payload_size / 64 + 2));
                }
                std::sort(keys.begin(), keys.end());
                std::vector<std::pair<std::string, uint64_t *>> string_keys;
                for (int32_t i = 0; i < n_keys; i++) {
                    size_t str_length;
                    if constexpr (diva_type == DivaType::Int)
                        str_length = 8;
                    else
                        str_length = 6 + rng() % 3;
                    const uint64_t value = to_big_endian_order(keys[i].first);
                    string_keys.emplace_back(std::string(reinterpret_cast<const char *>(&value), str_length), keys[i].second);
                }
                std::shuffle(string_keys.begin(), string_keys.end(), rng);

                const uint32_t bulk_n_keys = n_keys / 32;
                std::sort(string_keys.begin(), string_keys.begin() + bulk_n_keys);
                std::string *bulk_string_keys = new std::string[bulk_n_keys];
                uint64_t **bulk_payloads = new uint64_t*[bulk_n_keys];
                std::transform(string_keys.begin(), string_keys.begin() + bulk_n_keys, bulk_string_keys,
                               [&](std::pair<std::string, uint64_t *> in) { return in.first; });
                std::transform(string_keys.begin(), string_keys.begin() + bulk_n_keys, bulk_payloads,
                               [&](std::pair<std::string, uint64_t *> in) { return in.second; });
                Diva<diva_type, PayloadType::FixedLength> s(infix_size, bulk_string_keys, bulk_string_keys + bulk_n_keys, seed, load_factor,
                                                            payload_size, (const uint64_t **) bulk_payloads);

                std::vector<bool> deleted(n_keys, false);
                uint32_t i = bulk_n_keys;
                while (i < n_keys) {
                    if (rng() % 2 == 0) {
                        s.Insert(string_keys[i].first, string_keys[i].second);
                        REQUIRE(s.PointQuery(string_keys[i].first));
                        i++;
                    }
                    else {
                        const uint32_t pos = rng() % i;
                        if (!deleted[pos]) {
                            s.Delete(string_keys[pos].first,
                                     [&](const uint64_t *payload) { return compare_bitmap_to_bitmap(payload, 0,
                                                                                                    string_keys[pos].second, 0,
                                                                                                    payload_size); });
                            deleted[pos] = true;
                        }
                    }
                }
            }

            SUBCASE("multiple matches, choose with remove function") {
                std::vector<uint64_t> boundary_keys = 
                    {0b00001111'11111111'00000000'00000000UL,
                        0b00010000'00000000'11111111'11111111UL,
                        0b00010111'11111111'00000000'00000000UL,
                        0b11111111'11111111'11111111'11111111UL};
                for (uint64_t key : boundary_keys) {
                    const uint64_t conv_key = to_big_endian_order(key);
                    for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                        payload[i]++;
                    s.AddTreeKey(reinterpret_cast<const uint8_t *>(&conv_key), sizeof(conv_key), payload);
                }

                const uint8_t *key_ptr, *res_key;
                uint32_t key_len, res_size;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                typename Diva<diva_type, PayloadType::FixedLength>::InfixStore *store;
                uint32_t dummy;

                // Setup check data
                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                std::vector<uint64_t> store_infixes {0b0'001010101'01011,
                    0b0'011011011'00100, 0b0'011011011'00001,
                    0b1'001010101'01000, 0b1'001010101'01011,
                    0b1'011011011'00001};
                uint64_t store_payloads[store_infixes.size() * ((payload_size + 63) / 64) + 1];
                for (uint32_t i = 0; i < store_infixes.size() * ((payload_size + 63) / 64) + 1; i++)
                    store_payloads[i] = rng();
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {{0x66fde211cf59f263, 0x57c0b6a4e, }, {0x38259b56a79404f6, 0x7e29334ad, }, {0x43a576f638dabd66, 0xc3ad36f17, }, {0xc09a41eae9bc45a, 0xe282cba54, }, {0x74c4e8ddb87e9bf, 0x5560ecdd9, }};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/multiple_matches_remove_function");
                ReadStorePayloadsFromFile("payloads/delete/multiple_matches_remove_function", payload_size, check_payloads);

                // Do the checks
                if constexpr (diva_type == DivaType::Int) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, store_infixes.data(), store_infixes.size(), total_implicit,
                                           false, store_payloads);
                    wh_int_iter_destroy(it, check_it_write);

                    const uint64_t delete_key = to_big_endian_order(0b00001111'11111111'01101101'10000000UL);
                    s.Delete(reinterpret_cast<const uint8_t *>(&delete_key), sizeof(delete_key),
                            [] (const uint64_t *payload) {return payload[0] == 0x2de5277990af454b; });

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks, check_payloads);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 1);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);
                    }
                    wh_int_iter_destroy(it, check_it_write);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, store_infixes.data(), store_infixes.size(), total_implicit,
                                           false, store_payloads);
                    wh_iter_destroy(it, check_it_write);

                    const uint64_t delete_key = to_big_endian_order(0b00001111'11111111'01101101'10000000UL);
                    s.Delete(reinterpret_cast<const uint8_t *>(&delete_key), sizeof(delete_key),
                            [] (const uint64_t *payload) {return payload[0] == 0x2de5277990af454b; });

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value), check_it_write);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                        AssertStoreContents(s, *store, occupieds_pos, checks, check_payloads);
                        s.GetSamplePayload(*store, 0, payload);
                        REQUIRE_EQ(payload[0], 1);
                        REQUIRE_EQ(payload[1] & BITMASK(payload_size - 64), 1);
                    }
                    wh_iter_destroy(it, check_it_write);
                }
            }
        }

        SUBCASE("serialize and deserialize") {
            const uint32_t infix_size = 10;

            SUBCASE("bulk loaded") {
                Diva<diva_type, PayloadType::FixedLength> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor,
                                                            payload_size, (const uint64_t **) payloads);

                const uint32_t buf_size = s.Size() + 20;
                char *buf = new char[buf_size];
                const uint32_t serialized_size = s.Serialize(buf);
                REQUIRE_EQ(s.Size(), serialized_size);

                Diva<diva_type, PayloadType::FixedLength> reconstructed_s(buf);
                AssertDivas(s, reconstructed_s);

                delete[] buf;
            }

            Diva<diva_type, PayloadType::FixedLength> s(infix_size, seed, load_factor, payload_size, true);
            const uint32_t sample_gap = n_keys / 20;
            for (uint32_t i = 0; i < n_keys; i += sample_gap)
                s.AddTreeKey(reinterpret_cast<const uint8_t *>(string_keys[i].data()), string_keys[i].size(),
                             payloads[i]);

            SUBCASE("samples only") {
                const uint32_t buf_size = s.Size() + 20;
                char *buf = new char[buf_size];
                const uint32_t serialized_size = s.Serialize(buf);
                REQUIRE_EQ(s.Size(), serialized_size);

                Diva<diva_type, PayloadType::FixedLength> reconstructed_s(buf);
                AssertDivas(s, reconstructed_s);

                delete[] buf;
            }

            uint32_t *perm = new uint32_t[n_keys];
            for (uint32_t i = 0; i < n_keys; i++)
                perm[i] = i;
            std::shuffle(perm, perm + n_keys, rng);
            for (uint32_t i = 0; i < n_keys; i++) {
                if (perm[i] % sample_gap == 0)
                    continue;
                s.Insert(string_keys[perm[i]], payloads[perm[i]]);
            }
            delete[] perm;

            SUBCASE("incremental insertions") {
                const uint32_t buf_size = s.Size() + 20;
                char *buf = new char[buf_size];
                const uint32_t serialized_size = s.Serialize(buf);
                REQUIRE_EQ(s.Size(), serialized_size);

                Diva<diva_type, PayloadType::FixedLength> reconstructed_s(buf);
                AssertDivas(s, reconstructed_s);

                delete[] buf;
            }
        }

        SUBCASE("warmup insert split and insert with iterator gets before inserts") {
            const uint32_t infix_size = 5;
            const uint32_t key_len = 48;
            const uint32_t key_len_words = (key_len + sizeof(uint64_t) - 1) / sizeof(uint64_t);
            const uint32_t payload_len_words = (payload_size + 63) / 64;
            Diva<diva_type, PayloadType::FixedLength> s(infix_size, seed + 1, load_factor, payload_size, true);

            if constexpr (diva_type != DivaType::Int) {
                const uint32_t n_keys = 10000000;
                const uint32_t sample_key_threshold = 50000;
                constexpr bool ascii = true;
                std::string valid_ascii_characters = "";
                for (char c = 'A'; c <= 'Z'; c++)
                    valid_ascii_characters += c;
                for (char c = 'a'; c <= 'z'; c++)
                    valid_ascii_characters += c;
                for (char c = '0'; c <= '9'; c++)
                    valid_ascii_characters += c;
                for (uint32_t i = 0; i < n_keys; i++) {
                    uint64_t key[key_len_words], payload[payload_len_words];
                    if constexpr (ascii) {
                        for (uint32_t j = 0; j < key_len_words * 8; j++)
                            reinterpret_cast<uint8_t *>(key)[j] = valid_ascii_characters[rng() % valid_ascii_characters.size()];
                    }
                    else {
                        for (uint32_t j = 0; j < key_len_words; j++)
                            key[j] = rng();
                    }
                    std::string_view key_sv = {reinterpret_cast<const char *>(key), key_len};
                    for (uint32_t j = 0; j < payload_len_words; j++)
                        payload[j] = rng();

                    // Check for existence
                    auto it = s.GetIterator(key_sv);
                    if (it.IsValid()) {
                        bool matches = false;
                        do {
                            auto [fetch_key, bit_length] = *it;
                            typename diva::Diva<diva_type, diva::PayloadType::FixedLength>::InfiniteByteString curr_key;
                            uint64_t it_payload[payload_len_words + 2];
                            // If the key prefix matches get the payload
                            if (memcmp(key, fetch_key.data(),
                                        std::min<uint32_t>(key_len, fetch_key.size() - 1)) == 0) {
                                matches = true;
                                if (bit_length < key_len * 8)
                                    matches = (key[bit_length / 8] & BITMASK(bit_length % 8 == 0 ? 8 : 8 - bit_length % 8)) 
                                                    == fetch_key[bit_length / 8];
                                if (matches) {
                                    it.GetPayload(it_payload);
                                }
                            }
                            else 
                                matches = false;
                            it++;
                        } while(it.IsValid() && matches);
                    }

                    s.Insert(key_sv, payload, i < sample_key_threshold ? 1024 : 0);
                }
            }
        }
    }


    template <DivaType diva_type>
    static void Iterator() {
        const uint32_t infix_size = 10;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t payload_size = 100;
        const uint32_t infix_store_target_size = Diva<diva_type, PayloadType::FixedLength>::infix_store_target_size;
        const uint32_t n_keys = 100 * infix_store_target_size;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        std::vector<uint64_t> keys;
        for (int32_t i = 0; i < n_keys; i++)
            keys.push_back(rng());
        std::sort(keys.begin(), keys.end());
        std::vector<std::string> string_keys;
        for (int32_t i = 0; i < n_keys; i++) {
            size_t str_length;
            const size_t length_offset = rng() % 3;
            if constexpr (diva_type == DivaType::Int)
                str_length = 8;
            else
                str_length = 6 + length_offset;
            const uint64_t value = to_big_endian_order(keys[i]);
            string_keys.emplace_back(reinterpret_cast<const char *>(&value), str_length);
        }
        std::sort(string_keys.begin(), string_keys.end());

        uint64_t *payloads_contents = new uint64_t[n_keys * (payload_size / 64 + 2)];
        uint64_t **payloads = new uint64_t *[n_keys];
        for (uint32_t i = 0; i < n_keys; i++) {
            for (uint32_t j = 0; j < payload_size / 64 + 2; j++)
                payloads_contents[i * (payload_size / 64 + 2) + j] = rng();
            payloads[i] = &(payloads_contents[i * (payload_size / 64 + 2)]);
        }

        SUBCASE("bulk load start") {
            Diva<diva_type, PayloadType::FixedLength> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor,
                                                        payload_size, (const uint64_t **) payloads);

            SUBCASE("iterate over everything") {
                uint32_t ind = 0;
                auto it = s.GetIterator(string_keys[ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;
                    REQUIRE(key.IsPrefixOf(exp, last_bits_to_ignore));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind++;
                } while (ind < n_keys);
                REQUIRE(!it.IsValid());
            }

            SUBCASE("iterate from middle: existing key") {
                uint32_t ind = 1030;
                auto it = s.GetIterator(string_keys[ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    REQUIRE(key.IsPrefixOf(exp, bit_length % 8 == 0 ? 0 : 8 - bit_length % 8));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind++;
                } while (ind < n_keys);
                REQUIRE(!it.IsValid());
            }

            SUBCASE("iterate from middle: non-existent key") {
                uint32_t ind = 1031;
                auto it = s.GetIterator((keys[ind - 1] + keys[ind]) / 2);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    REQUIRE(key.IsPrefixOf(exp, bit_length % 8 == 0 ? 0 : 8 - bit_length % 8));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind++;
                } while (ind < n_keys);
                REQUIRE(!it.IsValid());
            }
        }

        SUBCASE("empty start") {
            Diva<diva_type, PayloadType::FixedLength> s(infix_size, seed, load_factor, payload_size, true);
            
            const uint32_t sample_gap = n_keys / 20;
            for (uint32_t i = 0; i < n_keys; i += sample_gap)
                s.AddTreeKey(reinterpret_cast<const uint8_t *>(string_keys[i].data()), string_keys[i].size(), payloads[i]);

            SUBCASE("iterate over every sample") {
                uint32_t ind = 0;
                auto it = s.GetIterator(string_keys[ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;
                    REQUIRE(key.IsPrefixOf(exp, last_bits_to_ignore));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind += sample_gap;
                } while (ind < n_keys);
                REQUIRE(!it.IsValid());
            }

            SUBCASE("iterate from middle: existing sample") {
                uint32_t ind = 10 * sample_gap;
                auto it = s.GetIterator(string_keys[ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;
                    REQUIRE(key.IsPrefixOf(exp, last_bits_to_ignore));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind += sample_gap;
                } while (ind < n_keys);
                REQUIRE(!it.IsValid());
            }

            SUBCASE("iterate from middle: non-existent sample") {
                uint32_t ind = sample_gap - 10;
                auto it = s.GetIterator(string_keys[ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    const uint32_t exp_ind = ((ind + sample_gap - 1) / sample_gap) * sample_gap;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[exp_ind].data()), static_cast<uint32_t>(string_keys[exp_ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[exp_ind].data()), static_cast<uint32_t>(string_keys[exp_ind].size())};
                    }
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;
                    REQUIRE(key.IsPrefixOf(exp, last_bits_to_ignore));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[exp_ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind += sample_gap;
                } while (((ind + sample_gap - 1) / sample_gap) * sample_gap < n_keys);
                REQUIRE(!it.IsValid());
            }

            const uint32_t infix_gap = 2;
            uint32_t *perm = new uint32_t[n_keys / infix_gap + 1];
            for (uint32_t i = 0; i < n_keys / infix_gap; i++)
                perm[i] = i * infix_gap;
            std::shuffle(perm, perm + n_keys / infix_gap, rng);
            for (uint32_t i = 0; i < n_keys / infix_gap; i++) {
                if (perm[i] % sample_gap == 0)
                    continue;
                s.Insert(string_keys[perm[i]], payloads[perm[i]]);
            }
            delete[] perm;

            const int32_t neighborhood_range = infix_gap * 30;
            SUBCASE("iterate over every key") {
                int32_t ind = 0;
                auto it = s.GetIterator();
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                    }
                    else
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;

                    bool found = false;
                    for (uint32_t i = std::max<int32_t>(0, ind - neighborhood_range); i < ind + neighborhood_range; i += infix_gap) {
                        typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString exp = {reinterpret_cast<const uint8_t *>(string_keys[i].data()),
                                                                                                      static_cast<uint32_t>(string_keys[i].size())};
                        if (key.IsPrefixOf(exp, last_bits_to_ignore)) {
                            uint64_t it_payload[payload_size / 64 + 2];
                            it.GetPayload(it_payload);
                            if (compare_bitmap_to_bitmap(payloads[i], 0, it_payload, 0, payload_size)) {
                                found = true;
                                break;
                            }
                        }
                    }
                    REQUIRE(found);

                    it++;
                    ind += infix_gap;
                } while (ind < n_keys);
                REQUIRE(!it.IsValid());
            }

            SUBCASE("iterate from middle: existing key") {
                uint32_t ind = 8 * infix_gap;
                auto it = s.GetIterator(string_keys[ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                    }
                    else
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;

                    bool found = false;
                    for (uint32_t i = std::max<int32_t>(0, ind - neighborhood_range); i < ind + neighborhood_range; i += infix_gap) {
                        typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString exp = {reinterpret_cast<const uint8_t *>(string_keys[i].data()),
                                                                                                      static_cast<uint32_t>(string_keys[i].size())};
                        if (key.IsPrefixOf(exp, last_bits_to_ignore)) {
                            uint64_t it_payload[payload_size / 64 + 2];
                            it.GetPayload(it_payload);
                            if (compare_bitmap_to_bitmap(payloads[i], 0, it_payload, 0, payload_size)) {
                                found = true;
                                break;
                            }
                        }
                    }
                    REQUIRE(found);

                    it++;
                    ind += infix_gap;
                } while (ind < n_keys);
                REQUIRE(!it.IsValid());
            }

            SUBCASE("iterate from middle: non-existent key") {
                uint32_t ind = 10 * infix_gap - 1;
                auto it = s.GetIterator(string_keys[ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                    }
                    else
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;

                    bool found = false;
                    for (uint32_t i = std::max<int32_t>(0, ind + 1 - neighborhood_range); i < ind + 1 + neighborhood_range; i += infix_gap) {
                        typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString exp = {reinterpret_cast<const uint8_t *>(string_keys[i].data()),
                                                                                                      static_cast<uint32_t>(string_keys[i].size())};
                        if (key.IsPrefixOf(exp, last_bits_to_ignore)) {
                            uint64_t it_payload[payload_size / 64 + 2];
                            it.GetPayload(it_payload);
                            if (compare_bitmap_to_bitmap(payloads[i], 0, it_payload, 0, payload_size)) {
                                found = true;
                                break;
                            }
                        }
                    }
                    REQUIRE(found);

                    it++;
                    ind += infix_gap;
                } while (((ind + infix_gap - 1) / infix_gap) * infix_gap < n_keys);
                REQUIRE(!it.IsValid());
            }
        }

        SUBCASE("iterate bounded range") {
            SUBCASE("sample end") {
                Diva<diva_type, PayloadType::FixedLength> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor,
                                                            payload_size, (const uint64_t **) payloads);
                uint32_t ind = 0;
                const uint32_t end_ind = 20 * infix_store_target_size;
                auto it = s.GetIterator(string_keys[ind], string_keys[end_ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;
                    REQUIRE(key.IsPrefixOf(exp, last_bits_to_ignore));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind++;
                } while (ind <= end_ind);
                REQUIRE(!it.IsValid());
            }

            SUBCASE("infix end") {
                Diva<diva_type, PayloadType::FixedLength> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor,
                                                            payload_size, (const uint64_t **) payloads);
                uint32_t ind = 0;
                const uint32_t end_ind = 20 * infix_store_target_size - 10;
                auto it = s.GetIterator(string_keys[ind], string_keys[end_ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;
                    REQUIRE(key.IsPrefixOf(exp, last_bits_to_ignore));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind++;
                } while (ind <= end_ind);
                REQUIRE(!it.IsValid());
            }

            Diva<diva_type, PayloadType::FixedLength> s(infix_size, seed, load_factor, payload_size, true);
            const uint32_t sample_gap = n_keys / 20;
            for (uint32_t i = 0; i < n_keys; i += sample_gap)
                s.AddTreeKey(reinterpret_cast<const uint8_t *>(string_keys[i].data()), string_keys[i].size(), payloads[i]);

            const uint32_t infix_gap = 2;
            uint32_t *perm = new uint32_t[n_keys / infix_gap + 1];
            for (uint32_t i = 0; i < n_keys / infix_gap; i++)
                perm[i] = i * infix_gap;
            std::shuffle(perm, perm + n_keys / infix_gap, rng);
            for (uint32_t i = 0; i < n_keys / infix_gap; i++) {
                if (perm[i] % sample_gap == 0)
                    continue;
                s.Insert(string_keys[perm[i]], payloads[perm[i]]);
            }
            delete[] perm;

            SUBCASE("non-existent end") {
                uint32_t ind = 0;
                const uint32_t end_ind = 20 * infix_store_target_size - 1;
                auto it = s.GetIterator(string_keys[ind], string_keys[end_ind]);
                do {
                    REQUIRE(it.IsValid());
                    auto [fetch_key, bit_length] = *it;
                    typename Diva<diva_type, PayloadType::FixedLength>::InfiniteByteString key, exp;
                    if constexpr (diva_type == DivaType::Int) {
                        fetch_key = to_big_endian_order(fetch_key);
                        key = {reinterpret_cast<const uint8_t *>(&fetch_key), (bit_length + 7) / 8};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    else {
                        key = {reinterpret_cast<const uint8_t *>(fetch_key.data()), static_cast<uint32_t>(fetch_key.size())};
                        exp = {reinterpret_cast<const uint8_t *>(string_keys[ind].data()), static_cast<uint32_t>(string_keys[ind].size())};
                    }
                    const uint32_t last_bits_to_ignore = bit_length % 8 == 0 ? 0 : 8 - bit_length % 8;
                    REQUIRE(key.IsPrefixOf(exp, last_bits_to_ignore));

                    uint64_t it_payload[payload_size / 64 + 2];
                    it.GetPayload(it_payload);
                    REQUIRE(compare_bitmap_to_bitmap(payloads[ind], 0, it_payload, 0, payload_size));

                    it++;
                    ind += infix_gap;
                } while (((ind + infix_gap - 1) / infix_gap) * infix_gap < end_ind);
                REQUIRE(!it.IsValid());
            }
        }

        SUBCASE("concurrency with payloads") {
            const uint32_t infix_size = 8;
            const uint32_t seed = 4;
            const float load_factor = 0.95;
            const uint32_t n_keys = 30000000;
            const uint32_t n_threads = 16;
            const uint32_t n_bulk = std::min(n_keys / n_threads, n_keys / 8);
            const uint32_t query_period = 5000000;
            
            constexpr bool ascii = false;
            std::string valid_ascii_characters = "";
            for (char c = 'A'; c <= 'Z'; c++)
                valid_ascii_characters += c;
            for (char c = 'a'; c <= 'z'; c++)
                valid_ascii_characters += c;
            for (char c = '0'; c <= '9'; c++)
                valid_ascii_characters += c;

            const uint32_t rng_seed = 2;
            std::mt19937_64 rng(rng_seed);

            std::vector<std::string_view> string_keys;
            for (int32_t i = 0; i < n_keys; i++) {
                size_t str_length;
                if constexpr (diva_type == DivaType::Int)
                    str_length = 8;
                else
                    str_length = 10 + rng() % 3;
                char *str = new char[48];
                for (uint32_t i = 0; i < str_length; i++) {
                    if constexpr (ascii)
                        str[i] = valid_ascii_characters[rng() % valid_ascii_characters.size()];
                    else 
                        str[i] = std::max(1UL, rng());
                }
                string_keys.emplace_back(reinterpret_cast<const char *>(str), str_length);
            }
            std::shuffle(string_keys.begin(), string_keys.end(), rng);
            std::sort(string_keys.begin(), string_keys.begin() + n_bulk);
            uint64_t *payloads_contents = new uint64_t[n_keys * (payload_size / 64 + 2)];
            uint64_t **payloads = new uint64_t *[n_keys];
            for (uint32_t i = 0; i < n_keys; i++) {
                for (uint32_t j = 0; j < payload_size / 64 + 2; j++)
                    payloads_contents[i * (payload_size / 64 + 2) + j] = rng();
                payloads[i] = &(payloads_contents[i * (payload_size / 64 + 2)]);
            }

            Diva<diva_type, PayloadType::FixedLength> s(infix_size,
                                                        string_keys.begin(),
                                                        string_keys.begin() + n_bulk,
                                                        seed,
                                                        load_factor,
                                                        payload_size,
                                                        (const uint64_t **) payloads);

            std::vector<std::thread> threads;
            std::vector<std::atomic<bool>> deleted(string_keys.size());
            for (uint32_t i = 0; i < string_keys.size(); i++)
                deleted[i].store(false, std::memory_order_release);
            for (uint32_t i = 0; i < n_threads; i++) {
                threads.emplace_back([&, i] {
                            std::mt19937_64 rng(rng_seed + i + 1);
                            uint32_t ti = n_bulk + i;
                            while (ti < n_keys) {
                                const bool insert = (rng() % 2) > 0;
                                if (insert) {
                                    auto it = s.GetIterator(string_keys[ti]);
                                    s.Insert(string_keys[ti], payloads[ti], rng());
                                    REQUIRE(s.PointQuery(string_keys[ti]));
                                    ti += n_threads;
                                }
                                else {
                                    const uint32_t offset = rng() % ((ti - n_threads) / n_threads) + 1;
                                    const uint32_t pos = ti - offset * n_threads;
                                    assert(pos < ti && (ti - pos) % n_threads == 0);
                                    if (!deleted[pos].load(std::memory_order_acquire)) {
                                        auto it = s.GetIterator(string_keys[pos]);
                                        s.Delete(string_keys[pos],
                                                [&](const uint64_t *ptr) {
                                                    return compare_bitmap_to_bitmap(ptr, 0, payloads[pos], 0, payload_size);
                                                });
                                        deleted[pos].store(true, std::memory_order_release);
                                    }
                                }
                                if ((ti - n_bulk - i) % query_period == 0) {    // Ensure no data loss
                                    std::cerr << "querying i=" << i << " ti=" << ti << std::endl;
                                    for (int32_t tj = ti - n_threads; tj >= 0; tj -= n_threads) {
                                        if (!deleted[tj].load(std::memory_order_acquire)) {
                                            bool found = false;
                                            for (auto it = s.GetIterator(string_keys[tj], string_keys[tj]);
                                                    it.IsValid();
                                                    it++) {
                                                uint64_t payload[(payload_size + 63) / 64 + 1];
                                                it.GetPayload(payload);
                                                if (compare_bitmap_to_bitmap(payloads[tj], 0, payload, 0, payload_size)) {
                                                    found = true;
                                                    break;
                                                }
                                            }
                                            REQUIRE(found);
                                        }
                                    }
                                }
                            }
                        });
            }
            for (auto& t : threads)
                t.join();
        }


        SUBCASE("concurrently iterating and deleting with payloads") {
            const uint32_t infix_size = 10;
            const uint32_t seed = 2;
            const float load_factor = 0.8;
            const uint32_t n_keys = 30000000;
            const uint32_t n_duplicates = 3;
            const uint32_t n_threads = 8;
            const uint32_t n_bulk = std::min(n_keys / n_threads, n_keys / 8);
            const uint64_t delete_threshold = 10000000;
            const uint64_t query_period = 5000000;

            constexpr bool ascii = false;
            std::string valid_ascii_characters = "";
            for (char c = 'A'; c <= 'Z'; c++)
                valid_ascii_characters += c;
            for (char c = 'a'; c <= 'z'; c++)
                valid_ascii_characters += c;
            for (char c = '0'; c <= '9'; c++)
                valid_ascii_characters += c;

            const uint32_t rng_seed = 2;
            std::mt19937_64 rng(rng_seed);

            std::vector<std::string_view> string_keys;
            for (int32_t i = 0; i < n_keys; i++) {
                size_t str_length;
                if constexpr (diva_type == DivaType::Int)
                    str_length = 8;
                else
                    str_length = 40 + rng() % 3;
                char *str = new char[48];
                for (uint32_t i = 0; i < str_length; i++) {
                    if constexpr (ascii)
                        str[i] = valid_ascii_characters[rng() % valid_ascii_characters.size()];
                    else
                        str[i] = std::max(1UL, rng());
                }
                string_keys.emplace_back(reinterpret_cast<const char *>(str), str_length);
            }
            std::shuffle(string_keys.begin(), string_keys.end(), rng);
            std::sort(string_keys.begin(), string_keys.begin() + n_bulk);
            uint64_t *payloads_contents = new uint64_t[n_keys * (payload_size / 64 + 2)];
            uint64_t **payloads = new uint64_t *[n_keys];
            for (uint32_t i = 0; i < n_keys; i++) {
                for (uint32_t j = 0; j < payload_size / 64 + 2; j++)
                    payloads_contents[i * (payload_size / 64 + 2) + j] = rng();
                payloads[i] = &(payloads_contents[i * (payload_size / 64 + 2)]);
            }

            Diva<diva_type, PayloadType::FixedLength> s(infix_size,
                                                        string_keys.begin(),
                                                        string_keys.begin() + n_bulk,
                                                        seed,
                                                        load_factor,
                                                        payload_size,
                                                        (const uint64_t **) payloads);
            REQUIRE_EQ(s.GetNumKeys(), n_bulk);

            std::cerr << "querying bulk loaded" << std::endl;
            for (int32_t i = 0; i < n_bulk; i++) {
                bool found = false;
                for (auto it = s.GetIterator(string_keys[i], string_keys[i]); it.IsValid(); ++it) {
                    uint64_t payload[(payload_size + 63) / 64 + 1];
                    it.GetPayload(payload);
                    if (compare_bitmap_to_bitmap(payloads[i], 0, payload, 0, payload_size))
                        found = true;
                }
                REQUIRE(found);
            }

            std::atomic<uint64_t> n_keys_inserted_overall = 1;
            std::vector<std::thread> threads;
            for (uint32_t i = 0; i < n_threads; i++) {
                threads.emplace_back([&, i] {
                        if (i > 0) {
                            for (uint32_t ti = n_bulk + i; ti < n_keys; ti += n_threads) {
                                for (uint32_t j = 0; j < n_duplicates; j++) {
                                    for (auto it = s.GetIterator(string_keys[ti], string_keys[ti]); 
                                            it.IsValid();
                                            ++it) {
                                        uint64_t payload[(payload_size + 63) / 64 + 1];
                                        it.GetPayload(payload);
                                        compare_bitmap_to_bitmap(payloads[ti], 0, payload, 0, payload_size);
                                    }
                                    payloads[ti][0] = n_keys_inserted_overall.load(std::memory_order_acquire);
                                    s.Insert(string_keys[ti], payloads[ti], rng());
                                }
                                n_keys_inserted_overall.fetch_add(1, std::memory_order_release);
                                if (payloads[ti][0] > delete_threshold)
                                    REQUIRE(s.PointQuery(string_keys[ti]));
                                if ((ti - n_bulk - i) % query_period == 0) {    // Ensure no data loss
                                    std::cerr << "querying i=" << i << " ti=" << ti << std::endl;
                                    for (int32_t tj = ti; tj >= 0; tj -= n_threads) {
                                        if (payloads[tj][0] > delete_threshold) {
                                            bool found = false;
                                            for (auto it = s.GetIterator(string_keys[tj], string_keys[tj]); 
                                                    it.IsValid();
                                                    ++it) {
                                                uint64_t payload[(payload_size + 63) / 64 + 1];
                                                it.GetPayload(payload);
                                                if (compare_bitmap_to_bitmap(payloads[tj], 0, payload, 0, payload_size))
                                                    found = true;
                                            }
                                            REQUIRE(found);
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            while (n_keys_inserted_overall.load(std::memory_order_acquire) <= delete_threshold)
                                cpu_pause();
                            s.DeleteRange(nullptr, 0, nullptr, 0, 
                                    [=](const uint64_t *payload) { 
                                        return payload[0] <= delete_threshold && payload[0] > 0;
                                    });
                        }
                    });
            }
            for (auto& t : threads)
                t.join();

            // Make sure there are no entries that should've been deleted
            for (auto it = s.GetIterator(); it.IsValid(); ++it) {
                uint64_t payload[(payload_size + 63) / 64 + 1];
                it.GetPayload(payload);
                REQUIRE((payload[0] > delete_threshold || payload[0] == 0));
            }
            for (uint32_t i = 0; i < n_keys; i++) {
                if (i >= n_bulk && (i - n_bulk) % n_threads == 0)
                    continue;
                bool found = false;
                for (auto it = s.GetIterator(string_keys[i], string_keys[i]); it.IsValid(); ++it) {
                    uint64_t payload[(payload_size + 63) / 64 + 1];
                    it.GetPayload(payload);
                    if (compare_bitmap_to_bitmap(payloads[i], 0, payload, 0, payload_size)) {
                        found = true;
                        break;
                    }
                }
                REQUIRE_EQ(found, (payloads[i][0] > delete_threshold || payloads[i][0] == 0));
            }
        }
    }


private:
    static void WriteStoreContentsToFile(std::string path,
                                         const std::vector<uint32_t> &occupieds_pos, 
                                         const std::vector<std::tuple<uint32_t, bool, uint64_t>> &checks,
                                         const uint64_t * const *payloads = nullptr,
                                         uint32_t payload_size = 0) {
        const std::string path_prefix = "./tests/data/diva/";
        path = path_prefix + path;
        std::ofstream fout;

        fout.open(path + "/occupieds_pos", std::ios::out | std::ios::binary);
        fout.write(reinterpret_cast<const char *>(occupieds_pos.data()),
                occupieds_pos.size() * sizeof(occupieds_pos[0]));
        fout.close();

        fout.open(path + "/checks", std::ios::out | std::ios::binary);
        fout.write(reinterpret_cast<const char *>(checks.data()),
                checks.size() * sizeof(checks[0]));
        fout.close();

        if (payloads != nullptr) {
            fout.open(path + "/payloads", std::ios::out | std::ios::binary);
            const uint32_t payload_size_bytes = sizeof(uint64_t) * ((payload_size + 63) / 64);
            for (int32_t i = 0; i < checks.size(); i++)
                fout.write(reinterpret_cast<const char *>(payloads[i]), payload_size_bytes);
            fout.close();
        }
    }

    static std::pair<std::vector<uint32_t>, std::vector<std::tuple<uint32_t, bool, uint64_t>>>
    ReadStoreContentsFromFile(std::string path) {
        const std::string path_prefix = "./tests/data/diva/";
        path = path_prefix + path;
        std::ifstream fin;

        const std::string occupieds_pos_path = path + "/occupieds_pos";
        const uint32_t occupieds_pos_n_bytes =
            std::filesystem::file_size(occupieds_pos_path);
        fin.open(occupieds_pos_path, std::ios::in | std::ios::binary);
        std::vector<uint32_t> occupieds_pos(occupieds_pos_n_bytes /
                sizeof(uint32_t));
        fin.read(reinterpret_cast<char *>(occupieds_pos.data()),
                occupieds_pos_n_bytes);
        fin.close();

        const std::string checks_path = path + "/checks";
        const uint32_t checks_n_bytes = std::filesystem::file_size(checks_path);
        fin.open(checks_path, std::ios::in | std::ios::binary);
        std::vector<std::tuple<uint32_t, bool, uint64_t>> checks(
                checks_n_bytes / sizeof(std::tuple<uint32_t, bool, uint64_t>));
        fin.read(reinterpret_cast<char *>(checks.data()), checks_n_bytes);
        fin.close();

        return {std::move(occupieds_pos), std::move(checks)};
    }

    static void ReadStorePayloadsFromFile(std::string path,
                                          uint32_t payload_size,
                                          uint64_t **out) {
        const std::string path_prefix = "./tests/data/diva/";
        path = path_prefix + path;
        std::ifstream fin;

        const std::string payloads_path = path + "/payloads";
        const uint32_t payloads_n_bytes = std::filesystem::file_size(payloads_path);
        const uint32_t payload_size_bytes = sizeof(uint64_t) * ((payload_size + 63) / 64);
        fin.open(payloads_path, std::ios::in | std::ios::binary);
        for (int32_t i = 0; i < payloads_n_bytes / payload_size_bytes; i++)
            fin.read(reinterpret_cast<char *>(out[i]), payload_size_bytes);
        fin.close();
    }

    template <DivaType diva_type, PayloadType payload_type>
    static void AssertStoreContents(const Diva<diva_type, payload_type> &s,
                                    const typename Diva<diva_type, payload_type>::InfixStore &store,
                                    const std::vector<uint32_t> &occupieds_pos,
                                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> &checks,
                                    const uint64_t * const *check_payloads = nullptr) {
        REQUIRE_NE(store.ptr, nullptr);
        if constexpr (diva_type != DivaType::BinaryTrie)
            REQUIRE_EQ(store.GetFullSlotCount(), checks.size());
        if constexpr (payload_type == PayloadType::FixedLength)
            assert(check_payloads != nullptr);
        const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + Diva<diva_type, payload_type>::num_metadata_offset_words;
        const uint64_t *runends = store.ptr + Diva<diva_type, payload_type>::num_metadata_offset_words
                                    + Diva<>::infix_store_target_size / 64;
        uint32_t ind = 0;
        for (uint32_t i = 0; i < Diva<>::infix_store_target_size; i++) {
            if (ind < occupieds_pos.size() && i == occupieds_pos[ind]) {
                REQUIRE_EQ(get_bitmap_bit(occupieds, i), 1);
                ind++;
            } else
                REQUIRE_EQ(get_bitmap_bit(occupieds, i), 0);
        }

        const uint32_t total_size = s.scaled_sizes_[store.GetSizeGrade()];
        ind = 0;
        uint32_t runend_count = 0;
        for (int32_t i = 0; i < total_size; i++) {
            const uint64_t slot = s.GetSlot(store, i);
            uint64_t read_payload[s.payload_size_ / 64 + 2];
            if constexpr (payload_type == PayloadType::FixedLength)
                s.GetPayload(store, i, read_payload);
            if (ind < checks.size()) {
                const auto [pos, runend, value] = checks[ind];
                if (i == pos) {
                    REQUIRE_EQ(value, slot);
                    REQUIRE_EQ(get_bitmap_bit(runends, i), runend);
                    if constexpr (payload_type == PayloadType::FixedLength) {
                        REQUIRE(compare_bitmap_to_bitmap(
                                    read_payload, 0, check_payloads[ind], 0, s.payload_size_));
                    }
                    runend_count += runend;
                    ind++;
                } else {
                    REQUIRE_EQ(slot, 0ULL);
                    REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
                }
            } else {
                REQUIRE_EQ(slot, 0ULL);
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
        }
        REQUIRE_EQ(occupieds_pos.size(), runend_count);

        uint32_t check_popcnts[2] = {};
        for (int32_t i = 0; i < Diva<>::infix_store_target_size / 128; i++) {
            check_popcnts[0] += __builtin_popcountll(occupieds[i]);
            const uint64_t masked_runends = runends[i] & BITMASK(std::min(64, std::max<int32_t>(total_size - 64 * i, 0)));
            check_popcnts[1] += __builtin_popcountll(masked_runends);
        }
        REQUIRE_EQ(popcnts[0], check_popcnts[0]);
        REQUIRE_EQ(popcnts[1], check_popcnts[1]);
    }


    template <DivaType diva_type, PayloadType payload_type>
    static void AssertDivas(const Diva<diva_type, payload_type>& a, const Diva<diva_type, payload_type>& b) {
        REQUIRE_EQ(a.infix_store_target_size, b.infix_store_target_size);
        REQUIRE_EQ(a.base_implicit_size, b.base_implicit_size);
        REQUIRE_EQ(a.scale_shift, b.scale_shift);
        REQUIRE_EQ(a.scale_implicit_shift, b.scale_implicit_shift);
        REQUIRE_EQ(a.size_scalar_count, b.size_scalar_count);
        REQUIRE_EQ(a.size_scalar_shrink_grow_sep, b.size_scalar_shrink_grow_sep);
        REQUIRE_EQ(a.load_factor_, b.load_factor_);
        REQUIRE_EQ(a.load_factor_alt_, b.load_factor_alt_);

        REQUIRE_EQ(a.infix_size_, b.infix_size_);
        REQUIRE_EQ(a.rng_seed_, b.rng_seed_);

        for (int32_t i = 0; i < a.size_scalar_count; i++) {
            REQUIRE_EQ(a.size_scalars_[i], b.size_scalars_[i]);
            REQUIRE_EQ(a.scaled_sizes_[i], b.scaled_sizes_[i]);
            REQUIRE_EQ(a.implicit_scalars_[i], b.implicit_scalars_[i]);
        }

        const uint8_t *tree_key_a, *tree_key_b;
        uint32_t tree_key_a_len, tree_key_b_len, dummy;
        typename Diva<diva_type>::InfixStore *store_a, *store_b;
        const bool check_it_write = false;
        const bool check_it_unlock = true;
        if constexpr (diva_type == DivaType::Int) {
            wormhole_int_iter it_a, it_b;
            it_a.ref = a.better_tree_int_;
            it_a.map = a.better_tree_int_->map;
            it_a.leaf = nullptr;
            it_a.is = 0;
            it_b.ref = b.better_tree_int_;
            it_b.map = b.better_tree_int_->map;
            it_b.leaf = nullptr;
            it_b.is = 0;
            wh_int_iter_seek(&it_a, nullptr, 0, check_it_write);
            wh_int_iter_seek(&it_b, nullptr, 0, check_it_write);
            while (wh_int_iter_valid(&it_a) && wh_int_iter_valid(&it_b)) {
                wh_int_iter_peek_ref(&it_a, reinterpret_cast<const void **>(&tree_key_a), &tree_key_a_len, 
                                            reinterpret_cast<void **>(&store_a), &dummy);
                wh_int_iter_peek_ref(&it_b, reinterpret_cast<const void **>(&tree_key_b), &tree_key_b_len, 
                                            reinterpret_cast<void **>(&store_b), &dummy);
                REQUIRE_EQ(tree_key_a_len, tree_key_b_len);
                REQUIRE_EQ(memcmp(tree_key_a, tree_key_b, tree_key_a_len), 0);
                REQUIRE_EQ(store_a->status, store_b->status);
                const uint32_t slot_count = a.scaled_sizes_[store_a->GetSizeGrade()];
                const uint32_t word_count = store_a->GetPtrWordCount(slot_count, a.infix_size_);
                REQUIRE_EQ(memcmp(store_a->ptr, store_b->ptr, word_count * sizeof(uint64_t)), 0);
                if constexpr (payload_type == PayloadType::FixedLength) {
                    REQUIRE_EQ(store_a->num_sample_payloads, store_b->num_sample_payloads);
                    const uint64_t *store_a_sample_payloads_ptr = reinterpret_cast<const uint64_t *>(store_a->ptr[1]);
                    const uint64_t *store_b_sample_payloads_ptr = reinterpret_cast<const uint64_t *>(store_b->ptr[1]);
                    const uint32_t num_sample_payload_bytes = (store_a->num_sample_payloads * a.payload_size_ + 7) / 8;
                    REQUIRE_EQ(memcmp(store_a_sample_payloads_ptr, store_b_sample_payloads_ptr, num_sample_payload_bytes), 0);
                }
                wh_int_iter_skip1(&it_a, check_it_write, check_it_unlock);
                wh_int_iter_skip1(&it_b, check_it_write, check_it_unlock);
            }
            REQUIRE_EQ(wh_int_iter_valid(&it_a), wh_int_iter_valid(&it_b));
            if (it_a.leaf)
                wormleaf_int_unlock_read(it_a.leaf);
            if (it_b.leaf)
                wormleaf_int_unlock_read(it_b.leaf);
        }
        else {
            wormhole_iter it_a, it_b;
            it_a.ref = a.better_tree_;
            it_a.map = a.better_tree_->map;
            it_a.leaf = nullptr;
            it_a.is = 0;
            it_b.ref = b.better_tree_;
            it_b.map = b.better_tree_->map;
            it_b.leaf = nullptr;
            it_b.is = 0;
            wh_iter_seek(&it_a, nullptr, 0, check_it_write);
            wh_iter_seek(&it_b, nullptr, 0, check_it_write);
            while (wh_iter_valid(&it_a) && wh_iter_valid(&it_b)) {
                wh_iter_peek_ref(&it_a, reinterpret_cast<const void **>(&tree_key_a), &tree_key_a_len, 
                                        reinterpret_cast<void **>(&store_a), &dummy);
                wh_iter_peek_ref(&it_b, reinterpret_cast<const void **>(&tree_key_b), &tree_key_b_len, 
                                        reinterpret_cast<void **>(&store_b), &dummy);
                REQUIRE_EQ(tree_key_a_len, tree_key_b_len);
                REQUIRE_EQ(memcmp(tree_key_a, tree_key_b, tree_key_a_len), 0);
                REQUIRE_EQ(store_a->status, store_b->status);
                const uint32_t slot_count = a.scaled_sizes_[store_a->GetSizeGrade()];
                const uint32_t word_count = store_a->GetPtrWordCount(slot_count, a.infix_size_);
                REQUIRE_EQ(store_a->ptr[0], store_b->ptr[0]);
                REQUIRE_EQ(memcmp(store_a->ptr + 2, store_b->ptr + 2, (word_count - 2) * sizeof(uint64_t)), 0);
                if constexpr (payload_type == PayloadType::FixedLength) {
                    REQUIRE_EQ(store_a->num_sample_payloads, store_b->num_sample_payloads);
                    const uint64_t *store_a_sample_payloads_ptr = reinterpret_cast<const uint64_t *>(store_a->ptr[1]);
                    const uint64_t *store_b_sample_payloads_ptr = reinterpret_cast<const uint64_t *>(store_b->ptr[1]);
                    const uint32_t num_sample_payload_bytes = (store_a->num_sample_payloads * a.payload_size_ + 7) / 8;
                    REQUIRE_EQ(memcmp(store_a_sample_payloads_ptr, store_b_sample_payloads_ptr, num_sample_payload_bytes), 0);
                }
                wh_iter_skip1(&it_a, check_it_write, check_it_unlock);
                wh_iter_skip1(&it_b, check_it_write, check_it_unlock);
            }
            REQUIRE_EQ(wh_iter_valid(&it_a), wh_iter_valid(&it_b));
            if (it_a.leaf)
                wormleaf_unlock_read(it_a.leaf);
            if (it_b.leaf)
                wormleaf_unlock_read(it_b.leaf);
        }
    }


    template <DivaType diva_type>
    static void CheckDivaPayloads(Diva<diva_type, PayloadType::FixedLength>& s,
                                  std::vector<std::string> keys,
                                  const uint64_t **payloads) {
        const uint32_t infix_store_target_size = Diva<diva_type, PayloadType::FixedLength>::infix_store_target_size;

        uint32_t check_pos = 0;
        const bool write = false, unlock = true;
        const uint8_t *tree_key, *next_tree_key;
        uint32_t tree_key_len, next_tree_key_len, dummy;
        typename Diva<diva_type, PayloadType::FixedLength>::InfixStore *store, *dummy_store;
        wormhole_int_iter it_int;
        wormhole_iter it;
        if constexpr (diva_type == DivaType::Int) {
            it_int.ref = s.better_tree_int_;
            it_int.map = s.better_tree_int_->map;
            it_int.leaf = nullptr;
            it_int.is = 0;
            wh_int_iter_seek(&it_int, nullptr, 0, write);
        }
        else {
            it.ref = s.better_tree_;
            it.map = s.better_tree_->map;
            it.leaf = nullptr;
            it.is = 0;
            wh_iter_seek(&it, nullptr, 0, write);
        }
        const uint8_t zero_key[8] = {}, one_key[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        while (true) {
            if constexpr (diva_type == DivaType::Int) {
                wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key), &tree_key_len, 
                                              reinterpret_cast<void **>(&store), &dummy);
            }
            else {
                wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key), &tree_key_len, 
                                      reinterpret_cast<void **>(&store), &dummy);
            }
            if (memcmp(tree_key, zero_key, tree_key_len) != 0 && memcmp(tree_key, one_key, tree_key_len) != 0) {
                REQUIRE_EQ(memcmp(tree_key, keys[check_pos].c_str(), tree_key_len), 0);
                uint64_t payload[s.payload_size_ / 64 + 2];
                s.GetSamplePayload(*store, 0, payload);
                REQUIRE(compare_bitmap_to_bitmap(payload, 0, payloads[check_pos], 0, s.payload_size_));
                check_pos++;
            }

            if constexpr (diva_type == DivaType::Int) {
                wh_int_iter_skip1(&it_int, write, unlock);
                if (!wh_int_iter_valid(&it_int))
                    break;
                wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_tree_key), &next_tree_key_len, 
                                              reinterpret_cast<void **>(&dummy_store), &dummy);
            }
            else {
                wh_iter_skip1(&it, write, unlock);
                if (!wh_iter_valid(&it))
                    break;
                wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_tree_key), &next_tree_key_len, 
                                      reinterpret_cast<void **>(&dummy_store), &dummy);
            }

            uint64_t infix_list[infix_store_target_size];
            uint64_t payload_list[infix_store_target_size * ((s.payload_size_ + 63) / 64) + 1];
            const uint32_t count = s.GetInfixList(*store, infix_list, payload_list);
            for (uint32_t i = 0; i < count; i++) {
                REQUIRE(compare_bitmap_to_bitmap(payload_list, i * s.payload_size_, payloads[check_pos], 0, s.payload_size_));
                check_pos++;
            }
        }
        if constexpr (diva_type == DivaType::Int) {
            if (it_int.leaf)
                wormleaf_int_unlock_read(it_int.leaf);
        }
        else {
            if (it.leaf)
                wormleaf_unlock_read(it.leaf);
        }
    }


    template <DivaType diva_type, PayloadType payload_type>
    static void PrintStore(const Diva<diva_type, payload_type> &s,
                           const typename Diva<diva_type, payload_type>::InfixStore &store) {
        const uint32_t size_grade = store.GetSizeGrade();
        const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + Diva<diva_type, payload_type>::num_metadata_offset_words;
        const uint64_t *runends = store.ptr + Diva<diva_type, payload_type>::num_metadata_offset_words
                                  + Diva<>::infix_store_target_size / 64;

        std::cerr << " size_grade=" << size_grade << " full_slot_count=" << store.GetFullSlotCount() << std::endl;
        if constexpr (payload_type == PayloadType::FixedLength) {
            std::cerr << "sample_payload(s)=" << std::hex;
            for (uint32_t i = 0; i < store.num_sample_payloads; i++) {
                uint64_t payload[(s.payload_size_ + 63) / 64];
                s.GetSamplePayload(store, i, payload);
                for (uint32_t j = 0; j < ((s.payload_size_ + 63) / 64); j++)
                    std::cerr << "0x" << payload[j] << ", ";
            }
            std::cerr << std::dec << std::endl;
        }
        std::cerr << "popcnts=[" << popcnts[0] << ", " << popcnts[1] << ']' << std::endl;
        std::cerr << "occupieds: ";
        for (int32_t i = 0; i < Diva<>::infix_store_target_size; i++) {
            if ((occupieds[i / 64] >> (i % 64)) & 1ULL)
                std::cerr << i << ", ";
        }
        std::cerr << std::endl << "runends + slots:" << std::endl;
        int32_t cnt = 0;
        for (int32_t i = 0; i < s.scaled_sizes_[size_grade]; i++) {
            const uint64_t value = s.GetSlot(store, i);
            if (value == 0 && !get_bitmap_bit(runends, i))
                continue;
            std::cerr << '{' << std::setfill(' ') << std::setw(3) << i;
            std::cerr << ',' << ((runends[i / 64] >> (i % 64)) & 1ULL) << ",0b";
            for (int32_t j = s.infix_size_ - 1; j >= 0; j--)
                std::cerr << ((value >> (j % 64)) & 1ULL);
            std::cerr << "},   ";
            if (cnt % 8 == 7)
                std::cerr << std::endl;
            cnt++;
        }
        if constexpr (payload_type == PayloadType::FixedLength) {
            std::cerr << std::endl << std::hex;
            for (int32_t i = 0; i < s.scaled_sizes_[size_grade]; i++) {
                const uint64_t value = s.GetSlot(store, i);
                if (value == 0)
                    continue;
                uint64_t payload[(s.payload_size_ + 63) / 64];
                s.GetPayload(store, i, payload);
                std::cerr << '{';
                for (uint32_t j = 0; j < ((s.payload_size_ + 63) / 64); j++)
                    std::cerr << "0x" << payload[j] << ", ";
                std::cerr << "}, ";
            }
        }
        std::cerr << std::dec << std::endl;
    }
};


TEST_SUITE("diva") {
    TEST_CASE("insert") {
        DivaTests::Insert<DivaType::Standard>();
    }

    TEST_CASE("random inserts") {
        DivaTests::RandomInsert<DivaType::Standard>();
    }

    TEST_CASE("point query") {
        DivaTests::PointQuery<DivaType::Standard>();
    }

    TEST_CASE("range query") {
        DivaTests::RangeQuery<DivaType::Standard>();
    }

    TEST_CASE("delete") {
        DivaTests::Delete<DivaType::Standard>();
    }

    TEST_CASE("bulk load") {
        DivaTests::BulkLoad<DivaType::Standard>();
        DivaTests::BulkLoadStreaming<DivaType::Standard>();
    }

    TEST_CASE("serialize and deserialize") {
        DivaTests::SerializeDeserialize<DivaType::Standard>();
    }

    TEST_CASE("concurrency") {
        DivaTests::Concurrency<DivaType::Standard>();
    }

    TEST_CASE("payloads") {
        DivaTests::Payloads<DivaType::Standard>();
    }

    /*
    TEST_CASE("iterator") {
        DivaTests::Iterator<DivaType::Standard>();
    }
    */
}

TEST_SUITE("diva (int optimized)") {
    TEST_CASE("insert") {
        DivaTests::Insert<DivaType::Int>();
    }

    TEST_CASE("random inserts") {
        DivaTests::RandomInsert<DivaType::Int>();
    }

    TEST_CASE("point query") {
        DivaTests::PointQuery<DivaType::Int>();
    }

    TEST_CASE("range query") {
        DivaTests::RangeQuery<DivaType::Int>();
    }

    TEST_CASE("delete") {
        DivaTests::Delete<DivaType::Int>();
    }

    TEST_CASE("bulk load") {
        DivaTests::BulkLoad<DivaType::Int>();
        DivaTests::BulkLoadStreaming<DivaType::Int>();
    }

    TEST_CASE("serialize and deserialize") {
        DivaTests::SerializeDeserialize<DivaType::Int>();
    }

    TEST_CASE("concurrency") {
        DivaTests::Concurrency<DivaType::Int>();
    }

    TEST_CASE("payloads") {
        DivaTests::Payloads<DivaType::Int>();
    }

    /*
    TEST_CASE("iterator") {
        DivaTests::Iterator<DivaType::Int>();
    }
    */
}

TEST_SUITE("diva (binary trie)") {
}

}
