/**
 * @file infix tests
 * @author ---
 */

#include <cstring>
#include <random>
#include <sys/types.h>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <cstdint>
#include <iostream>

#include "diva.hpp"

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
                keys[i] = {keys_contents[i], 8 * key_len};
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

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 0;
                check_infix.num_trie_bits_ = 48;
                check_infix.trie_.push_back(0b111110000100011001000101110100110011100110001000);
                check_infix.num_suffix_bits_ = 10;
                check_infix.num_suffixes_ = 10;
                check_infix.trie_suffixes_.push_back(0b0);
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 1);
                AssertInfix(infix, check_infix);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 0;
                check_infix.num_trie_bits_ = 48;
                check_infix.trie_.push_back(0b111110000100011001000101110100110011100110001000);
                check_infix.num_suffix_bits_ = 50;
                check_infix.num_suffixes_ = 10;
                check_infix.trie_suffixes_.push_back(0b1000010000111001111010110100001100010010111001000);
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 5);
                AssertInfix(infix, check_infix);
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 2;
                check_infix.num_trie_bits_ = 59;
                check_infix.trie_.push_back(0b11100101000110111001100011011001100011010011011000100001110);
                check_infix.num_suffix_bits_ = 9;
                check_infix.num_suffixes_ = 9;
                check_infix.trie_suffixes_.push_back(0b0);
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 1);
                AssertInfix(infix, check_infix);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 2;
                check_infix.num_trie_bits_ = 59;
                check_infix.trie_.push_back(0b11100101000110111001100011011001100011010011011000100001110);
                check_infix.num_suffix_bits_ = 36;
                check_infix.num_suffixes_ = 9;
                check_infix.trie_suffixes_.push_back(0b11001100101011001100111011101010100);
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 4);
                AssertInfix(infix, check_infix);
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 0;
                check_infix.num_trie_bits_ = 84;
                check_infix.trie_.push_back(0b1110110110000000100101011101101001011011011010011010111010001000);
                check_infix.trie_.push_back(0b11110011111000100101);
                check_infix.num_suffix_bits_ = 20;
                check_infix.num_suffixes_ = 20;
                check_infix.trie_suffixes_.push_back(0b0);
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 1);
                AssertInfix(infix, check_infix);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 0;
                check_infix.num_trie_bits_ = 84;
                check_infix.trie_.push_back(0b1110110110000000100101011101101001011011011010011010111010001000);
                check_infix.trie_.push_back(0b11110011111000100101);
                check_infix.num_suffix_bits_ = 100;
                check_infix.num_suffixes_ = 20;
                check_infix.trie_suffixes_.push_back(0b1001010100101101000011000100101100011100101001000010100110101000);
                check_infix.trie_suffixes_.push_back(0b11010101001000010000111001000011110);
                REQUIRE_EQ(infix.GetActualSuffixLen(slot_size), 5);
                AssertInfix(infix, check_infix);
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
                keys[i] = {keys_contents[i], 8 * key_len};
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
                keys[i] = {keys_contents[i], 8 * key_len};
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
                keys[i] = {keys_contents[i], 8 * key_len};
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_.data());

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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_.data());

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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            Diva<>::Infix::TrieIterator it(infix.trie_.data());
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
            keys[i] = {keys_contents[i], 8 * key_len};
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

            Diva<>::Infix check_infix(expected_prefix);
            check_infix.trie_suffixes_.clear();
            check_infix.trie_suffixes_.push_back(0b101000000000010110000000001110000000000101110000000010100);
            check_infix.trie_suffixes_.push_back(0b1010000000000101000000000010111000000001111100000000111010);
            check_infix.trie_suffixes_.push_back(0b0);
            check_infix.num_suffix_bits_ = 130;
            AssertInfix(infix, check_infix);
        }

        SUBCASE("prefix keys to no prefix keys") {
            infix.BuildTrie(keys, N, key_start_bit, slot_size, true);
            infix.SwitchTrieEncoding(false, slot_size);

            Diva<>::Infix check_infix(expected_no_prefix);
            check_infix.trie_suffixes_.clear();
            check_infix.trie_suffixes_.push_back(0b10000100001100011000100001000011000100001100010);
            check_infix.num_suffix_bits_ = 50;
            AssertInfix(infix, check_infix);
        }
    }


    static void TrieInsert() {
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            SUBCASE("path diverging to the right") {
                SUBCASE("first part zero") {
                    uint8_t insertee_contents[8] = {0b00000000, 0b10100110, 0b00000000};
                    infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                    Diva<>::Infix check_infix(infix_value);
                    check_infix.num_prefix_keys_ = 0;
                    check_infix.num_trie_bits_ = 49;
                    check_infix.trie_.push_back(0b1111100001000111010010101110100110011100110001000);
                    check_infix.num_suffix_bits_ = 55;
                    check_infix.num_suffixes_ = 11;
                    check_infix.trie_suffixes_.push_back(0b100001000011100100101111010110100001100010010111001000);
                    AssertInfix(infix, check_infix);
                }

                SUBCASE("second part zero") {
                    uint8_t insertee_contents[8] = {0b00000000, 0b10010110, 0b00000000};
                    infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                    Diva<>::Infix check_infix(infix_value);
                    check_infix.num_prefix_keys_ = 0;
                    check_infix.num_trie_bits_ = 49;
                    check_infix.trie_.push_back(0b1111100001000111100100101110100110011100110001000);
                    check_infix.num_suffix_bits_ = 55;
                    check_infix.num_suffixes_ = 11;
                    check_infix.trie_suffixes_.push_back(0b100001000011100101101111010110100001100010010111001000);
                    AssertInfix(infix, check_infix);
                }
            }

            SUBCASE("path diverging to the left") {
                uint8_t insertee_contents[8] = {0b00000000, 0b01110010, 0b10000000};
                infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 0;
                check_infix.num_trie_bits_ = 49;
                check_infix.trie_.push_back(0b1111100001000110010001011101001011011100110001000);
                check_infix.num_suffix_bits_ = 55;
                check_infix.num_suffixes_ = 11;
                check_infix.trie_suffixes_.push_back(0b100001000011100111101011010000110001001010100111001000);
                AssertInfix(infix, check_infix);
            }

            SUBCASE("path diverging from suffix from left") {
                uint8_t insertee_contents[8] = {0b00000000, 0b10000010, 0b10101010};
                infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 0;
                check_infix.num_trie_bits_ = 55;
                check_infix.trie_.push_back(0b1111100001000111011000001000101110100110011100110001000);
                check_infix.num_suffix_bits_ = 55;
                check_infix.num_suffixes_ = 11;
                check_infix.trie_suffixes_.push_back(0b100001000011100111100001011010100001100010010111001000);
                AssertInfix(infix, check_infix);
            }

            SUBCASE("path diverging from suffix from right") {
                uint8_t insertee_contents[8] = {0b00000000, 0b11000101, 0b11010101};
                infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 0;
                check_infix.num_trie_bits_ = 55;
                check_infix.trie_.push_back(0b1111111000110000100011001000101110100110011100110001000);
                check_infix.num_suffix_bits_ = 55;
                check_infix.num_suffixes_ = 11;
                check_infix.trie_suffixes_.push_back(0b100001000010100000101111010110100001100010010111001000);
                AssertInfix(infix, check_infix);
            }

            SUBCASE("create prefix key") {
                uint8_t insertee_contents[8] = {0b00000000, 0b10000011, 0b10101010};
                infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 2;
                check_infix.num_trie_bits_ = 80;
                check_infix.trie_.push_back(0b1110000100111001000011100011011100101010101011010101001100001110);
                check_infix.trie_.push_back(0b1111010100001110);
                check_infix.num_suffix_bits_ = 120;
                check_infix.num_suffixes_ = 10;
                check_infix.trie_suffixes_.push_back(0b101000000000010110000000001110000000000101110000000010100);
                check_infix.trie_suffixes_.push_back(0b101000000000010100000000001011100000000111110100);
                check_infix.trie_suffixes_.push_back(0b0);
                AssertInfix(infix, check_infix);
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            SUBCASE("path diverging") {
                uint8_t insertee_contents[8] = {0b00000000, 0b10111111, 0b11111111};
                infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 2;
                check_infix.num_trie_bits_ = 64;
                check_infix.trie_.push_back(0b1111001100011000110111001100011011001100011010011011000100001110);
                check_infix.trie_.push_back(0b0);
                check_infix.num_suffix_bits_ = 40;
                check_infix.num_suffixes_ = 10;
                check_infix.trie_suffixes_.push_back(0b110011101100101011001100111011101010100);
                AssertInfix(infix, check_infix);
            }

            SUBCASE("path diverging from suffix from left") {
                uint8_t insertee_contents[8] = {0b00000000, 0b01100101, 0b00000000};
                infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 2;
                check_infix.num_trie_bits_ = 66;
                check_infix.trie_.push_back(0b1001010001101111101010001100011011001100011010011011000100001110);
                check_infix.trie_.push_back(0b11);
                check_infix.num_suffix_bits_ = 40;
                check_infix.num_suffixes_ = 10;
                check_infix.trie_suffixes_.push_back(0b110011001010110011000010110011101010100);
                AssertInfix(infix, check_infix);
            }

            SUBCASE("path diverging from suffix from right") {
                uint8_t insertee_contents[8] = {0b00000000, 0b10001101, 0b00000000};
                infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 2;
                check_infix.num_trie_bits_ = 65;
                check_infix.trie_.push_back(0b1110011000101000110111001100011011001100011010011011000100001110);
                check_infix.trie_.push_back(0b1);
                check_infix.num_suffix_bits_ = 40;
                check_infix.num_suffixes_ = 10;
                check_infix.trie_suffixes_.push_back(0b110011001100011011001100111011101010100);
                AssertInfix(infix, check_infix);
            }

            SUBCASE("create new prefix key") {
                uint8_t insertee_contents[8] = {0b00000000, 0b10011001, 0b10110110};
                infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.num_prefix_keys_ = 3;
                check_infix.num_trie_bits_ = 73;
                check_infix.trie_.push_back(0b1001110100101000110111001100011011001100011010011011000100001110);
                check_infix.trie_.push_back(0b110100010);
                check_infix.num_suffix_bits_ = 36;
                check_infix.num_suffixes_ = 9;
                check_infix.trie_suffixes_.push_back(0b11001110101011001100111011101010100);
                AssertInfix(infix, check_infix);

                SUBCASE("create new prefix key child") {
                    uint8_t insertee_contents[8] = {0b00000000, 0b10011011, 0b11111111};
                    infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

                    Diva<>::Infix check_infix(infix_value);
                    check_infix.num_prefix_keys_ = 3;
                    check_infix.num_trie_bits_ = 74;
                    check_infix.trie_.push_back(0b1001110100101000110111001100011011001100011010011011000100001110);
                    check_infix.trie_.push_back(0b1111100010);
                    check_infix.num_suffix_bits_ = 40;
                    check_infix.num_suffixes_ = 10;
                    check_infix.trie_suffixes_.push_back(0b110011101110101011001100111011101010100);
                    AssertInfix(infix, check_infix);
                }
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            uint8_t insertee_contents[8] = {0b00000000, 0b10000010, 0b00000000};
            infix.InsertTrie({insertee_contents, 3}, key_start_bit, slot_size);

            Diva<>::Infix check_infix(infix_value);
            check_infix.num_prefix_keys_ = 0;
            check_infix.num_trie_bits_ = 85;
            check_infix.trie_.push_back(0b1101100010110000100101011101101001011011011010011010111010001000);
            check_infix.trie_.push_back(0b111100111110001001011);
            check_infix.num_suffix_bits_ = 105;
            check_infix.num_suffixes_ = 21;
            check_infix.trie_suffixes_.push_back(0b1010010000101101000011000100101100011100101001000010100110101000);
            check_infix.trie_suffixes_.push_back(0b1101010100100001000011100100001111010010);
            AssertInfix(infix, check_infix);
        }
    }


    static void TrieGetStrings() {
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                const auto [recovered_keys, recovered_key_contents] = infix.GetStrings(slot_size);

                std::vector<uint32_t> expected_key_bit_lens = {4,
                                                               6,
                                                               10,
                                                               10,
                                                               8,
                                                               7,
                                                               7,
                                                               9,
                                                               9,
                                                               7};
                std::vector<uint8_t> expected_key_contents = {0b00000000,
                                                              0b00011000,
                                                              0b00011110, 0b00000000,
                                                              0b00011110, 0b01000000,
                                                              0b00011111,
                                                              0b00100000,
                                                              0b00100010,
                                                              0b00110001, 0b00000000,
                                                              0b00110001, 0b10000000,
                                                              0b00110010};
                AssertRecoveredStrings(recovered_keys, recovered_key_contents,
                                       expected_key_bit_lens, expected_key_contents);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                const auto [recovered_keys, recovered_key_contents] = infix.GetStrings(slot_size);
                std::vector<uint32_t> expected_key_bit_lens = {7,
                                                               9,
                                                               13,
                                                               13,
                                                               11,
                                                               10,
                                                               10,
                                                               12,
                                                               12,
                                                               10};
                std::vector<uint8_t> expected_key_contents = {0b00000000,
                                                              0b00011011, 0b00000000,
                                                              0b00011110, 0b00001000,
                                                              0b00011110, 0b01100000,
                                                              0b00011111, 0b00000000,
                                                              0b00100000, 0b11000000,
                                                              0b00100011, 0b11000000,
                                                              0b00110001, 0b01100000,
                                                              0b00110001, 0b10000000,
                                                              0b00110010, 0b00000000};
                AssertRecoveredStrings(recovered_keys, recovered_key_contents,
                                       expected_key_bit_lens, expected_key_contents);
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                const auto [recovered_keys, recovered_key_contents] = infix.GetStrings(slot_size);
                std::vector<uint32_t> expected_key_bit_lens = {2,
                                                               4,
                                                               6,
                                                               6,
                                                               7,
                                                               7,
                                                               6,
                                                               6,
                                                               6,
                                                               4};
                std::vector<uint8_t> expected_key_contents = {0b00000000,
                                                              0b00000000,
                                                              0b00010000,
                                                              0b00010100,
                                                              0b00011000,
                                                              0b00011010,
                                                              0b00011100,
                                                              0b00100000,
                                                              0b00100100,
                                                              0b00110000};
                AssertRecoveredStrings(recovered_keys, recovered_key_contents,
                                       expected_key_bit_lens, expected_key_contents);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                const auto [recovered_keys, recovered_key_contents] = infix.GetStrings(slot_size);
                std::vector<uint32_t> expected_key_bit_lens = {2,
                                                               6,
                                                               8,
                                                               8,
                                                               9,
                                                               9,
                                                               8,
                                                               8,
                                                               8,
                                                               6};
                std::vector<uint8_t> expected_key_contents = {0b00000000,
                                                              0b00000000,
                                                              0b00010001,
                                                              0b00010111,
                                                              0b00011001, 0b10000000,
                                                              0b00011011, 0b00000000,
                                                              0b00011110,
                                                              0b00100001,
                                                              0b00100110,
                                                              0b00111000};
                AssertRecoveredStrings(recovered_keys, recovered_key_contents,
                                       expected_key_bit_lens, expected_key_contents);
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                const auto [recovered_keys, recovered_key_contents] = infix.GetStrings(slot_size);
                std::vector<uint32_t> expected_key_bit_lens = {5,
                                                               5,
                                                               6,
                                                               8,
                                                               8,
                                                               6,
                                                               7,
                                                               10,
                                                               10,
                                                               9,
                                                               9,
                                                               13,
                                                               13,
                                                               7,
                                                               5,
                                                               9,
                                                               9,
                                                               7,
                                                               7,
                                                               7};
                std::vector<uint8_t> expected_key_contents = {0b00000000,
                                                              0b00001000,
                                                              0b00010000,
                                                              0b00010100,
                                                              0b00010101,
                                                              0b00011000,
                                                              0b00011100,
                                                              0b00011110, 0b00000000,
                                                              0b00011110, 0b01000000,
                                                              0b00011111, 0b00000000,
                                                              0b00011111, 0b10000000,
                                                              0b00100000, 0b11010000,
                                                              0b00100000, 0b11011000,
                                                              0b00100010,
                                                              0b00101000,
                                                              0b00110001, 0b00000000,
                                                              0b00110001, 0b10000000,
                                                              0b00110010,
                                                              0b00111100,
                                                              0b00111110};
                AssertRecoveredStrings(recovered_keys, recovered_key_contents,
                                       expected_key_bit_lens, expected_key_contents);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                const auto [recovered_keys, recovered_key_contents] = infix.GetStrings(slot_size);
                std::vector<uint32_t> expected_key_bit_lens = {8,
                                                               8,
                                                               9,
                                                               11,
                                                               11,
                                                               9,
                                                               10,
                                                               13,
                                                               13,
                                                               12,
                                                               12,
                                                               16,
                                                               16,
                                                               10,
                                                               8,
                                                               12,
                                                               12,
                                                               10,
                                                               10,
                                                               10};
                std::vector<uint8_t> expected_key_contents = {0b00000000,
                                                              0b00001101,
                                                              0b00010001, 0b00000000,
                                                              0b00010100, 0b00000000,
                                                              0b00010101, 0b01000000,
                                                              0b00011011, 0b00000000,
                                                              0b00011101, 0b00000000,
                                                              0b00011110, 0b00001000,
                                                              0b00011110, 0b01100000,
                                                              0b00011111, 0b00000000,
                                                              0b00011111, 0b10110000,
                                                              0b00100000, 0b11010010,
                                                              0b00100000, 0b11011001,
                                                              0b00100011, 0b11000000,
                                                              0b00101000,
                                                              0b00110001, 0b01100000,
                                                              0b00110001, 0b10000000,
                                                              0b00110010, 0b00000000,
                                                              0b00111100, 0b10000000,
                                                              0b00111111, 0b01000000};
                AssertRecoveredStrings(recovered_keys, recovered_key_contents,
                                       expected_key_bit_lens, expected_key_contents);
            }
        }
    }


    static void TrieDelete() {
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            const uint32_t victim = 3;
            Diva<>::InfiniteByteString baseline_keys[N - 1];
            for (uint32_t i = 0; i < N; i++) {
                if (i == victim)
                    continue;
                baseline_keys[i - (i > victim)] = keys[i];
            }

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                infix.DeleteTrie({keys[victim].str, keys[victim].length / 8}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(baseline_keys, N - 1, key_start_bit, slot_size);
                check_infix.num_suffix_bits_ = 14;
                check_infix.trie_suffixes_[0] = 0b100100;
                AssertInfix(infix, check_infix);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                infix.DeleteTrie({keys[victim].str, keys[victim].length / 8}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(baseline_keys, N - 1, key_start_bit, slot_size);
                check_infix.num_suffix_bits_ = 55;
                check_infix.trie_suffixes_[0] = 0b100001000011100111101011010000000000011100000111001000;
                AssertInfix(infix, check_infix);
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("delete prefix key") {
                const uint32_t victim = 0;
                Diva<>::InfiniteByteString baseline_keys[N - 1];
                for (uint32_t i = 0; i < N; i++) {
                    if (i == victim)
                        continue;
                    baseline_keys[i - (i > victim)] = keys[i];
                }
                uint8_t victim_key[1] = {0b00001111};

                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                infix.DeleteTrie({victim_key, 1}, 0, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(baseline_keys, N - 1, key_start_bit, slot_size);
                check_infix.trie_suffixes_[0] = 0b110000110000101000110000110000111000111000101000100;
                AssertInfix(infix, check_infix);
            }

            SUBCASE("delete non-prefix key") {
                const uint32_t victim = 4;
                Diva<>::InfiniteByteString baseline_keys[N - 1];
                for (uint32_t i = 0; i < N; i++) {
                    if (i == victim)
                        continue;
                    baseline_keys[i - (i > victim)] = keys[i];
                }

                SUBCASE("small slots") {
                    infix.BuildTrie(keys, N, key_start_bit, slot_size);
                    infix.DeleteTrie({keys[victim].str, keys[victim].length / 8}, key_start_bit, slot_size);

                    Diva<>::Infix check_infix(infix_value);
                    check_infix.BuildTrie(baseline_keys, N - 1, key_start_bit, slot_size);
                    check_infix.num_suffix_bits_ = 13;
                    check_infix.trie_suffixes_[0] = 0b111000;
                    AssertInfix(infix, check_infix);
                }
                SUBCASE("wide slots") {
                    const uint32_t slot_size = 10;
                    infix.BuildTrie(keys, N, key_start_bit, slot_size);
                    infix.DeleteTrie({keys[victim].str, keys[victim].length / 8}, key_start_bit, slot_size);

                    Diva<>::Infix check_infix(infix_value);
                    check_infix.BuildTrie(baseline_keys, N - 1, key_start_bit, slot_size);
                    check_infix.num_suffix_bits_ = 42;
                    check_infix.trie_suffixes_[0] = 0b11001100101011000000000011110011101010100;
                    AssertInfix(infix, check_infix);
                }
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            const uint32_t victim = 2;
            Diva<>::InfiniteByteString baseline_keys[N - 1];
            for (uint32_t i = 0; i < N; i++) {
                if (i == victim)
                    continue;
                baseline_keys[i - (i > victim)] = keys[i];
            }

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                infix.DeleteTrie({keys[victim].str, keys[victim].length / 8}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(baseline_keys, N - 1, key_start_bit, slot_size);
                AssertInfix(infix, check_infix);
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);
                infix.DeleteTrie({keys[victim].str, keys[victim].length / 8}, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(baseline_keys, N - 1, key_start_bit, slot_size);
                AssertInfix(infix, check_infix);
            }
        }
    }


    static void TrieGetLongestMatch() {
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    const uint32_t query_key_len = 2;
                    uint8_t query_key_contents[query_key_len] = {0b00000000, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 4);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00011110, 0b00110001, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 10);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00011110, 0b10110001, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), -1);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    const uint32_t query_key_len = 2;
                    uint8_t query_key_contents[query_key_len] = {0b00000000, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 7);
                }
                {
                    const uint32_t query_key_len = 2;
                    uint8_t query_key_contents[query_key_len] = {0b00000111, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), -1);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00100011, 0b11010010, 0b10101101};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 10);
                }
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    const uint32_t query_key_len = 2;
                    uint8_t query_key_contents[query_key_len] = {0b00000000, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 4);
                }
                {
                    const uint32_t query_key_len = 2;
                    uint8_t query_key_contents[query_key_len] = {0b00101110, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 2);
                }
                {
                    const uint32_t query_key_len = 2;
                    uint8_t query_key_contents[query_key_len] = {0b11111110, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), -1);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00100101, 0b10101011, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 6);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    const uint32_t query_key_len = 2;
                    uint8_t query_key_contents[query_key_len] = {0b00000000, 0b00000000};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 6);
                }
                {
                    const uint32_t query_key_len = 2;
                    uint8_t query_key_contents[query_key_len] = {0b00011111, 0b11111110};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 2);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00100001, 0b00101001, 0b11011011};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 8);
                }
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00100000, 0b11011001, 0b11101010};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 13);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00100000, 0b10101001, 0b11101010};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), -1);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00111111, 0b01101001, 0b11101010};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 7);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00100000, 0b11011001, 0b11101010};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 16);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00100000, 0b10101001, 0b11101010};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), -1);
                }
                {
                    const uint32_t query_key_len = 3;
                    uint8_t query_key_contents[query_key_len] = {0b00111111, 0b01101001, 0b11101010};
                    REQUIRE_EQ(infix.GetLongestMatch({query_key_contents, query_key_len}, 0, slot_size), 10);
                }
            }
        }
    }


    static void TrieAdapt() {
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(keys, N, key_start_bit, slot_size);
                {
                    const uint32_t adapt_key_len_bits = 16;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00000000, 0b10101010};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 30;
                    check_infix.trie_suffixes_[0] = 0b11101011010100001;
                    AssertInfix(infix, check_infix);
                }
                {
                    const uint32_t adapt_key_len_bits = 15;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00100000, 0b11011110};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 45;
                    check_infix.trie_suffixes_[0] = 0b1111111011010000000011101011010100001;
                    AssertInfix(infix, check_infix);
                }
                {
                    const uint32_t adapt_key_len_bits = 17;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00000000, 0b10101010, 0b10000000};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 45;
                    check_infix.trie_suffixes_[0] = 0b1111111011010000000111101011010100001;
                    AssertInfix(infix, check_infix);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(keys, N, key_start_bit, slot_size);
                {
                    const uint32_t adapt_key_len_bits = 16;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00000000, 0b10101010};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 60;
                    check_infix.trie_suffixes_[0] = 0b10000100001110011110101101000011000100101110011010101010000;
                    AssertInfix(infix, check_infix);
                }
                {
                    const uint32_t adapt_key_len_bits = 15;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00100000, 0b11011110};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 70;
                    check_infix.trie_suffixes_[0] = 0b1000011100111100000111111011001000011000100101110011010101010000;
                    check_infix.trie_suffixes_.push_back(0b10000);
                    AssertInfix(infix, check_infix);
                }
                {
                    const uint32_t adapt_key_len_bits = 17;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00000000, 0b10101010, 0b10000000};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 80;
                    check_infix.trie_suffixes_[0] = 0b1111000001111110110010000110001001011100000000001110101010110000;
                    check_infix.trie_suffixes_[1] = 0b100001000011100;
                    AssertInfix(infix, check_infix);
                }
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(keys, N, key_start_bit, slot_size);
                {
                    const uint32_t adapt_key_len_bits = 16;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00000000, 0b11111111};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 29;
                    check_infix.trie_suffixes_[0] = 0b11111111111100001;
                    AssertInfix(infix, check_infix);
                }
                {
                    const uint32_t adapt_key_len_bits = 15;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00101010, 0b11011110};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_prefix_keys_ = 0;
                    check_infix.num_trie_bits_ = 32;
                    check_infix.trie_[0] = 0b11111010101111010111010110001000;
                    check_infix.num_suffixes_ = 10;
                    check_infix.num_suffix_bits_ = 45;
                    check_infix.trie_suffixes_[0] = 0b111110111010110000000000001111111111100001;
                    AssertInfix(infix, check_infix);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(keys, N, key_start_bit, slot_size);
                {
                    const uint32_t adapt_key_len_bits = 16;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00000000, 0b11111111};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 56;
                    check_infix.trie_suffixes_[0] = 0b1100110010101100110011101110101000000000110111111111000;
                    AssertInfix(infix, check_infix);
                }
                {
                    const uint32_t adapt_key_len_bits = 15;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00101010, 0b11011110};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_prefix_keys_ = 0;
                    check_infix.num_trie_bits_ = 32;
                    check_infix.trie_[0] = 0b11111010101111010111010110001000;
                    check_infix.num_suffixes_ = 10;
                    check_infix.num_suffix_bits_ = 80;
                    check_infix.trie_suffixes_[0] = 0b1010110001100001010001100001100001110001110001010011111111100001;
                    check_infix.trie_suffixes_.push_back(0b1100000101111);
                    AssertInfix(infix, check_infix);
                }
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(keys, N, key_start_bit, slot_size);
                {
                    const uint32_t adapt_key_len_bits = 15;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00010001, 0b01101010};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 35;
                    check_infix.trie_suffixes_[0] = 0b111101010101100;
                    AssertInfix(infix, check_infix);
                }
                {
                    const uint32_t adapt_key_len_bits = 24;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00001101, 0b10101010, 0b11111111};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 60;
                    check_infix.trie_suffixes_[0] = 0b1111010101011011111111110101101011101110;
                    AssertInfix(infix, check_infix);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                Diva<>::Infix check_infix(infix_value);
                check_infix.BuildTrie(keys, N, key_start_bit, slot_size);
                {
                    const uint32_t adapt_key_len_bits = 15;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00010001, 0b01101010};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 110;
                    check_infix.trie_suffixes_[0] = 0b1011010000110001001011000111001010010000000110101101010110101000;
                    check_infix.trie_suffixes_[1] = 0b110101010010000100001110010000111101001010100;
                    AssertInfix(infix, check_infix);
                }
                {
                    const uint32_t adapt_key_len_bits = 24;
                    const uint32_t adapt_key_len = (adapt_key_len_bits + 7) / 8;
                    uint8_t adapt_key_contents[adapt_key_len] = {0b00001101, 0b10101010, 0b11111111};
                    infix.AdaptTrie({adapt_key_contents, adapt_key_len}, 0, adapt_key_len_bits, slot_size);

                    check_infix.num_suffix_bits_ = 130;
                    check_infix.trie_suffixes_[0] = 0b1100011100101001000000011010110101000111111110101010111101101000;
                    check_infix.trie_suffixes_[1] = 0b1010101001000010000111001000011110100101010010110100001100010010;
                    check_infix.trie_suffixes_.push_back(0b1);
                    AssertInfix(infix, check_infix);
                }
            }
        }
    }


    static void TrieSplit() {
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    auto split_infixes = infix.SplitPrefixBits(3, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 19;
                    check_infixes.back().trie_.push_back(0b1110100110011100110);
                    check_infixes.back().num_suffixes_ = 5;
                    check_infixes.back().num_suffix_bits_ = 5;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 23;
                    check_infixes.back().trie_.push_back(0b11111000010001100100010);
                    check_infixes.back().num_suffixes_ = 5;
                    check_infixes.back().num_suffix_bits_ = 5;
                    check_infixes.back().trie_suffixes_.push_back(0b0);
                    
                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(4, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 16;
                    check_infixes.back().trie_.push_back(0b1110100110011100);
                    check_infixes.back().num_suffixes_ = 4;
                    check_infixes.back().num_suffix_bits_ = 4;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    check_infixes.emplace_back(0b101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 8;
                    check_infixes.back().trie_.push_back(0b11001000);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 2;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    check_infixes.emplace_back(0b111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 13;
                    check_infixes.back().trie_.push_back(0b1111100001000);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 3;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(5, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b10);

                    check_infixes.emplace_back(0b111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 14;
                    check_infixes.back().trie_.push_back(0b11101001100110);
                    check_infixes.back().num_suffixes_ = 4;
                    check_infixes.back().num_suffix_bits_ = 4;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    check_infixes.emplace_back(0b1001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 6;
                    check_infixes.back().trie_.push_back(0b110100);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 4;
                    check_infixes.back().trie_suffixes_.push_back(0b101);

                    check_infixes.emplace_back(0b1101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 11;
                    check_infixes.back().trie_.push_back(0b11111000100);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 3;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    auto split_infixes = infix.SplitPrefixBits(3, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 19;
                    check_infixes.back().trie_.push_back(0b1110100110011100110);
                    check_infixes.back().num_suffixes_ = 5;
                    check_infixes.back().num_suffix_bits_ = 30;
                    check_infixes.back().trie_suffixes_.push_back(0b1000001100001001001110001000);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 23;
                    check_infixes.back().trie_.push_back(0b11111000010001100100010);
                    check_infixes.back().num_suffixes_ = 5;
                    check_infixes.back().num_suffix_bits_ = 25;
                    check_infixes.back().trie_suffixes_.push_back(0b100001000011100111101011);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(4, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b1000);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 16;
                    check_infixes.back().trie_.push_back(0b1110100110011100);
                    check_infixes.back().num_suffixes_ = 4;
                    check_infixes.back().num_suffix_bits_ = 24;
                    check_infixes.back().trie_suffixes_.push_back(0b1000001100001001001110);

                    check_infixes.emplace_back(0b101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 8;
                    check_infixes.back().trie_.push_back(0b11001000);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 12;
                    check_infixes.back().trie_suffixes_.push_back(0b1111001011);

                    check_infixes.emplace_back(0b111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 13;
                    check_infixes.back().trie_.push_back(0b1111100001000);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 15;
                    check_infixes.back().trie_suffixes_.push_back(0b10000100001110);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(5, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b100);

                    check_infixes.emplace_back(0b111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 14;
                    check_infixes.back().trie_.push_back(0b11101001100110);
                    check_infixes.back().num_suffixes_ = 4;
                    check_infixes.back().num_suffix_bits_ = 24;
                    check_infixes.back().trie_suffixes_.push_back(0b1000001100001001001110);

                    check_infixes.emplace_back(0b1001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 6;
                    check_infixes.back().trie_.push_back(0b110100);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 14;
                    check_infixes.back().trie_suffixes_.push_back(0b11110001011);

                    check_infixes.emplace_back(0b1101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 11;
                    check_infixes.back().trie_.push_back(0b11111000100);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 18;
                    check_infixes.back().trie_suffixes_.push_back(0b1000001000001110);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
            }
        }

        SUBCASE("prefix keys") {
            const uint32_t min_key_len = 1;
            const uint32_t max_key_len = 10;

            uint8_t keys_contents[N][max_key_len + 1] = {};
            Diva<>::InfiniteByteString keys[N];
            for (uint32_t i = 0; i < N; i++) {
                const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    auto split_infixes = infix.SplitPrefixBits(1, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 2;
                    check_infixes.back().num_trie_bits_ = 58;
                    check_infixes.back().trie_.push_back(0b1110010100011011100110001101100110001101001101100010001010);
                    check_infixes.back().num_suffixes_ = 9;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(2, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 2;
                    check_infixes.back().num_trie_bits_ = 57;
                    check_infixes.back().trie_.push_back(0b111001010001101110011000110110011000110100110110001000110);
                    check_infixes.back().num_suffixes_ = 9;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(3, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b10);

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 16;
                    check_infixes.back().trie_.push_back(0b1111010111010110);
                    check_infixes.back().num_suffixes_ = 6;
                    check_infixes.back().num_suffix_bits_ = 12;
                    check_infixes.back().trie_suffixes_.push_back(0b10101010101);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 9;
                    check_infixes.back().trie_.push_back(0b111010010);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 6;
                    check_infixes.back().trie_suffixes_.push_back(0b10101);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(4, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b100);

                    check_infixes.emplace_back(0b1);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 13;
                    check_infixes.back().trie_.push_back(0b1111010111010);
                    check_infixes.back().num_suffixes_ = 5;
                    check_infixes.back().num_suffix_bits_ = 10;
                    check_infixes.back().trie_suffixes_.push_back(0b101010101);

                    check_infixes.emplace_back(0b101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 6;
                    check_infixes.back().trie_.push_back(0b110100);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 4;
                    check_infixes.back().trie_suffixes_.push_back(0b101);

                    check_infixes.emplace_back(0b111);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(5, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1000);

                    check_infixes.emplace_back(0b10);

                    check_infixes.emplace_back(0b101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 4;
                    check_infixes.back().trie_.push_back(0b1110);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 6;
                    check_infixes.back().trie_suffixes_.push_back(0b1001);

                    check_infixes.emplace_back(0b111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 7;
                    check_infixes.back().trie_.push_back(0b1111010);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 6;
                    check_infixes.back().trie_suffixes_.push_back(0b10101);

                    check_infixes.emplace_back(0b1001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 4;
                    check_infixes.back().trie_.push_back(0b1110);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 6;
                    check_infixes.back().trie_suffixes_.push_back(0b1001);

                    check_infixes.emplace_back(0b1110);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    auto split_infixes = infix.SplitPrefixBits(1, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 2;
                    check_infixes.back().num_trie_bits_ = 58;
                    check_infixes.back().trie_.push_back(0b1110010100011011100110001101100110001101001101100010001010);
                    check_infixes.back().num_suffixes_ = 9;
                    check_infixes.back().num_suffix_bits_ = 36;
                    check_infixes.back().trie_suffixes_.push_back(0b11001100101011001100111011101010100);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(2, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 2;
                    check_infixes.back().num_trie_bits_ = 57;
                    check_infixes.back().trie_.push_back(0b111001010001101110011000110110011000110100110110001000110);
                    check_infixes.back().num_suffixes_ = 9;
                    check_infixes.back().num_suffix_bits_ = 36;
                    check_infixes.back().trie_suffixes_.push_back(0b11001100101011001100111011101010100);
                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(3, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b10);

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 16;
                    check_infixes.back().trie_.push_back(0b1111010111010110);
                    check_infixes.back().num_suffixes_ = 6;
                    check_infixes.back().num_suffix_bits_ = 42;
                    check_infixes.back().trie_suffixes_.push_back(0b11000001100000111000011100001010000100);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 9;
                    check_infixes.back().trie_.push_back(0b111010010);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 21;
                    check_infixes.back().trie_suffixes_.push_back(0b11000001100000101);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(4, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b100);

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b100);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 13;
                    check_infixes.back().trie_.push_back(0b1111010111010);
                    check_infixes.back().num_suffixes_ = 5;
                    check_infixes.back().num_suffix_bits_ = 35;
                    check_infixes.back().trie_suffixes_.push_back(0b1100000110000011100001110000101);

                    check_infixes.emplace_back(0b101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 6;
                    check_infixes.back().trie_.push_back(0b110100);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 14;
                    check_infixes.back().trie_suffixes_.push_back(0b1100000101);

                    check_infixes.emplace_back(0b111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b110);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(5, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1000);

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b10);

                    check_infixes.emplace_back(0b101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 4;
                    check_infixes.back().trie_.push_back(0b1110);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 16;
                    check_infixes.back().trie_suffixes_.push_back(0b11100000101);

                    check_infixes.emplace_back(0b111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 7;
                    check_infixes.back().trie_.push_back(0b1111010);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 21;
                    check_infixes.back().trie_suffixes_.push_back(0b11000001100000111);

                    check_infixes.emplace_back(0b1001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 4;
                    check_infixes.back().trie_.push_back(0b1110);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 16;
                    check_infixes.back().trie_suffixes_.push_back(0b11000000101);

                    check_infixes.emplace_back(0b1111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b10);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
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
                keys[i] = {keys_contents[i], 8 * key_len};
                for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                    keys_contents[i][j] = rng();
            }
            std::sort(keys, keys + N);

            const uint64_t infix_value = 1;
            Diva<>::Infix infix(infix_value);

            SUBCASE("small slots") {
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    auto split_infixes = infix.SplitPrefixBits(3, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 35;
                    check_infixes.back().trie_.push_back(0b11101101001011011011010011010111010);
                    check_infixes.back().num_suffixes_ = 11;
                    check_infixes.back().num_suffix_bits_ = 11;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 43;
                    check_infixes.back().trie_.push_back(0b1111001111100010010111101101100000001001010);
                    check_infixes.back().num_suffixes_ = 9;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(6, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b10);

                    check_infixes.emplace_back(0b110);
                    
                    check_infixes.emplace_back(0b1001);

                    check_infixes.emplace_back(0b1011);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 6;
                    check_infixes.back().trie_.push_back(0b110100);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 4;
                    check_infixes.back().trie_suffixes_.push_back(0b101);

                    check_infixes.emplace_back(0b1101);

                    check_infixes.emplace_back(0b1111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 15;
                    check_infixes.back().trie_.push_back(0b111011010010110);
                    check_infixes.back().num_suffixes_ = 5;
                    check_infixes.back().num_suffix_bits_ = 10;
                    check_infixes.back().trie_suffixes_.push_back(0b101010101);

                    check_infixes.emplace_back(0b10001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 17;
                    check_infixes.back().trie_.push_back(0b11101101100000010);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 3;
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    check_infixes.emplace_back(0b10110);

                    check_infixes.emplace_back(0b11001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 9;
                    check_infixes.back().trie_.push_back(0b111110010);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 6;
                    check_infixes.back().trie_suffixes_.push_back(0b10101);

                    check_infixes.emplace_back(0b11111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 4;
                    check_infixes.back().trie_.push_back(0b1110);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 6;
                    check_infixes.back().trie_suffixes_.push_back(0b1001);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
            }
            SUBCASE("wide slots") {
                const uint32_t slot_size = 10;
                infix.BuildTrie(keys, N, key_start_bit, slot_size);

                {
                    auto split_infixes = infix.SplitPrefixBits(3, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 35;
                    check_infixes.back().trie_.push_back(0b11101101001011011011010011010111010);
                    check_infixes.back().num_suffixes_ = 11;
                    check_infixes.back().num_suffix_bits_ = 66;
                    check_infixes.back().trie_suffixes_.push_back(0b1011001000001100001001001100001110001010001000001010001101001000);
                    check_infixes.back().trie_suffixes_.push_back(0b0);

                    check_infixes.emplace_back(0b11);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 43;
                    check_infixes.back().trie_.push_back(0b1111001111100010010111101101100000001001010);
                    check_infixes.back().num_suffixes_ = 9;
                    check_infixes.back().num_suffix_bits_ = 45;
                    check_infixes.back().trie_suffixes_.push_back(0b11010101001000010000111001000011110100101010);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
                {
                    auto split_infixes = infix.SplitPrefixBits(6, slot_size);

                    std::vector<Diva<>::Infix> check_infixes;

                    check_infixes.emplace_back(0b1);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b100);

                    check_infixes.emplace_back(0b111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b101);
                    
                    check_infixes.emplace_back(0b1001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b1010);

                    check_infixes.emplace_back(0b1011);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 6;
                    check_infixes.back().trie_.push_back(0b110100);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 14;
                    check_infixes.back().trie_suffixes_.push_back(0b10100001000);

                    check_infixes.emplace_back(0b1101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b1110);

                    check_infixes.emplace_back(0b1111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 15;
                    check_infixes.back().trie_.push_back(0b111011010010110);
                    check_infixes.back().num_suffixes_ = 5;
                    check_infixes.back().num_suffix_bits_ = 35;
                    check_infixes.back().trie_suffixes_.push_back(0b10110001000000110000010010001100);

                    check_infixes.emplace_back(0b10001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 17;
                    check_infixes.back().trie_.push_back(0b11101101100000010);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 42;
                    check_infixes.back().trie_suffixes_.push_back(0b111110000000001100100000000011010);

                    check_infixes.emplace_back(0b10101);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 1;
                    check_infixes.back().trie_.push_back(0b1);
                    check_infixes.back().num_suffixes_ = 1;
                    check_infixes.back().num_suffix_bits_ = 9;
                    check_infixes.back().trie_suffixes_.push_back(0b100);

                    check_infixes.emplace_back(0b11001);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 9;
                    check_infixes.back().trie_.push_back(0b111110010);
                    check_infixes.back().num_suffixes_ = 3;
                    check_infixes.back().num_suffix_bits_ = 21;
                    check_infixes.back().trie_suffixes_.push_back(0b100000010000001110);

                    check_infixes.emplace_back(0b11111);
                    check_infixes.back().num_prefix_keys_ = 0;
                    check_infixes.back().num_trie_bits_ = 4;
                    check_infixes.back().trie_.push_back(0b1110);
                    check_infixes.back().num_suffixes_ = 2;
                    check_infixes.back().num_suffix_bits_ = 16;
                    check_infixes.back().trie_suffixes_.push_back(0b110100001010);

                    for (uint32_t i = 0; i < check_infixes.size(); i++)
                        AssertInfix(split_infixes[i], check_infixes[i]);
                }
            }
        }
    }


private:
    static void AssertInfix(const Diva<>::Infix& infix, const Diva<>::Infix& check_infix) {
        REQUIRE_EQ(infix.infix_, check_infix.infix_);
        REQUIRE_EQ(infix.num_prefix_keys_, check_infix.num_prefix_keys_);
        REQUIRE_EQ(infix.num_trie_bits_, check_infix.num_trie_bits_);
        REQUIRE_EQ(infix.trie_, check_infix.trie_);
        REQUIRE_EQ(infix.num_suffix_bits_, check_infix.num_suffix_bits_);
        REQUIRE_EQ(infix.num_suffixes_, check_infix.num_suffixes_);
        REQUIRE_EQ(infix.trie_suffixes_, check_infix.trie_suffixes_);
    }


    static void AssertRecoveredStrings(std::vector<Diva<>::InfiniteByteString> recovered_keys, std::vector<uint8_t> recovered_key_contents,
                                       std::vector<uint32_t> expected_key_bit_lens, std::vector<uint8_t> expected_key_contents) {
        REQUIRE_EQ(recovered_keys.size(), expected_key_bit_lens.size());
        REQUIRE_EQ(recovered_key_contents.size(), expected_key_contents.size());
        uint32_t ind = 0;
        for (uint32_t i = 0; i < recovered_keys.size(); i++) {
            const uint32_t bit_len = recovered_keys[i].length;
            const uint32_t byte_len = (bit_len + 7) / 8;
            REQUIRE_EQ(bit_len, expected_key_bit_lens[i]);
            REQUIRE_EQ(memcmp(recovered_key_contents.data() + ind, expected_key_contents.data() + ind, byte_len), 0);
            ind += byte_len;
        }
    }


    static void PrintKeys(const Diva<>::InfiniteByteString *keys, uint32_t N) {
        for (uint32_t i = 0; i < N; i++) {
            std::cerr << "key_len=" << keys[i].length << ": ";
            for (uint32_t j = 0; j < (keys[i].length + 7) / 8; j++) {
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
                  << " num_suffixes=" << infix.num_suffixes_
                  << " num_suffix_bits_=" << infix.num_suffix_bits_
                  << " num_trie_bits=" << infix.num_trie_bits_ << std::endl;
        std::cerr << "trie: ";
        for (uint32_t i = 0; i < infix.trie_.size(); i++) {
            for (uint32_t j = 0; j < 64; j++)
                std::cerr << ((infix.trie_[i] >> j) & 1);
        }
        std::cerr << std::endl << "trie_suffixes: ";
        for (uint32_t i = 0; i < infix.trie_suffixes_.size(); i++) {
            for (uint32_t j = 0; j < 64; j++)
                std::cerr << ((infix.trie_suffixes_[i] >> j) & 1);
        }
        std::cerr << std::endl;
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

    TEST_CASE("get strings") {
        InfixTests::TrieGetStrings();
    }

    TEST_CASE("delete") {
        InfixTests::TrieDelete();
    }

    TEST_CASE("get longest match") {
        InfixTests::TrieGetLongestMatch();
    }

    TEST_CASE("adapt") {
        InfixTests::TrieAdapt();
    }

    TEST_CASE("split") {
        InfixTests::TrieSplit();
    }

    TEST_CASE("merge") {
        //InfixTests::TrieMerge();
    }
}

}
