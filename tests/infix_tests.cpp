/**
 * @file infix tests
 * @author ---
 */

#include <bitset>
#include <cstring>
#include <random>
#include <sys/types.h>
#include <utility>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "diva.hpp"
#include "util.hpp"

namespace diva {

typedef Diva<false, PayloadType::None> PayloadDiva;

class InfixTests {
public:
    static void TrieBuild() {
        const uint32_t N = 10;
        const uint32_t slot_size = 5;
        const uint32_t key_start_bit = 6;
        const uint32_t rng_seed = 1380;
        std::mt19937_64 rng(rng_seed);

        SUBCASE("no prefix keys") {
            const uint32_t min_key_len = 6;
            const uint32_t max_key_len = 17;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            const std::vector<uint64_t> expected_trie = {0b1010'00000000000000000000000000110000,
                                                         0b111110000100011001000101110100110011100110001000};
            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                const std::vector<uint64_t> expected_trie_suffixes = {0b0};
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 1);
                AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                const std::vector<uint64_t> expected_trie_suffixes = {0b1000010000111001111010110100001100010010111001000};
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 5);
                AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            const std::vector<uint64_t> expected_trie = {0b1'000000000000001'0000000000001001'00000000000000000000000000111011,
                                                         0b11100101000110111001100011011001100011010011011000100001110};
            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                const std::vector<uint64_t> expected_trie_suffixes = {0b0};
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 1);
                AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                const std::vector<uint64_t> expected_trie_suffixes = {0b11001100101011001100111011101010100};
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 4);
                AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
            }
        }

        SUBCASE("many keys") {
            const uint32_t N = 20;
            const uint32_t min_key_len = 6;
            const uint32_t max_key_len = 17;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            const std::vector<uint64_t> expected_trie = {0b010100'00000000000000000000000001010100,
                                                         0b1110110110000000100101011101101001011011011010011010111010001000,
                                                         0b11110011111000100101};
            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                const std::vector<uint64_t> expected_trie_suffixes = {0b0};
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 1);
                AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                const std::vector<uint64_t> expected_trie_suffixes = {0b1001010100101101000011000100101100011100101001000010100110101000,
                                                                      0b11010101001000010000111001000011110};
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 5);
                AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
            }
        }
    }


    static void TrieQuery() {
        const uint32_t N = 10;
        const uint32_t slot_size = 10;
        const uint32_t key_start_bit = 6;
        const uint32_t rng_seed = 1380;
        std::mt19937_64 rng(rng_seed);

        SUBCASE("no prefix keys") {
            const uint32_t min_key_len = 6;
            const uint32_t max_key_len = 17;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            {
                const uint8_t l_key[20] = {0b00000000, 0b00000000};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000000, 0b00000001};
                const uint32_t r_key_len = 2;
                REQUIRE(infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000010, 0b00000000};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000010, 0b00000000};
                const uint32_t r_key_len = 2;
                REQUIRE(!infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000000, 0b10001010};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000000, 0b10001111};
                const uint32_t r_key_len = 2;
                REQUIRE(infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000000, 0b10101010};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000000, 0b10101110};
                const uint32_t r_key_len = 2;
                REQUIRE(!infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000000, 0b01110010};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000000, 0b01110011};
                const uint32_t r_key_len = 2;
                REQUIRE(!infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            {
                const uint8_t l_key[20] = {0b00000000, 0b11110010};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000000, 0b11110011};
                const uint32_t r_key_len = 2;
                REQUIRE(infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000000, 0b10111011};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000000, 0b10111111};
                const uint32_t r_key_len = 2;
                REQUIRE(infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000001, 0b10111011};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000001, 0b10111111};
                const uint32_t r_key_len = 2;
                REQUIRE(!infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000001, 0b00000000};
                const uint32_t l_key_len = 2;
                const uint8_t r_key[20] = {0b00000001, 0b00000000};
                const uint32_t r_key_len = 2;
                REQUIRE(!infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }
        }

        SUBCASE("many keys") {
            const uint32_t N = 20;
            const uint32_t min_key_len = 6;
            const uint32_t max_key_len = 17;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            {
                const uint8_t l_key[20] = {0b00000000, 0b01111110, 0b11101101, 0b11101101};
                const uint32_t l_key_len = 4;
                const uint8_t r_key[20] = {0b00000000, 0b01111110, 0b11101111};
                const uint32_t r_key_len = 3;
                REQUIRE(infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000000, 0b10000010, 0b01101010};
                const uint32_t l_key_len = 3;
                const uint8_t r_key[20] = {0b00000000, 0b10000010, 0b11101111, 0b01010101};
                const uint32_t r_key_len = 4;
                REQUIRE(!infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000000, 0b10000110, 0b01101010};
                const uint32_t l_key_len = 3;
                const uint8_t r_key[20] = {0b00000000, 0b10000110, 0b11101111, 0b01010101};
                const uint32_t r_key_len = 4;
                REQUIRE(!infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000000, 0b10000011, 0b11101010};
                const uint32_t l_key_len = 3;
                const uint8_t r_key[20] = {0b00000000, 0b10000011, 0b11101111};
                const uint32_t r_key_len = 3;
                REQUIRE(!infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }

            {
                const uint8_t l_key[20] = {0b00000000, 0b10000010, 0b11101010};
                const uint32_t l_key_len = 3;
                const uint8_t r_key[20] = {0b00000000, 0b10000011, 0b11101111};
                const uint32_t r_key_len = 3;
                REQUIRE(infix.QueryTrie({l_key, l_key_len}, {r_key, r_key_len}, key_start_bit, slot_size));
            }
        }
    }


    static void TrieIterate() {
        const uint32_t N = 10;
        const uint32_t slot_size = 5;
        const uint32_t key_start_bit = 6;
        const uint32_t rng_seed = 1380;
        std::mt19937_64 rng(rng_seed);

        SUBCASE("no prefix keys") {
            const uint32_t min_key_len = 6;
            const uint32_t max_key_len = 17;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_->data() + 1);

            SUBCASE("iterate all") {
                const std::vector<int32_t> bit_pos_checks = {0, 6, 8, 9, 13,
                    14, 18, 22, 23, 24, 25, 27, 33, 34, 35, 41, 45, 46, 47};
                const std::vector<int32_t> depth_checks = {-1, 2, 3, 3, 5, 5,
                    7, 9, 9, 7, 2, 3, 6, 6, 3, 6, 8, 8, 6};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11,
                    0b11, 0b10, 0b11, 0b10, 0b11, 0b11, 0b10, 0b10, 0b10, 0b11,
                    0b11, 0b10, 0b10, 0b11, 0b11, 0b10, 0b10};
                const std::vector<int32_t> num_keys_read_checks = {0, 0, 0, 1,
                    1, 2, 2, 2, 3, 4, 5, 5, 5, 6, 7, 7, 7, 8, 9};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

                REQUIRE(it.depth_branch_.back().first == -1);
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }

            SUBCASE("skip subtrees") {
                const std::vector<int32_t> bit_pos_checks = {6, 27, 41};
                const std::vector<int32_t> depth_checks = {2, 3, 6};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11, 0b11};
                const std::vector<int32_t> num_keys_read_checks = {0, 5, 7};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0, 0};

                REQUIRE(it.depth_branch_.back().first == -1);
                it.Advance(infix.HasPrefixKeys());
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.SkipSubtree(infix.HasPrefixKeys());
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_->data() + 1);

            SUBCASE("iterate all") {
                const std::vector<int32_t> bit_pos_checks = {0, 7, 14, 19, 20,
                    25, 30, 31, 32, 37, 42, 43, 44, 45, 50, 56, 57, 58};
                const std::vector<int32_t> depth_checks = {-1, 2, 2, 3, 3, 4,
                    5, 5, 4, 5, 6, 6, 5, 2, 3, 5, 5, 3};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11,
                    0b11, 0b11, 0b10, 0b11, 0b11, 0b10, 0b10, 0b11, 0b11, 0b10,
                    0b10, 0b10, 0b11, 0b11, 0b10, 0b10};
                const std::vector<int32_t> num_keys_read_checks = {0, 0, 0, 0,
                    1, 1, 1, 2, 3, 3, 3, 4, 5, 6, 6, 6, 7, 8};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

                REQUIRE(it.depth_branch_.back().first == -1);
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }

            SUBCASE("skip subtrees") {
                const std::vector<int32_t> bit_pos_checks = {14, 50};
                const std::vector<int32_t> depth_checks = {2, 3};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11};
                const std::vector<int32_t> num_keys_read_checks = {0, 6};
                const std::vector<int32_t> num_prefix_keys_read_checks = {1, 1};

                REQUIRE(it.depth_branch_.back().first == -1);
                it.Advance(infix.HasPrefixKeys());
                it.Advance(infix.HasPrefixKeys());
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.SkipSubtree(infix.HasPrefixKeys());
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }
        }

        SUBCASE("many keys") {
            const uint32_t N = 20;
            const uint32_t min_key_len = 6;
            const uint32_t max_key_len = 17;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_->data() + 1);
            SUBCASE("iterate all") {
                const std::vector<int32_t> bit_pos_checks = {0, 6, 8, 10, 11,
                    12, 14, 16, 17, 21, 22, 23, 25, 26, 28, 29, 31, 35, 36, 37,
                    39, 40, 41, 43, 45, 49, 61, 62, 63, 64, 65, 67, 71, 75, 76,
                    77, 78, 82, 83};
                const std::vector<int32_t> depth_checks = {-1, 2, 3, 4, 4, 3,
                    4, 5, 5, 7, 7, 4, 5, 5, 6, 6, 7, 9, 9, 7, 8, 8, 2, 3, 4, 6,
                    12, 12, 6, 4, 3, 4, 6, 8, 8, 6, 4, 6, 6};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11,
                    0b11, 0b11, 0b10, 0b10, 0b11, 0b11, 0b10, 0b11, 0b10, 0b10,
                    0b11, 0b10, 0b11, 0b10, 0b11, 0b11, 0b10, 0b10, 0b11, 0b10,
                    0b10, 0b11, 0b11, 0b11, 0b11, 0b10, 0b10, 0b10, 0b10, 0b11,
                    0b11, 0b11, 0b10, 0b10, 0b10, 0b11, 0b10};
                const std::vector<int32_t> num_keys_read_checks = {0, 0, 0, 0,
                    1, 2, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 7, 8, 9, 9, 10, 11,
                    11, 11, 11, 11, 12, 13, 14, 15, 15, 15, 15, 16, 17, 18, 18,
                    19};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

                REQUIRE(it.depth_branch_.back().first == -1);
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }

            SUBCASE("skip subtrees") {
                const std::vector<int32_t> bit_pos_checks = {6, 43, 67, 82};
                const std::vector<int32_t> depth_checks = {2, 3, 4, 6};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11, 0b11, 0b11};
                const std::vector<int32_t> num_keys_read_checks = {0, 11, 15, 18};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0, 0, 0};

                REQUIRE(it.depth_branch_.back().first == -1);
                it.Advance(infix.HasPrefixKeys());
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.SkipSubtree(infix.HasPrefixKeys());
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }
        }
    }


    static void TrieSwitchEncoding() {
        const uint32_t N = 10;
        const uint32_t slot_size = 10;
        const uint32_t key_start_bit = 6;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint64_t infix_value = 1;
        const uint32_t rng_seed = 1380;
        std::mt19937_64 rng(rng_seed);

        uint8_t keys_contents[N][max_key_len + 1] = {};
        Diva<>::InfiniteByteString keys[N];
        for (uint32_t i = 0; i < N; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
        }
        std::sort(keys, keys + N);

        Diva<>::Infix expected_no_prefix(infix_value);
        expected_no_prefix.BuildTrie(keys, N, key_start_bit, slot_size);

        Diva<>::Infix expected_prefix(infix_value);
        expected_prefix.BuildTrie(keys, N, key_start_bit, slot_size, true);

        Diva<>::Infix infix(infix_value);

        SUBCASE("no prefix keys to prefix keys") {
            infix.BuildTrie(keys, N, key_start_bit, slot_size);
            infix.SwitchTrieEncoding(true, slot_size);
            const std::vector<uint64_t> expected_trie_suffixes = {0b101000000000011100000000001010100000000111100000000010100,
                                                                  0b1010000000000101000000000011110000000001111100000000101110,
                                                                  0b0};
            AssertTrieContents(infix, *expected_prefix.trie_, expected_trie_suffixes);
        }

        SUBCASE("prefix keys to no prefix keys") {
            infix.BuildTrie(keys, N, key_start_bit, slot_size, true);
            infix.SwitchTrieEncoding(false, slot_size);
            const std::vector<uint64_t> expected_trie_suffixes = {0b10000100001100011000100001000011000100001100010};
            AssertTrieContents(infix, *expected_no_prefix.trie_, expected_trie_suffixes);
        }
    }

    static void TrieInsert() {
        const uint32_t N = 10;
        const uint32_t slot_size = 5;
        const uint32_t key_start_bit = 6;
        const uint32_t rng_seed = 1380;
        std::mt19937_64 rng(rng_seed);

        SUBCASE("no prefix keys") {
            const uint32_t min_key_len = 6;
            const uint32_t max_key_len = 17;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_->data() + 1);

            SUBCASE("iterate all") {
                const std::vector<int32_t> bit_pos_checks = {0, 6, 8, 9, 13,
                    14, 18, 22, 23, 24, 25, 27, 33, 34, 35, 41, 45, 46, 47};
                const std::vector<int32_t> depth_checks = {-1, 2, 3, 3, 5, 5,
                    7, 9, 9, 7, 2, 3, 6, 6, 3, 6, 8, 8, 6};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11,
                    0b11, 0b10, 0b11, 0b10, 0b11, 0b11, 0b10, 0b10, 0b10, 0b11,
                    0b11, 0b10, 0b10, 0b11, 0b11, 0b10, 0b10};
                const std::vector<int32_t> num_keys_read_checks = {0, 0, 0, 1,
                    1, 2, 2, 2, 3, 4, 5, 5, 5, 6, 7, 7, 7, 8, 9};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

                REQUIRE(it.depth_branch_.back().first == -1);
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }

            SUBCASE("skip subtrees") {
                const std::vector<int32_t> bit_pos_checks = {6, 27, 41};
                const std::vector<int32_t> depth_checks = {2, 3, 6};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11, 0b11};
                const std::vector<int32_t> num_keys_read_checks = {0, 5, 7};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0, 0};

                REQUIRE(it.depth_branch_.back().first == -1);
                it.Advance(infix.HasPrefixKeys());
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.SkipSubtree(infix.HasPrefixKeys());
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_->data() + 1);

            SUBCASE("iterate all") {
                const std::vector<int32_t> bit_pos_checks = {0, 7, 14, 19, 20,
                    25, 30, 31, 32, 37, 42, 43, 44, 45, 50, 56, 57, 58};
                const std::vector<int32_t> depth_checks = {-1, 2, 2, 3, 3, 4,
                    5, 5, 4, 5, 6, 6, 5, 2, 3, 5, 5, 3};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11,
                    0b11, 0b11, 0b10, 0b11, 0b11, 0b10, 0b10, 0b11, 0b11, 0b10,
                    0b10, 0b10, 0b11, 0b11, 0b10, 0b10};
                const std::vector<int32_t> num_keys_read_checks = {0, 0, 0, 0,
                    1, 1, 1, 2, 3, 3, 3, 4, 5, 6, 6, 6, 7, 8};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

                REQUIRE(it.depth_branch_.back().first == -1);
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }

            SUBCASE("skip subtrees") {
                const std::vector<int32_t> bit_pos_checks = {14, 50};
                const std::vector<int32_t> depth_checks = {2, 3};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11};
                const std::vector<int32_t> num_keys_read_checks = {0, 6};
                const std::vector<int32_t> num_prefix_keys_read_checks = {1, 1};

                REQUIRE(it.depth_branch_.back().first == -1);
                it.Advance(infix.HasPrefixKeys());
                it.Advance(infix.HasPrefixKeys());
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.SkipSubtree(infix.HasPrefixKeys());
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }
        }

        SUBCASE("many keys") {
            const uint32_t N = 20;
            const uint32_t min_key_len = 6;
            const uint32_t max_key_len = 17;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_->data() + 1);
            SUBCASE("iterate all") {
                const std::vector<int32_t> bit_pos_checks = {0, 6, 8, 10, 11,
                    12, 14, 16, 17, 21, 22, 23, 25, 26, 28, 29, 31, 35, 36, 37,
                    39, 40, 41, 43, 45, 49, 61, 62, 63, 64, 65, 67, 71, 75, 76,
                    77, 78, 82, 83};
                const std::vector<int32_t> depth_checks = {-1, 2, 3, 4, 4, 3,
                    4, 5, 5, 7, 7, 4, 5, 5, 6, 6, 7, 9, 9, 7, 8, 8, 2, 3, 4, 6,
                    12, 12, 6, 4, 3, 4, 6, 8, 8, 6, 4, 6, 6};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11,
                    0b11, 0b11, 0b10, 0b10, 0b11, 0b11, 0b10, 0b11, 0b10, 0b10,
                    0b11, 0b10, 0b11, 0b10, 0b11, 0b11, 0b10, 0b10, 0b11, 0b10,
                    0b10, 0b11, 0b11, 0b11, 0b11, 0b10, 0b10, 0b10, 0b10, 0b11,
                    0b11, 0b11, 0b10, 0b10, 0b10, 0b11, 0b10};
                const std::vector<int32_t> num_keys_read_checks = {0, 0, 0, 0,
                    1, 2, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 7, 8, 9, 9, 10, 11,
                    11, 11, 11, 11, 12, 13, 14, 15, 15, 15, 15, 16, 17, 18, 18,
                    19};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

                REQUIRE(it.depth_branch_.back().first == -1);
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }

            SUBCASE("skip subtrees") {
                const std::vector<int32_t> bit_pos_checks = {6, 43, 67, 82};
                const std::vector<int32_t> depth_checks = {2, 3, 4, 6};
                const std::vector<int32_t> children_mask_checks = {0b11, 0b11, 0b11, 0b11};
                const std::vector<int32_t> num_keys_read_checks = {0, 11, 15, 18};
                const std::vector<int32_t> num_prefix_keys_read_checks = {0, 0, 0, 0};

                REQUIRE(it.depth_branch_.back().first == -1);
                it.Advance(infix.HasPrefixKeys());
                for (uint32_t i = 0; i < bit_pos_checks.size(); i++) {
                    REQUIRE_EQ(it.bit_pos_, bit_pos_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().first, depth_checks[i]);
                    REQUIRE_EQ(it.depth_branch_.back().second, children_mask_checks[i]);
                    REQUIRE_EQ(it.num_keys_read_, num_keys_read_checks[i]);
                    REQUIRE_EQ(it.num_prefix_keys_read_, num_prefix_keys_read_checks[i]);
                    it.SkipSubtree(infix.HasPrefixKeys());
                    it.Advance(infix.HasPrefixKeys());
                }
                REQUIRE(it.depth_branch_.back().first == -1);
            }
        }
    }


private:
    static void AssertTrieContents(const Diva<>::Infix& infix,
                                   const std::vector<uint64_t>& expected_trie,
                                   const std::vector<uint64_t>& expected_trie_suffixes) {
        if (expected_trie.empty())
            REQUIRE_EQ(infix.trie_, nullptr);
        else {
            REQUIRE_EQ(infix.trie_->size(), expected_trie.size());
            REQUIRE_EQ(memcmp(infix.trie_->data(),
                              expected_trie.data(),
                              sizeof(expected_trie[0]) * expected_trie.size()),
                       0);
        }
        if (expected_trie_suffixes.empty())
            REQUIRE_EQ(infix.trie_suffixes_, nullptr);
        else {
            REQUIRE_EQ(infix.trie_suffixes_->size(), expected_trie_suffixes.size());
            REQUIRE_EQ(memcmp(infix.trie_suffixes_->data(),
                              expected_trie_suffixes.data(),
                              sizeof(expected_trie_suffixes[0]) * expected_trie_suffixes.size()),
                       0);
        }
    }


    static void PrintKeys(const Diva<>::InfiniteByteString *keys, uint32_t N) {
        for (uint32_t i = 0; i < N; i++) {
            std::cerr << "key_len=" << keys[i].length << ": ";
            for (uint32_t j = 0; j < keys[i].length; j++) {
                for (int32_t k = 7; k >= 0; k--)
                    std::cerr << ((keys[i].str[j] >> k) & 1);
                std::cerr << ' ';
            }
            std::cerr << std::endl;
        }
    }


    static void PrintTrieAndTrieSuffixes(Diva<>::Infix& infix) {
        std::cerr << "has_prefix_keys=" << infix.HasPrefixKeys() 
                  << " num_prefix_keys=" << infix.GetNumPrefixKeys() 
                  << " num_suffixes=" << infix.GetNumSuffixes() 
                  << " num_trie_bits=" << infix.GetNumTrieBits() << std::endl;
        if (infix.trie_ != nullptr) {
            std::cerr << "trie: ";
            for (uint32_t i = 1; i < infix.trie_->size(); i++) {
                for (uint32_t j = 0; j < 64; j++)
                    std::cerr << (((*infix.trie_)[i] >> j) & 1);
            }
            std::cerr << std::endl;
        }
        if (infix.trie_suffixes_ != nullptr) {
            std::cerr << "trie_suffixes: ";
            for (uint32_t i = 0; i < infix.trie_suffixes_->size(); i++) {
                for (uint32_t j = 0; j < 64; j++)
                    std::cerr << (((*infix.trie_suffixes_)[i] >> j) & 1);
            }
            std::cerr << std::endl;
        }
    }
};

TEST_SUITE("infix") {
    TEST_CASE("build") {
        InfixTests::TrieBuild();
    }

    TEST_CASE("query") {
        InfixTests::TrieQuery();
    }

    TEST_CASE("iterate") {
        InfixTests::TrieIterate();
    }

    TEST_CASE("switch encoding") {
        InfixTests::TrieSwitchEncoding();
    }

    TEST_CASE("insert") {
        InfixTests::TrieInsert();
    }
}

}
