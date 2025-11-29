/**
 * @file infix tests
 * @author ---
 */

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
            infix.BuildTrie(keys, N, key_start_bit, slot_size);

            const std::vector<uint64_t> expected_trie = {0b1010'00000000000000000000000000110000,
                                                         0b111110000100011001000101110100110011100110001000};
            const std::vector<uint64_t> expected_trie_suffixes = {0b1000000110101110001100000001000000100110100000};
            AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
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

            const std::vector<uint64_t> expected_trie = {0b1'000000000000000'0000000000001001'00000000000000000000000000111011,
                                                         0b11100101000110111001100011011001100011010011011000100001110};
            const std::vector<uint64_t> expected_trie_suffixes = {0b10100100000100010000100001111011000010100000};
            AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
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

            const std::vector<uint64_t> expected_trie = {0b010100'00000000000000000000000001010100,
                                                         0b1110110010000000100101011101101001011011011010011010111010001000,
                                                         0b11110011111000100101};
            const std::vector<uint64_t> expected_trie_suffixes = {0b11001010011100000010000001001000011010010000000001010101100001,
                                                                  0b10110010100001000000110100001011100};
            AssertTrieContents(infix, expected_trie, expected_trie_suffixes);
        }
    }


    static void TrieQuery() {
        const uint32_t N = 10;
        const uint32_t slot_size = 5;
        const uint32_t key_start_bit = 6;
        const uint32_t rng_seed = 1380;
        std::mt19937_64 rng(rng_seed);

    }


private:
    static void AssertTrieContents(const Diva<>::Infix& infix,
                                   const std::vector<uint64_t>& expected_trie,
                                   const std::vector<uint64_t>& expected_trie_suffixes) {
        if (expected_trie.empty())
            REQUIRE_EQ(infix.trie, nullptr);
        else {
            REQUIRE_EQ(infix.trie->size(), expected_trie.size());
            REQUIRE_EQ(memcmp(infix.trie->data(),
                              expected_trie.data(),
                              sizeof(expected_trie[0]) * expected_trie.size()),
                       0);
        }
        if (expected_trie_suffixes.empty())
            REQUIRE_EQ(infix.trie_suffixes, nullptr);
        else {
            REQUIRE_EQ(infix.trie_suffixes->size(), expected_trie_suffixes.size());
            REQUIRE_EQ(memcmp(infix.trie_suffixes->data(),
                              expected_trie_suffixes.data(),
                              sizeof(expected_trie_suffixes[0]) * expected_trie_suffixes.size()),
                       0);
        }
    }


    static void PrintTrieAndTrieSuffixes(Diva<>::Infix& infix) {
        std::cerr << "has_prefix_keys=" << infix.HasPrefixKeys() 
                  << " call_depth=" << infix.GetCallDepth() 
                  << " num_trie_bits=" << infix.GetNumTrieBits() << std::endl;
        if (infix.trie != nullptr) {
            std::cerr << "trie: ";
            for (uint32_t i = 1; i < infix.trie->size(); i++) {
                for (uint32_t j = 0; j < 64; j++)
                    std::cerr << (((*infix.trie)[i] >> j) & 1);
            }
            std::cerr << std::endl;
        }
        if (infix.trie_suffixes != nullptr) {
            std::cerr << "trie_suffixes: ";
            for (uint32_t i = 0; i < infix.trie_suffixes->size(); i++) {
                for (uint32_t j = 0; j < 64; j++)
                    std::cerr << (((*infix.trie_suffixes)[i] >> j) & 1);
            }
            std::cerr << std::endl;
        }
    }
};

TEST_SUITE("infix_store") {
    TEST_CASE("build") {
        InfixTests::TrieBuild();
    }

    TEST_CASE("query") {
        InfixTests::TrieQuery();
    }
}

}
