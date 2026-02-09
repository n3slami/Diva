/**
 * @file infix store tests
 * @author ---
 */

#include <bitset>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <limits>
#include <random>
#include <sys/types.h>
#include <utility>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <cstdint>
#include <doctest/doctest.h>
#include <iomanip>
#include <iostream>

#include "diva.hpp"
#include "util.hpp"

namespace diva {

typedef Diva<DivaType::Standard, PayloadType::FixedLength> PayloadDiva;
typedef Diva<DivaType::BinaryTrie, PayloadType::None> BinaryTrieDiva;

class InfixStoreTests {
public:
    static void Allocation() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep],
                s.infix_size_, s.size_scalar_shrink_grow_sep);

        const uint32_t total_words = (Diva<>::infix_store_target_size + 
                                        (s.infix_size_ + 1) * s.scaled_sizes_[s.size_scalar_shrink_grow_sep] + 63) / 64;
        for (int32_t i = 0; i < total_words; i++)
            REQUIRE_EQ(store.ptr[i], 0);
    }

    static void ShiftingSlots() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep],
                s.infix_size_, s.size_scalar_shrink_grow_sep);

        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        for (int32_t i = 0; i < total_slots; i++)
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
        for (int32_t i = 0; i < total_slots; i++)
            REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));

        SUBCASE("right shifting: short segment + short shift") {
            s.ShiftSlotsRight(store, 2, 4, 2);
            REQUIRE_EQ(s.GetSlot(store, 0), 0);
            REQUIRE_EQ(s.GetSlot(store, 1), 1);
            REQUIRE_EQ(s.GetSlot(store, 2), 0);
            REQUIRE_EQ(s.GetSlot(store, 3), 0);
            REQUIRE_EQ(s.GetSlot(store, 4), 2);
            REQUIRE_EQ(s.GetSlot(store, 5), 3);
            for (int32_t i = 6; i < total_slots; i++)
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
        }

        SUBCASE("right shifting: long segment + short shift") {
            for (int32_t i = 0; i < total_slots; i++)
                s.SetSlot(store, i, i & BITMASK(s.infix_size_));
            s.ShiftSlotsRight(store, 1, 100, 2);
            for (int32_t i = 0; i < 3; i++)
                REQUIRE_EQ(s.GetSlot(store, i), 0);
            for (int32_t i = 1; i < 100; i++)
                REQUIRE_EQ(s.GetSlot(store, 2 + i), (i & BITMASK(s.infix_size_)));
            for (int32_t i = 102; i < total_slots; i++)
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
        }

        SUBCASE("right shifting: short segment + long shift") {
            for (int32_t i = 0; i < total_slots; i++)
                s.SetSlot(store, i, i & BITMASK(s.infix_size_));
            s.ShiftSlotsRight(store, 3, 7, 150);
            REQUIRE_EQ(s.GetSlot(store, 0), 0);
            REQUIRE_EQ(s.GetSlot(store, 1), 1);
            REQUIRE_EQ(s.GetSlot(store, 2), 2);
            for (int32_t i = 3; i < 153; i++)
                REQUIRE_EQ(s.GetSlot(store, i), 0);
            REQUIRE_EQ(s.GetSlot(store, 153), 3);
            REQUIRE_EQ(s.GetSlot(store, 154), 4);
            REQUIRE_EQ(s.GetSlot(store, 155), 5);
            REQUIRE_EQ(s.GetSlot(store, 156), 6);
            for (int32_t i = 157; i < total_slots; i++)
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
        }

        SUBCASE("right shifting: long segment + long shift") {
            for (int32_t i = 0; i < total_slots; i++)
                s.SetSlot(store, i, i & BITMASK(s.infix_size_));
            s.ShiftSlotsRight(store, 0, 110, 200);
            for (int32_t i = 0; i < 200; i++)
                REQUIRE_EQ(s.GetSlot(store, i), 0);
            for (int32_t i = 0; i < 110; i++)
                REQUIRE_EQ(s.GetSlot(store, i + 200), (i & BITMASK(s.infix_size_)));
            for (int32_t i = 310; i < total_slots; i++)
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
        }

        SUBCASE("left shifting: short segment + short shift") {
            for (int32_t i = 0; i < total_slots; i++)
                s.SetSlot(store, i, i & BITMASK(s.infix_size_));
            s.ShiftSlotsLeft(store, total_slots - 10, total_slots, 2);
            REQUIRE_EQ(s.GetSlot(store, total_slots - 1), 0);
            REQUIRE_EQ(s.GetSlot(store, total_slots - 2), 0);
            for (int32_t i = total_slots - 10; i < total_slots; i++)
                REQUIRE_EQ(s.GetSlot(store, i - 2), (i & BITMASK(s.infix_size_)));
            for (int32_t i = 0; i < total_slots - 12; i++)
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
        }

        SUBCASE("left shifting: short segment + long shift") {
            for (int32_t i = 0; i < total_slots; i++)
                s.SetSlot(store, i, i & BITMASK(s.infix_size_));
            s.ShiftSlotsLeft(store, total_slots - 15, total_slots - 2, 210);
            REQUIRE_EQ(s.GetSlot(store, total_slots - 1),
                    ((total_slots - 1) & BITMASK(s.infix_size_)));
            REQUIRE_EQ(s.GetSlot(store, total_slots - 2),
                    ((total_slots - 2) & BITMASK(s.infix_size_)));
            for (int32_t i = total_slots - 212; i < total_slots - 2; i++)
                REQUIRE_EQ(s.GetSlot(store, i), 0);
            for (int32_t i = total_slots - 15; i < total_slots - 2; i++)
                REQUIRE_EQ(s.GetSlot(store, i - 210), (i & BITMASK(s.infix_size_)));
            for (int32_t i = 0; i < total_slots - 225; i++)
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
        }

        SUBCASE("left shifting: long segment + short shift") {
            for (int32_t i = 0; i < total_slots; i++)
                s.SetSlot(store, i, i & BITMASK(s.infix_size_));
            s.ShiftSlotsLeft(store, total_slots - 250, total_slots - 1, 1);
            REQUIRE_EQ(s.GetSlot(store, total_slots - 1),
                    ((total_slots - 1) & BITMASK(s.infix_size_)));
            REQUIRE_EQ(s.GetSlot(store, total_slots - 2), 0);
            for (int32_t i = total_slots - 250; i < total_slots - 1; i++)
                REQUIRE_EQ(s.GetSlot(store, i - 1), (i & BITMASK(s.infix_size_)));
            for (int32_t i = 0; i < total_slots - 251; i++)
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
        }

        SUBCASE("left shifting: long segment + long shift") {
            for (int32_t i = 0; i < total_slots; i++)
                s.SetSlot(store, i, i & BITMASK(s.infix_size_));
            s.ShiftSlotsLeft(store, total_slots - 100, total_slots - 1, 200);
            REQUIRE_EQ(s.GetSlot(store, total_slots - 1),
                    ((total_slots - 1) & BITMASK(s.infix_size_)));
            for (int32_t i = total_slots - 201; i < total_slots - 1; i++)
                REQUIRE_EQ(s.GetSlot(store, i), 0);
            for (int32_t i = total_slots - 100; i < total_slots - 1; i++)
                REQUIRE_EQ(s.GetSlot(store, i - 200), (i & BITMASK(s.infix_size_)));
            for (int32_t i = 0; i < total_slots - 300; i++)
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
        }
    }

    static void ShiftingRunends() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep],
                s.infix_size_, s.size_scalar_shrink_grow_sep);

        uint64_t *runends = store.ptr + Diva<>::num_metadata_offset_words +
            Diva<>::infix_store_target_size / 64;
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;

        SUBCASE("right shifting: short segment + short shift") {
            s.ShiftRunendsRight(store, 62, 65, 5);
            REQUIRE_EQ(runends[0], 0b0000100010001000100010001000100010001000100010001000100010001000);
            REQUIRE_EQ(runends[1], 0b0101010101010101010101010101010101010101010101010101010101110000);
            REQUIRE_EQ(runends[2], 0b1010101010101010101010101010101010101010101010101010101010101010);
            REQUIRE_EQ(runends[3], 0b1111111111111111111111111111111100000000000000000000000000000000);
        }

        SUBCASE("right shifting: short segment + long shift") {
            s.ShiftRunendsRight(store, 60, 64, 125);
            REQUIRE_EQ(runends[0], 0b0000100010001000100010001000100010001000100010001000100010001000);
            REQUIRE_EQ(runends[1], 0b0000000000000000000000000000000000000000000000000000000000000000);
            REQUIRE_EQ(runends[2], 0b1011000000000000000000000000000000000000000000000000000000000000);
            REQUIRE_EQ(runends[3], 0b1111111111111111111111111111111100000000000000000000000000000000);
        }

        SUBCASE("right shifting: long segment + short shift") {
            s.ShiftRunendsRight(store, 1, 123, 3);
            REQUIRE_EQ(runends[0], 0b0100010001000100010001000100010001000100010001000100010001000000);
            REQUIRE_EQ(runends[1], 0b0110101010101010101010101010101010101010101010101010101010101100);
            REQUIRE_EQ(runends[2], 0b1010101010101010101010101010101010101010101010101010101010101010);
            REQUIRE_EQ(runends[3], 0b1111111111111111111111111111111100000000000000000000000000000000);
        }

        SUBCASE("right shifting: long segment + long shift") {
            s.ShiftRunendsRight(store, 2, 127, 100);
            REQUIRE_EQ(runends[0], 0b0000000000000000000000000000000000000000000000000000000000000000);
            REQUIRE_EQ(runends[1], 0b1000100010001000100010001000000000000000000000000000000000000000);
            REQUIRE_EQ(runends[2], 0b0101010101010101010101010101100010001000100010001000100010001000);
            REQUIRE_EQ(runends[3], 0b1111111111111111111111111111110101010101010101010101010101010101);
        }

        SUBCASE("left shifting: short segment + short shift") {
            s.ShiftRunendsLeft(store, 190, 194, 4);
            REQUIRE_EQ(runends[0], 0b1000100010001000100010001000100010001000100010001000100010001000);
            REQUIRE_EQ(runends[1], 0b0101010101010101010101010101010101010101010101010101010101010101);
            REQUIRE_EQ(runends[2], 0b0000101010101010101010101010101010101010101010101010101010101010);
            REQUIRE_EQ(runends[3], 0b1111111111111111111111111111111100000000000000000000000000000000);
        }

        SUBCASE("left shifting: short segment + long shift") {
            s.ShiftRunendsLeft(store, 190, 194, 130);
            REQUIRE_EQ(runends[0], 0b0010100010001000100010001000100010001000100010001000100010001000);
            REQUIRE_EQ(runends[1], 0b0000000000000000000000000000000000000000000000000000000000000000);
            REQUIRE_EQ(runends[2], 0b0000000000000000000000000000000000000000000000000000000000000000);
            REQUIRE_EQ(runends[3], 0b1111111111111111111111111111111100000000000000000000000000000000);
        }

        SUBCASE("left shifting: long segment + short shift") {
            s.ShiftRunendsLeft(store, 120, 250, 3);
            REQUIRE_EQ(runends[0], 0b1000100010001000100010001000100010001000100010001000100010001000);
            REQUIRE_EQ(runends[1], 0b0100101010110101010101010101010101010101010101010101010101010101);
            REQUIRE_EQ(runends[2], 0b0001010101010101010101010101010101010101010101010101010101010101);
            REQUIRE_EQ(runends[3], 0b1111110001111111111111111111111111100000000000000000000000000000);
        }

        SUBCASE("left shifting: long segment + long shift") {
            s.ShiftRunendsLeft(store, 120, 250, 100);
            REQUIRE_EQ(runends[0], 0b1010101010101010101010101010101010100101010110001000100010001000);
            REQUIRE_EQ(runends[1], 0b1111000000000000000000000000000000001010101010101010101010101010);
            REQUIRE_EQ(runends[2], 0b0000000000000000000000000000000000000000001111111111111111111111);
            REQUIRE_EQ(runends[3], 0b1111110000000000000000000000000000000000000000000000000000000000);
        }
    }

    static void InsertRaw() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep],
                s.infix_size_, s.size_scalar_shrink_grow_sep);
        uint64_t *runends = store.ptr + Diva<>::num_metadata_offset_words 
                            + Diva<>::infix_store_target_size / 64;
        uint64_t inserts[100];

        inserts[0] = 0b0100000000001100;
        inserts[1] = 0b0100000000001011;
        inserts[2] = 0b0100000000001101;
        inserts[3] = 0b0100000000001110;
        s.InsertRawIntoInfixStore(store, inserts[0]);
        s.InsertRawIntoInfixStore(store, inserts[2]);
        s.InsertRawIntoInfixStore(store, inserts[1]);
        s.InsertRawIntoInfixStore(store, inserts[3]);
        SUBCASE("insertion and shifting of a single run") {
            for (int32_t i = 0; i < 4; i++)
                REQUIRE_EQ(s.GetSlot(store, 538 + i), (inserts[i] & BITMASK(infix_size)));
            for (int32_t i = 538; i < 541; i++)
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            REQUIRE_EQ(get_bitmap_bit(runends, 541), 1);
        }

        inserts[4] = 0b0011111111100001;
        inserts[5] = 0b0011111111100010;
        inserts[6] = 0b0011111111100100;
        inserts[7] = 0b0011111111100011;
        s.InsertRawIntoInfixStore(store, inserts[4]);
        s.InsertRawIntoInfixStore(store, inserts[5]);
        s.InsertRawIntoInfixStore(store, inserts[6]);
        s.InsertRawIntoInfixStore(store, inserts[7]);
        SUBCASE("insertion and shifting of a new run that shifts an old run") {
            for (int32_t i = 537; i < 540; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 537 + 4] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 540), (inserts[7] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 540), 1);
            for (int32_t i = 541; i < 544; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 541] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 544), (inserts[3] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 544), 1);
        }

        inserts[0] = 0b0000000000000001;
        inserts[1] = 0b0000000000000001;
        inserts[2] = 0b0000000000000010;
        inserts[3] = 0b0000000000000011;
        s.InsertRawIntoInfixStore(store, inserts[0]);
        s.InsertRawIntoInfixStore(store, inserts[3]);
        s.InsertRawIntoInfixStore(store, inserts[1]);
        s.InsertRawIntoInfixStore(store, inserts[2]);
        SUBCASE("insertion and shifting at the very beginning of the array") {
            for (int32_t i = 0; i < 3; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 3), (inserts[3] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 3), 1);
        }

        SUBCASE("inserting new runs in between two touching runs: 0 slots added") {
            REQUIRE_EQ(s.GetSlot(store, 4), 0b00000);
            REQUIRE_EQ(get_bitmap_bit(runends, 4), 0);
            REQUIRE_EQ(s.GetSlot(store, 5), 0b00000);
            REQUIRE_EQ(get_bitmap_bit(runends, 5), 0);
        }

        inserts[5] = 0b0000000001111111;
        s.InsertRawIntoInfixStore(store, inserts[5]);
        SUBCASE("inserting new runs in between two touching runs: 1 slots added") {
            REQUIRE_EQ(s.GetSlot(store, 4), (inserts[5] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 4), 1);
            REQUIRE_EQ(s.GetSlot(store, 5), 0b00000);
            REQUIRE_EQ(get_bitmap_bit(runends, 5), 0);
        }

        inserts[4] = 0b0000000000110101;
        s.InsertRawIntoInfixStore(store, inserts[4]);
        SUBCASE("inserting new runs in between two touching runs: 2 slots added") {
            REQUIRE_EQ(s.GetSlot(store, 4), (inserts[4] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 4), 1);
            REQUIRE_EQ(s.GetSlot(store, 5), (inserts[5] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 5), 1);
        }

        inserts[0] = 0b0111111110100001;
        inserts[1] = 0b0111111110100010;
        inserts[2] = 0b0111111110100011;
        inserts[3] = 0b0111111110100111;
        inserts[4] = 0b0111111111100001;
        inserts[5] = 0b0111111111100001;
        inserts[6] = 0b0111111111100010;
        s.InsertRawIntoInfixStore(store, inserts[0]);
        s.InsertRawIntoInfixStore(store, inserts[4]);
        s.InsertRawIntoInfixStore(store, inserts[5]);
        s.InsertRawIntoInfixStore(store, inserts[1]);
        s.InsertRawIntoInfixStore(store, inserts[6]);
        s.InsertRawIntoInfixStore(store, inserts[2]);
        s.InsertRawIntoInfixStore(store, inserts[3]);
        SUBCASE("insertion and shifting at the very end of the array") {
            for (int32_t i = 1070; i < 1073; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1070] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1073), (inserts[3] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 1073), 1);
            for (int32_t i = 1074; i < 1076; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1074 + 4] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1076), (inserts[6] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 1076), 1);
        }

        inserts[7] = 0b0111111111011101;
        inserts[8] = 0b0111111111011110;
        inserts[9] = 0b0111111111011111;
        s.InsertRawIntoInfixStore(store, inserts[9]);
        s.InsertRawIntoInfixStore(store, inserts[7]);
        s.InsertRawIntoInfixStore(store, inserts[8]);
        SUBCASE("insertion and shifting in between touching runs at the very end "
                "of the array") {
            for (int32_t i = 1067; i < 1070; i++) {
                REQUIRE_EQ(s.GetSlot(store, i),
                        (inserts[i - 1067] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1070), (inserts[3] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 1070), 1);
            for (int32_t i = 1071; i < 1073; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1071 + 7] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1073), (inserts[9] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 1073), 1);
            for (int32_t i = 1074; i < 1076; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1074 + 4] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1076), (inserts[6] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 1076), 1);
        }
    }

    static void DeleteRaw() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep],
                s.infix_size_, s.size_scalar_shrink_grow_sep);

        const uint32_t rng_seed = 20;
        std::mt19937_64 rng(rng_seed);

        std::vector<uint64_t> keys{ 0b000000010011000,
            0b000000010010100, 0b000000010010110,
            0b000000010010101, 0b000000010011111,
            0b000000010110101, 0b000000010110111,
            0b000000010111001, 0b000000011111011,
            0b000000100011011, 0b000000100011111,
            0b000000111100001, 0b000000111100011,
            0b000000111100111, 0b111111010000001,
            0b111111101100101, 0b111111101100111,
            0b111111110011111, 0b111111110110101,
            0b111111111011000, 0b111111111010100,
            0b111111111010110, 0b111111111010101,
            0b111111111011111, 0b111111111100001,
            0b111111111100011};
        while (keys.size() < s.scaled_sizes_[store.GetSizeGrade() - 1]) {
            const uint64_t candidate =
                rng() & BITMASK(Diva<>::base_implicit_size + infix_size);
            if (candidate & BITMASK(infix_size))
                keys.push_back(candidate);
        }
        std::sort(keys.begin(), keys.end(),
                [&](uint64_t a, uint64_t b) { return s.CompareInfixes(a, b); });
        s.LoadListToInfixStore(store, keys.data(), keys.size());

        SUBCASE("single match, shift left") {
            s.DeleteRawFromInfixStore(store, 0b000000010011111);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("delete/single_match/shift_left");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("multiple matches, shift left") {
            s.DeleteRawFromInfixStore(store, 0b000000010010101);
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("delete/multiple_matches/shift_left/1");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }

            s.DeleteRawFromInfixStore(store, 0b000000010010101);
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("delete/multiple_matches/shift_left/2");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }

        SUBCASE("destroy run, shift left") {
            s.DeleteRawFromInfixStore(store, 0b00000011111011);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("delete/destroy_run/shift_left");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("end of run, shift left") {
            s.DeleteRawFromInfixStore(store, 0b000000010011111);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("delete/end_of_run/shift_left");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("single match, shift right") {
            s.DeleteRawFromInfixStore(store, 0b111111111011111);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("delete/single_match/shift_right");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("destroy run, shift right") {
            s.DeleteRawFromInfixStore(store, 0b111111110110101);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("delete/destroy_run/shift_right");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("end of run, shift right") {
            s.DeleteRawFromInfixStore(store, 0b111111111100011);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("delete/end_of_run/shift_right");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b010000000010101);
        SUBCASE("lone run with single slot") {
            s.DeleteRawFromInfixStore(store, 0b010000000010101);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("delete/lone_run_with_single_slot");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }

    static void GetLongestMatchingInfixSize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep],
                s.infix_size_, s.size_scalar_shrink_grow_sep);

        const std::vector<uint64_t> keys{ 0b000000010011000,
            0b000000010010100, 0b000000010010110,
            0b000000010010101, 0b000000010011111,
            0b000000010110101, 0b000000010110111,
            0b000000010111001, 0b000000011111011,
            0b000000100011011, 0b000000100011111,
            0b000000111100001, 0b000000111100011,
            0b000000111100111, 0b000100111110000,
            0b000100111110010, 0b000100111110011,
            0b111111010000001, 0b111111101100101,
            0b111111101100111, 0b111111110011111,
            0b111111110110101, 0b111111111011000,
            0b111111111010100, 0b111111111010110,
            0b111111111010101, 0b111111111011111,
            0b111111111100001, 0b111111111100011};
        s.LoadListToInfixStore(store, keys.data(), keys.size());

        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110011), 
                infix_size - 1);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110001), 
                infix_size - 2);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111100001),
                infix_size - 5);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b111111111111111),
                -1);
    }


    static void GetInfixList() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep], s.infix_size_,
                s.size_scalar_shrink_grow_sep);

        const std::vector<uint64_t> keys {0b000000000000001, 0b000000000000101,
            0b000000000010101, 0b000000000100001, 0b000000000100011,
            0b000000000100101, 0b001000000100001, 0b001000000100011,
            0b001000000100101, 0b001000000100110, 0b001000000100110,
            0b001000000100110, 0b001000001100001, 0b001000001100011,
            0b001000001100101, 0b001111111000001, 0b001111111000010,
            0b001111111000010, 0b001111111100001, 0b001111111100010,
            0b001111111100010};
        for (uint64_t key : keys)
            s.InsertRawIntoInfixStore(store, key);
        uint64_t res[keys.size() + 1];
        const uint32_t len = s.GetInfixList(store, res);
        REQUIRE_EQ(len, keys.size());
        for (int32_t i = 0; i < keys.size(); i++)
            REQUIRE_EQ(res[i], keys[i]);
    }


    static void LoadInfixList() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep], s.infix_size_,
                                 s.size_scalar_shrink_grow_sep);

        SUBCASE("fetch") {
            const std::vector<uint64_t> keys {0b0000000000000001,
                0b0000000000000101, 0b0000000000010101,
                0b0000000000100001, 0b0000000000100011,
                0b0000000000100101, 0b0001000000100001,
                0b0001000000100011, 0b0001000000100110,
                0b0001000000100110, 0b0001000000100110,
                0b0001000000100101, 0b0001000001100001,
                0b0001000001100011, 0b0001000001100101,
                0b0001111111000010, 0b0001111111000010,
                0b0001111111000001,  0b0001111111100010,
                0b0001111111100010, 0b0001111111100001,
                0b0111111111000010, 0b0111111111000010,
                0b0111111111000001,  0b0111111111100010,
                0b0111111111100010, 0b0111111111100001};
            s.LoadListToInfixStore(store, keys.data(), keys.size());
            uint64_t res[keys.size() + 1];
            const uint32_t len = s.GetInfixList(store, res);
            REQUIRE_EQ(len, keys.size());
            for (int32_t i = 0; i < keys.size(); i++)
                REQUIRE_EQ(res[i], keys[i]);
        }

        SUBCASE("vs. insert one by one") {
            const uint32_t n_keys = Diva<>::infix_store_target_size;
            const uint32_t rng_seed = 1;
            std::mt19937_64 rng(rng_seed);
            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++)
                keys.push_back((rng() & BITMASK(Diva<>::base_implicit_size + infix_size)) | 1ULL);
            std::sort(keys.begin(), keys.end(), 
                    [&](uint64_t a, uint64_t b) { return s.CompareInfixes(a, b); } );
            s.LoadListToInfixStore(store, keys.data(), keys.size());

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("load_infix_list");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }


    static void PointQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep],
                                 s.infix_size_,
                                 s.size_scalar_shrink_grow_sep);

        const std::vector<uint64_t> keys {0b000000000000001,
            0b000000000000101, 0b000000000010101,
            0b000000000100001, 0b000000000101000,
            0b000000000101011, 0b001000000100001,
            0b001000000100011, 0b001000000100101,
            0b001000000100110, 0b001000000100110,
            0b001000000100110, 0b001000001110000,
            0b001000001100011, 0b001000001100101,
            0b001111111000001, 0b001111111000011,
            0b001111111000111, 0b001111111100001,
            0b001111111100010, 0b001111111100010};
        for (uint64_t key : keys)
            s.InsertRawIntoInfixStore(store, key);

        SUBCASE("no false negatives") {
            for (uint64_t key : keys)
                REQUIRE_EQ(s.PointQueryInfixStore(store, key), true);
        }

        SUBCASE("extensions of partial infix") {
            std::vector<uint64_t> queries;
            for (uint64_t key : keys)
                for (uint64_t query_key = key - (key & -key); query_key < (key | (key - 1)); query_key++)
                    queries.emplace_back(query_key);
            for (uint64_t query : queries)
                REQUIRE_EQ(s.PointQueryInfixStore(store, query), true);
        }

        SUBCASE("negatives") {
            std::vector<uint64_t> queries;
            for (uint64_t query_key = 0; query_key < (1ULL << (s.infix_size_ + Diva<>::base_implicit_size)); query_key++) {
                bool valid = true;
                for (uint64_t key : keys)
                    if (key - (key & -key) <= query_key && query_key <= (key | (key - 1))) {
                        valid = false;
                        break;
                    }
                if (valid)
                    queries.emplace_back(query_key);
            }
            for (uint64_t query : queries)
                REQUIRE_EQ(s.PointQueryInfixStore(store, query), false);
        }
    }


    static void RangeQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep],
                                 s.infix_size_,
                                 s.size_scalar_shrink_grow_sep);
        
        const uint32_t n_queries = 100000;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        const std::vector<uint64_t> keys {0b000000000000001,
            0b000000000000101, 0b000000000010101,
            0b000000000100001, 0b000000000101000,
            0b000000000101011, 0b001000000100001,
            0b001000000100011, 0b001000000100101,
            0b001000000100110, 0b001000000100110,
            0b001000000100110, 0b001000001110000,
            0b001000001100011, 0b001000001100101,
            0b011111111000001, 0b011111111000011,
            0b011111111000111, 0b011111111100001,
            0b011111111100010, 0b011111111100010};
        for (uint64_t key : keys)
            s.InsertRawIntoInfixStore(store, key);

        SUBCASE("no false negatives") {
            std::vector<std::pair<uint64_t, uint64_t>> queries;
            while (queries.size() < n_queries) {
                uint64_t l = rng() & BITMASK(s.infix_size_ + Diva<>::base_implicit_size);
                uint64_t r = rng() & BITMASK(s.infix_size_ + Diva<>::base_implicit_size);
                if (l > r)
                    std::swap(l, r);

                bool valid = false;
                for (uint64_t key : keys) {
                    const uint64_t key_l = key & (key - 1);
                    const uint64_t key_r = key | (key - 1);
                    if (std::max(key_l, l) <= std::min(key_r, r)) {
                        valid = true;
                        break;
                    }
                }
                if (valid)
                    queries.emplace_back(l, r);
            }
            for (auto [query_l, query_r] : queries)
                REQUIRE_EQ(s.RangeQueryInfixStore(store, query_l, query_r), true);
        }

        SUBCASE("negatives") {
            std::vector<std::pair<uint64_t, uint64_t>> queries;
            while (queries.size() < n_queries) {
                uint64_t l = rng() & BITMASK(s.infix_size_ + Diva<>::base_implicit_size);
                uint64_t r = rng() & BITMASK(s.infix_size_ + Diva<>::base_implicit_size);
                if (l > r)
                    std::swap(l, r);

                bool valid = true;
                for (uint64_t key : keys) {
                    const uint64_t key_l = key - (key & -key);
                    const uint64_t key_r = key | (key - 1);
                    if (std::max(key_l, l) <= std::min(key_r, r)) {
                        valid = false;
                        break;
                    }
                }
                if (valid)
                    queries.emplace_back(l, r);
            }
            for (auto [query_l, query_r] : queries)
                REQUIRE_EQ(s.RangeQueryInfixStore(store, query_l, query_r), false);
        }
    }


    static void Resize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        Diva<> s(infix_size, seed, load_factor);
        Diva<>::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep], s.infix_size_,
                                 s.size_scalar_shrink_grow_sep);
        
        SUBCASE("expand") {
            const uint32_t n_keys = s.scaled_sizes_[s.size_scalar_shrink_grow_sep] - 1;
            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++) {
                keys.push_back((rng() & BITMASK(Diva<>::base_implicit_size + infix_size)) | 1UL);
                s.InsertRawIntoInfixStore(store, keys.back());
                REQUIRE_EQ(store.GetFullSlotCount(), i + 1);
                uint64_t infix_list[n_keys];
                REQUIRE_EQ(store.GetFullSlotCount(), s.GetInfixList(store, infix_list));
                std::sort(keys.begin(), keys.end(),
                        [&](uint64_t a, uint64_t b) { return s.CompareInfixes(a, b); });
                for (int32_t j = 0; j < store.GetFullSlotCount(); j++)
                    REQUIRE_EQ(keys[j], infix_list[j]);
            }
            s.ResizeInfixStore(store);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("resize/expand");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
        SUBCASE("contract") {
            const uint32_t n_keys = s.scaled_sizes_[s.size_scalar_shrink_grow_sep] - 500;
            for (int32_t i = 0; i < n_keys; i++)
                s.InsertRawIntoInfixStore(store, (rng() & BITMASK(Diva<>::base_implicit_size + infix_size)) | 1ULL);
            s.ResizeInfixStore(store);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("resize/contract");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }


    static void PayloadsSanity() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const uint32_t payload_size = 100;
        const float load_factor = 0.95;

        SUBCASE("copy bitmap") {
            uint64_t a[4], b[4];
            a[0] = 0b0010100111010100110101011010100100101001110101001101010110101001;
            a[1] = 0b1101000110111111111111111111111111110000000000000000000000000000;
            a[2] = 0b0000000000000000000000000000011111111111111111111111111111111110;
            a[3] = 0b0000000010101010101010101100110011001100110011001100110011001100;
            b[0] = 0b0000000000000000000000000000000000000000000000000000000000000000;
            b[1] = 0b0000000001110001110001110001110001110001110001110001110001110001;
            b[2] = 0b0000000000000000000000000000000000000000000000000000000000000000;
            b[3] = 0b1111111111111111111111111111111111111111111111111111111111111111;
            copy_bitmap_to_bitmap(a, 10, b, 20, 110);
            REQUIRE_EQ(b[0], 0b0101001101010110101001001010011101010011010100000000000000000000);
            REQUIRE_EQ(b[1], 0b1111111111111111111111111100000000000000000000000000000010100111);
            REQUIRE_EQ(b[2], 0b0000000000000000000000000000000000000000000000000000000000000010);
            REQUIRE_EQ(b[3], 0b1111111111111111111111111111111111111111111111111111111111111111);

            a[0] = 0b0010111010010011010011101010011101010111111100010100111010010101;
            a[1] = 0b1001110101110000101010010011000000111010101111111111100101001001;
            a[2] = 0b0001111111010100111111101010010001100001010100000011010101000010;
            a[3] = 0b1010111010100011101010111111111100000000000101010101010101010101;
            b[0] = 0b0000000000000000000000000000000000000000000000000000000000000000;
            b[1] = 0b0000000001110001110001110001110001110001110001110001110001110001;
            b[2] = 0b0000000000000000000000000000000000000000000000000000000000000000;
            b[3] = 0b1111111111111111111111111111111111111111111111111111111111111111;
            copy_bitmap_to_bitmap(a, 63, b, 125, 77);
            REQUIRE_EQ(b[0], 0b0000000000000000000000000000000000000000000000000000000000000000);
            REQUIRE_EQ(b[1], 0b0100000001110001110001110001110001110001110001110001110001110001);
            REQUIRE_EQ(b[2], 0b1010011101011100001010100100110000001110101011111111111001010010);
            REQUIRE_EQ(b[3], 0b1111111111111111111111111111111111111111111111111111110101010000);
        }

        PayloadDiva s(infix_size, seed, load_factor, payload_size);
        PayloadDiva::InfixStore store(s.scaled_sizes_[s.size_scalar_shrink_grow_sep], s.infix_size_,
                                      s.size_scalar_shrink_grow_sep, payload_size);
        
        SUBCASE("allocation") {
            const std::vector<uint32_t> occupieds_pos = {};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
            const uint64_t check_payloads_contents[1][payload_size / 64 + 2] = {{0xb7355bcccb7eb8c5, 0x1c59030f7, }};
            const uint64_t *check_payloads[1] = {&(check_payloads_contents[0][0])};
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }

        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        for (int32_t i = 0; i < total_slots; i++) {
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
            uint64_t payload[payload_size / 64 + 1];
            memset(payload, 0, sizeof(payload));
            const uint32_t set_bit_pos = i % payload_size;
            payload[set_bit_pos / 64] = 1ULL << (set_bit_pos % 64);
            s.SetPayload(store, i, payload);
        }

        SUBCASE("set and get") {
            for (int32_t i = 0; i < total_slots; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (i & BITMASK(s.infix_size_)));
                uint64_t expected_payload[payload_size / 64 + 1];
                memset(expected_payload, 0, sizeof(expected_payload));
                const uint32_t set_bit_pos = i % payload_size;
                expected_payload[set_bit_pos / 64] = 1ULL << (set_bit_pos % 64);
                uint64_t payload[payload_size / 64 + 1];
                s.GetPayload(store, i, payload);
                REQUIRE(compare_bitmap_to_bitmap(payload, 0, expected_payload, 0, payload_size));
            }
        }

        s.ShiftSlotsRight(store, 10, 55, 5);
        s.ShiftPayloadsRight(store, 10, 55, 5);

        SUBCASE("shifting right") {
            for (int32_t i = 0; i < total_slots; i++) {
                int32_t j = i;
                bool zero_payload = false;
                if (10 <= i && i < 15) {
                    j = 0;
                    zero_payload = true;
                }
                else if (15 <= i && i < 60)
                    j -= 5;
                REQUIRE_EQ(s.GetSlot(store, i), (j & BITMASK(s.infix_size_)));
                uint64_t expected_payload[payload_size / 64 + 1];
                memset(expected_payload, 0, sizeof(expected_payload));
                if (!zero_payload) {
                    const uint32_t set_bit_pos = j % payload_size;
                    expected_payload[set_bit_pos / 64] = 1ULL << (set_bit_pos % 64);
                }
                uint64_t payload[payload_size / 64 + 1];
                s.GetPayload(store, i, payload);
                REQUIRE(compare_bitmap_to_bitmap(payload, 0, expected_payload, 0, payload_size));
            }
        }

        s.ShiftSlotsLeft(store, 70, 80, 20);
        s.ShiftPayloadsLeft(store, 70, 80, 20);

        SUBCASE("shifting left") {
            for (int32_t i = 0; i < total_slots; i++) {
                int32_t j = i;
                bool zero_payload = false;
                if (10 <= i && i < 15) {
                    j = 0;
                    zero_payload = true;
                }
                else if (15 <= i && i < 50)
                    j -= 5;
                else if (50 <= i && i < 60)
                    j += 20;
                else if (60 <= i && i < 80) {
                    j = 0;
                    zero_payload = true;
                }
                REQUIRE_EQ(s.GetSlot(store, i), (j & BITMASK(s.infix_size_)));
                uint64_t expected_payload[payload_size / 64 + 1];
                memset(expected_payload, 0, sizeof(expected_payload));
                if (!zero_payload) {
                    const uint32_t set_bit_pos = j % payload_size;
                    expected_payload[set_bit_pos / 64] = 1ULL << (set_bit_pos % 64);
                }
                uint64_t payload[payload_size / 64 + 1];
                s.GetPayload(store, i, payload);
                REQUIRE(compare_bitmap_to_bitmap(payload, 0, expected_payload, 0, payload_size));
            }
        }
    }


    static void PayloadsInsertRaw() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const uint32_t payload_size = 100;
        const uint32_t infix_store_target_size = PayloadDiva::infix_store_target_size;
        const float load_factor = 0.95;

        const uint32_t rng_seed = 20;
        std::mt19937_64 rng(rng_seed);

        PayloadDiva s(infix_size, seed, load_factor, payload_size);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        PayloadDiva::InfixStore store(total_slots, s.infix_size_, s.size_scalar_shrink_grow_sep, payload_size);
        uint64_t *runends = store.ptr + PayloadDiva::num_metadata_offset_words + PayloadDiva::infix_store_target_size / 64;
        uint64_t inserts[100], payloads[100][payload_size / 64 + 2];
        uint64_t read_payload[payload_size / 64 + 2];
        memset(payloads, 0, sizeof(payloads));

        inserts[0] = 0b0100000000001100;
        payloads[0][(payload_size - 10) / 64] = rng();
        inserts[1] = 0b0100000000001011;
        payloads[1][(payload_size - 10) / 64] = rng();
        inserts[2] = 0b0100000000001101;
        payloads[2][(payload_size - 10) / 64] = rng();
        inserts[3] = 0b0100000000001110;
        payloads[3][(payload_size - 10) / 64] = rng();
        s.InsertRawIntoInfixStore(store, inserts[0], infix_store_target_size, payloads[0]);
        s.InsertRawIntoInfixStore(store, inserts[2], infix_store_target_size, payloads[2]);
        s.InsertRawIntoInfixStore(store, inserts[1], infix_store_target_size, payloads[1]);
        s.InsertRawIntoInfixStore(store, inserts[3], infix_store_target_size, payloads[3]);
        SUBCASE("insertion and shifting of a single run") {
            for (int32_t i = 0; i < 4; i++) {
                REQUIRE_EQ(s.GetSlot(store, 538 + i), (inserts[i] & BITMASK(infix_size)));
                s.GetPayload(store, 538 + i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i], 0, payload_size));
            }
            for (int32_t i = 538; i < 541; i++)
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            REQUIRE_EQ(get_bitmap_bit(runends, 541), 1);
        }

        inserts[4] = 0b0011111111100001;
        payloads[4][(payload_size - 10) / 64] = rng();
        inserts[5] = 0b0011111111100010;
        payloads[5][(payload_size - 10) / 64] = rng();
        inserts[6] = 0b0011111111100100;
        payloads[6][(payload_size - 10) / 64] = rng();
        inserts[7] = 0b0011111111100011;
        payloads[7][(payload_size - 10) / 64] = rng();
        s.InsertRawIntoInfixStore(store, inserts[4], infix_store_target_size, payloads[4]);
        s.InsertRawIntoInfixStore(store, inserts[5], infix_store_target_size, payloads[5]);
        s.InsertRawIntoInfixStore(store, inserts[6], infix_store_target_size, payloads[6]);
        s.InsertRawIntoInfixStore(store, inserts[7], infix_store_target_size, payloads[7]);
        SUBCASE("insertion and shifting of a new run that shifts an old run") {
            for (int32_t i = 537; i < 540; i++) { 
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 537 + 4] & BITMASK(infix_size)));
                s.GetPayload(store, i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i - 537 + 4], 0, payload_size));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 540), (inserts[7] & BITMASK(infix_size)));
            s.GetPayload(store, 540, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[7], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 540), 1);
            for (int32_t i = 541; i < 544; i++) { 
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 541] & BITMASK(infix_size)));
                s.GetPayload(store, i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i - 541], 0, payload_size));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 544), (inserts[3] & BITMASK(infix_size)));
            s.GetPayload(store, 544, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[3], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 544), 1);
        }

        inserts[0] = 0b0000000000000001;
        payloads[0][(payload_size - 10) / 64] = rng();
        inserts[1] = 0b0000000000000001;
        payloads[1][(payload_size - 10) / 64] = rng();
        inserts[2] = 0b0000000000000010;
        payloads[2][(payload_size - 10) / 64] = rng();
        inserts[3] = 0b0000000000000011;
        payloads[3][(payload_size - 10) / 64] = rng();
        s.InsertRawIntoInfixStore(store, inserts[1], infix_store_target_size, payloads[1]);
        s.InsertRawIntoInfixStore(store, inserts[3], infix_store_target_size, payloads[3]);
        s.InsertRawIntoInfixStore(store, inserts[0], infix_store_target_size, payloads[0]);
        s.InsertRawIntoInfixStore(store, inserts[2], infix_store_target_size, payloads[2]);
        SUBCASE("insertion and shifting at the very beginning of the array") {
            for (int32_t i = 0; i < 3; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i] & BITMASK(infix_size)));
                s.GetPayload(store, i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i], 0, payload_size));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 3), (inserts[3] & BITMASK(infix_size)));
            s.GetPayload(store, 3, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[3], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 3), 1);
        }

        SUBCASE("inserting new runs in between two touching runs: 0 slots added") {
            REQUIRE_EQ(s.GetSlot(store, 4), 0b00000);
            s.GetPayload(store, 4, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[99], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 4), 0);
            REQUIRE_EQ(s.GetSlot(store, 5), 0b00000);
            s.GetPayload(store, 5, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[99], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 5), 0);
        }

        inserts[5] = 0b0000000001111111;
        payloads[5][(payload_size - 10) / 64] = rng();
        s.InsertRawIntoInfixStore(store, inserts[5], infix_store_target_size, payloads[5]);
        SUBCASE("inserting new runs in between two touching runs: 1 slots added") {
            REQUIRE_EQ(s.GetSlot(store, 4), (inserts[5] & BITMASK(infix_size)));
            s.GetPayload(store, 4, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[5], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 4), 1);
            REQUIRE_EQ(s.GetSlot(store, 5), 0b00000);
            s.GetPayload(store, 5, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[99], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 5), 0);
        }

        inserts[4] = 0b0000000000110101;
        payloads[4][(payload_size - 10) / 64] = rng();
        s.InsertRawIntoInfixStore(store, inserts[4], infix_store_target_size, payloads[4]);
        SUBCASE("inserting new runs in between two touching runs: 2 slots added") {
            REQUIRE_EQ(s.GetSlot(store, 4), (inserts[4] & BITMASK(infix_size)));
            s.GetPayload(store, 4, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[4], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 4), 1);
            REQUIRE_EQ(s.GetSlot(store, 5), (inserts[5] & BITMASK(infix_size)));
            s.GetPayload(store, 5, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[5], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 5), 1);
        }

        inserts[0] = 0b0111111110100001;
        payloads[0][(payload_size - 10) / 64] = rng();
        inserts[1] = 0b0111111110100010;
        payloads[1][(payload_size - 10) / 64] = rng();
        inserts[2] = 0b0111111110100011;
        payloads[2][(payload_size - 10) / 64] = rng();
        inserts[3] = 0b0111111110100111;
        payloads[3][(payload_size - 10) / 64] = rng();
        inserts[4] = 0b0111111111100001;
        payloads[4][(payload_size - 10) / 64] = rng();
        inserts[5] = 0b0111111111100001;
        payloads[5][(payload_size - 10) / 64] = rng();
        inserts[6] = 0b0111111111100010;
        payloads[6][(payload_size - 10) / 64] = rng();
        s.InsertRawIntoInfixStore(store, inserts[0], infix_store_target_size, payloads[0]);
        s.InsertRawIntoInfixStore(store, inserts[5], infix_store_target_size, payloads[5]);
        s.InsertRawIntoInfixStore(store, inserts[4], infix_store_target_size, payloads[4]);
        s.InsertRawIntoInfixStore(store, inserts[1], infix_store_target_size, payloads[1]);
        s.InsertRawIntoInfixStore(store, inserts[6], infix_store_target_size, payloads[6]);
        s.InsertRawIntoInfixStore(store, inserts[2], infix_store_target_size, payloads[2]);
        s.InsertRawIntoInfixStore(store, inserts[3], infix_store_target_size, payloads[3]);
        SUBCASE("insertion and shifting at the very end of the array") {
            for (int32_t i = 1070; i < 1073; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1070] & BITMASK(infix_size)));
                s.GetPayload(store, i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i - 1070], 0, payload_size));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1073), (inserts[3] & BITMASK(infix_size)));
            s.GetPayload(store, 1073, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[3], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 1073), 1);
            for (int32_t i = 1074; i < 1076; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1074 + 4] & BITMASK(infix_size)));
                s.GetPayload(store, i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i - 1074 + 4], 0, payload_size));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1076), (inserts[6] & BITMASK(infix_size)));
            s.GetPayload(store, 1076, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[6], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 1076), 1);
        }

        inserts[7] = 0b0111111111011101;
        payloads[7][(payload_size - 10) / 64] = rng();
        inserts[8] = 0b0111111111011110;
        payloads[8][(payload_size - 10) / 64] = rng();
        inserts[9] = 0b0111111111011111;
        payloads[9][(payload_size - 10) / 64] = rng();
        s.InsertRawIntoInfixStore(store, inserts[9], infix_store_target_size, payloads[9]);
        s.InsertRawIntoInfixStore(store, inserts[7], infix_store_target_size, payloads[7]);
        s.InsertRawIntoInfixStore(store, inserts[8], infix_store_target_size, payloads[8]);
        SUBCASE("insertion and shifting in between touching runs at the very end of the array") {
            for (int32_t i = 1067; i < 1070; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1067] & BITMASK(infix_size)));
                s.GetPayload(store, i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i - 1067], 0, payload_size));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1070), (inserts[3] & BITMASK(infix_size)));
            s.GetPayload(store, 1070, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[3], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 1070), 1);
            for (int32_t i = 1071; i < 1073; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1071 + 7] & BITMASK(infix_size)));
                s.GetPayload(store, i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i - 1071 + 7], 0, payload_size));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1073), (inserts[9] & BITMASK(infix_size)));
            s.GetPayload(store, 1073, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[9], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 1073), 1);
            for (int32_t i = 1074; i < 1076; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 1074 + 4] & BITMASK(infix_size)));
                s.GetPayload(store, i, read_payload);
                REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[i - 1074 + 4], 0, payload_size));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 1076), (inserts[6] & BITMASK(infix_size)));
            s.GetPayload(store, 1076, read_payload);
            REQUIRE(compare_bitmap_to_bitmap(read_payload, 0, payloads[6], 0, payload_size));
            REQUIRE_EQ(get_bitmap_bit(runends, 1076), 1);
        }
    }


    static void PayloadsGetInfixList() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const uint32_t payload_size = 100;
        const uint32_t infix_store_target_size = PayloadDiva::infix_store_target_size;
        const float load_factor = 0.95;

        const uint32_t rng_seed = 20;
        std::mt19937_64 rng(rng_seed);

        PayloadDiva s(infix_size, seed, load_factor, payload_size);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        PayloadDiva::InfixStore store(total_slots, s.infix_size_, s.size_scalar_shrink_grow_sep, payload_size);

        const std::vector<uint64_t> keys {0b000000000000001,
            0b000000000000101, 0b000000000010101,
            0b000000000100001, 0b000000000100011,
            0b000000000100101, 0b001000000100001,
            0b001000000100011, 0b001000000100101,
            0b001000000100110, 0b001000000100110,
            0b001000000100110, 0b001000001100001,
            0b001000001100011, 0b001000001100101,
            0b001111111000001, 0b001111111000010,
            0b001111111000010, 0b001111111100001,
            0b001111111100010, 0b001111111100010};
        uint64_t payloads[keys.size() + 1][payload_size / 64 + 2];
        for (uint32_t i = 0; i < keys.size(); i++) {
            for (uint32_t j = 0; j < payload_size / 64 + 2; j++)
                payloads[i][j] = rng();
        }
        SUBCASE("get infix list") {
            for (uint32_t i = 0; i < keys.size(); i++) {
                const uint64_t key = keys[i];
                s.InsertRawIntoInfixStore(store, key, infix_store_target_size, payloads[i]);
            }

            // Adjust the payloads with the same infixes that have to be reversed
            int32_t adjust_l = 0;
            for (int32_t i = 1; i < keys.size(); i++) {
                if (keys[i] != keys[i - 1]) {
                    for (int32_t j = 0; j < (i - adjust_l) / 2; j++)
                        for (int32_t k = 0; k < payload_size / 64 + 2; k++)
                            std::swap(payloads[adjust_l + j][k], payloads[i - j - 1][k]);
                    adjust_l = i;
                }
            }
            if (adjust_l < keys.size() - 1) {
                for (int32_t j = 0; j < (keys.size() - adjust_l) / 2; j++)
                    for (int32_t k = 0; k < payload_size / 64 + 2; k++)
                        std::swap(payloads[adjust_l + j][k], payloads[keys.size() - j - 1][k]);
            }

            uint64_t res[keys.size() + 1], read_payloads[(keys.size() * payload_size + 63) / 64];
            const uint32_t len = s.GetInfixList(store, res, read_payloads);
            REQUIRE_EQ(len, keys.size());
            for (int32_t i = 0; i < keys.size(); i++) {
                REQUIRE_EQ(res[i], keys[i]);
                REQUIRE(compare_bitmap_to_bitmap(read_payloads, payload_size * i, payloads[i], 0, payload_size));
            }
        }
    }


    static void PayloadsLoadInfixList() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const uint32_t payload_size = 100;
        const uint32_t infix_store_target_size = PayloadDiva::infix_store_target_size;
        const float load_factor = 0.95;

        const uint32_t rng_seed = 20;
        std::mt19937_64 rng(rng_seed);

        PayloadDiva s(infix_size, seed, load_factor, payload_size);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        PayloadDiva::InfixStore store(total_slots, s.infix_size_, s.size_scalar_shrink_grow_sep, payload_size);

        uint64_t payloads[(infix_store_target_size + 1) * (payload_size / 64 + 2)];
        for (uint32_t i = 0; i < (infix_store_target_size + 1) * (payload_size / 64 + 2); i++)
            payloads[i] = rng();

        SUBCASE("fetch") {
            const std::vector<uint64_t> keys {0b0000000000000001,
                0b0000000000000101, 0b0000000000010101,
                0b0000000000100001, 0b0000000000100011,
                0b0000000000100101, 0b0001000000100001,
                0b0001000000100011, 0b0001000000100110,
                0b0001000000100110, 0b0001000000100110,
                0b0001000000100101, 0b0001000001100001,
                0b0001000001100011, 0b0001000001100101,
                0b0001111111000010, 0b0001111111000010,
                0b0001111111000001, 0b0001111111100010,
                0b0001111111100010, 0b0001111111100001,
                0b0111111111000010, 0b0111111111000010,
                0b0111111111000001, 0b0111111111100010,
                0b0111111111100010, 0b0111111111100001};
            s.LoadListToInfixStore(store, keys.data(), keys.size(), infix_store_target_size, true, payloads);
            uint64_t res[keys.size() + 1], read_payloads[(keys.size() * payload_size + 63) / 64];
            const uint32_t len = s.GetInfixList(store, res, read_payloads);
            REQUIRE_EQ(len, keys.size());
            for (int32_t i = 0; i < keys.size(); i++) {
                REQUIRE_EQ(res[i], keys[i]);
                REQUIRE(compare_bitmap_to_bitmap(read_payloads, payload_size * i, payloads, payload_size * i, payload_size));
            }
        }

        SUBCASE("vs. insert one by one") {
            const uint32_t n_keys = infix_store_target_size;
            const uint32_t rng_seed = 1;
            std::mt19937_64 rng(rng_seed);
            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++) {
                keys.push_back((rng() & BITMASK(PayloadDiva::base_implicit_size + infix_size)) | 1ULL);

                uint64_t tmp_payload[payload_size / 64 + 2];
                copy_bitmap_to_bitmap(payloads, payload_size * i, tmp_payload, 0, payload_size);
                s.InsertRawIntoInfixStore(store, keys[keys.size() - 1], infix_store_target_size, tmp_payload);
            }
            std::sort(keys.begin(), keys.end(),
                    [&](uint64_t a, uint64_t b) { return s.CompareInfixes(a, b); });
            s.LoadListToInfixStore(store, keys.data(), keys.size(), infix_store_target_size, true, payloads);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("payloads/load_infix_list");
            uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
            uint64_t *check_payloads[infix_store_target_size + 100];
            for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                check_payloads[i] = &(check_payloads_contents[i][0]);
            ReadStorePayloadsFromFile("payloads/load_infix_list", payload_size, check_payloads);
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }
    }


    static void PayloadsDeleteRaw() {
        const uint32_t infix_size = 5;
        const uint32_t payload_size = 100;
        const uint32_t infix_store_target_size = PayloadDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;

        PayloadDiva s(infix_size, seed, load_factor, payload_size);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        PayloadDiva::InfixStore store(total_slots, s.infix_size_, s.size_scalar_shrink_grow_sep, payload_size);

        const uint32_t rng_seed = 20;
        std::mt19937_64 rng(rng_seed);

        std::vector<uint64_t> keys {0b000000010011000,
            0b000000010010100, 0b000000010010110,
            0b000000010010101, 0b000000010011111,
            0b000000010110101, 0b000000010110111,
            0b000000010111001, 0b000000011111011,
            0b000000100011011, 0b000000100011111,
            0b000000111100001, 0b000000111100011,
            0b000000111100111, 0b111111010000001,
            0b111111101100101, 0b111111101100111,
            0b111111110011111, 0b111111110110101,
            0b111111111011000, 0b111111111010100,
            0b111111111010110, 0b111111111010101,
            0b111111111011111, 0b111111111100001,
            0b111111111100011};
        while (keys.size() < infix_store_target_size) {
            const uint64_t candidate = rng() & BITMASK(Diva<>::base_implicit_size + infix_size);
            if (candidate & BITMASK(infix_size))
                keys.push_back(candidate);
        }
        std::sort(keys.begin(), keys.end(), 
                [&](uint64_t a, uint64_t b) { return s.CompareInfixes(a, b); });

        uint64_t payloads[(keys.size() + 1) * (payload_size / 64 + 2)];
        for (uint32_t i = 0; i < (keys.size() + 1) * (payload_size / 64 + 2); i++)
            payloads[i] = rng();

        s.LoadListToInfixStore(store, keys.data(), keys.size(), infix_store_target_size, true, payloads);

        SUBCASE("single match, shift left") {
            s.DeleteRawFromInfixStore(store, 0b000000010011111);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("payloads/delete/single_match/shift_left");
            uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
            uint64_t *check_payloads[infix_store_target_size + 100];
            for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                check_payloads[i] = &(check_payloads_contents[i][0]);
            ReadStorePayloadsFromFile("payloads/delete/single_match/shift_left", payload_size, check_payloads);
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }

        SUBCASE("choose from multiple matches, shift left") {
            s.DeleteRawFromInfixStore(store, 0b000000010010101, infix_store_target_size, 
                    [](const uint64_t *payload) { return payload[0] == 0xb45c4694c48e921f; });
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/multiple_matches/shift_left/1");
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                ReadStorePayloadsFromFile("payloads/delete/multiple_matches/shift_left/1", payload_size, check_payloads);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
            }

            s.DeleteRawFromInfixStore(store, 0b000000010010101);
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/multiple_matches/shift_left/2");
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                ReadStorePayloadsFromFile("payloads/delete/multiple_matches/shift_left/2", payload_size, check_payloads);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
            }
        }

        SUBCASE("destroy run, shift left") {
            s.DeleteRawFromInfixStore(store, 0b00000011111011, infix_store_target_size, 
                                      [](const uint64_t *payload) { return payload[0] == 0x10c8fd5b4b0459bc; });
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/destroy_run/shift_left/1");
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                ReadStorePayloadsFromFile("payloads/delete/destroy_run/shift_left/1", payload_size, check_payloads);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
            }
            s.DeleteRawFromInfixStore(store, 0b00000011111011, infix_store_target_size, 
                                      [](const uint64_t *payload) { return payload[0] == 0x19b28ae9e8437575; });
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/destroy_run/shift_left/2");
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                ReadStorePayloadsFromFile("payloads/delete/destroy_run/shift_left/2", payload_size, check_payloads);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
            }
        }

        SUBCASE("end of run, shift left") {
            s.DeleteRawFromInfixStore(store, 0b000000010011111, infix_store_target_size, 
                                      [](const uint64_t *payload) { return payload[0] == 0xfb9c2e1f14d77d65; });
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("payloads/delete/end_of_run/shift_left");
            uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
            uint64_t *check_payloads[infix_store_target_size + 100];
            for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                check_payloads[i] = &(check_payloads_contents[i][0]);
            ReadStorePayloadsFromFile("payloads/delete/end_of_run/shift_left", payload_size, check_payloads);
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }

        SUBCASE("single match, shift right") {
            s.DeleteRawFromInfixStore(store, 0b111111111011111, infix_store_target_size, 
                                      [](const uint64_t *payload) { return payload[0] == 0xbfe6180250364ecf; });
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("payloads/delete/single_match/shift_right");
            uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
            uint64_t *check_payloads[infix_store_target_size + 100];
            for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                check_payloads[i] = &(check_payloads_contents[i][0]);
            ReadStorePayloadsFromFile("payloads/delete/single_match/shift_right", payload_size, check_payloads);
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }

        SUBCASE("multiple matches, shift right") {
            s.DeleteRawFromInfixStore(store, 0b111111111010101, infix_store_target_size, 
                                      [](const uint64_t *payload) { return payload[0] == 0x5a53fa59c2975c9; });
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/multiple_matches/shift_right/1");
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                ReadStorePayloadsFromFile("payloads/delete/multiple_matches/shift_right/1", payload_size, check_payloads);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
            }
            s.DeleteRawFromInfixStore(store, 0b111111111010101);
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/multiple_matches/shift_right/2");
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                ReadStorePayloadsFromFile("payloads/delete/multiple_matches/shift_right/2", payload_size, check_payloads);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
            }
        }

        SUBCASE("destroy run, shift right") {
            s.DeleteRawFromInfixStore(store, 0b111111110110101, infix_store_target_size, 
                                      [](const uint64_t *payload) { return payload[0] == 0xb71307c72019bed6; });
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/destroy_run/shift_right/1");
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                ReadStorePayloadsFromFile("payloads/delete/destroy_run/shift_right/1", payload_size, check_payloads);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
            }
            s.DeleteRawFromInfixStore(store, 0b111111110110101, infix_store_target_size, 
                                      [](const uint64_t *payload) { return payload[0] == 0xa9822db95c22d90e; });
            {
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("payloads/delete/destroy_run/shift_right/2");
                uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
                uint64_t *check_payloads[infix_store_target_size + 100];
                for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                    check_payloads[i] = &(check_payloads_contents[i][0]);
                ReadStorePayloadsFromFile("payloads/delete/destroy_run/shift_right/2", payload_size, check_payloads);
                AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
            }
        }

        SUBCASE("end of run, shift right") {
            s.DeleteRawFromInfixStore(store, 0b111111111100011, infix_store_target_size, 
                                      [](const uint64_t *payload) { return payload[0] == 0xa84843ffeafb62b2; });
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("payloads/delete/end_of_run/shift_right");
            uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
            uint64_t *check_payloads[infix_store_target_size + 100];
            for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                check_payloads[i] = &(check_payloads_contents[i][0]);
            ReadStorePayloadsFromFile("payloads/delete/end_of_run/shift_right", payload_size, check_payloads);
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }

        uint64_t lone_payload[payload_size / 64 + 2];
        for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
            lone_payload[i] = rng();
        s.InsertRawIntoInfixStore(store, 0b010000000010101, infix_store_target_size, lone_payload);
        SUBCASE("lone run with single slot") {
            s.DeleteRawFromInfixStore(store, 0b010000000010101, infix_store_target_size, 
                                      [=](const uint64_t *payload) { return payload[0] == lone_payload[0]; });
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("payloads/delete/lone_run_with_single_slot");
            uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
            uint64_t *check_payloads[infix_store_target_size + 100];
            for (uint32_t i = 0; i < infix_store_target_size + 100; i++)
                check_payloads[i] = &(check_payloads_contents[i][0]);
            ReadStorePayloadsFromFile("payloads/delete/lone_run_with_single_slot", payload_size, check_payloads);
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }
    }


    static void PayloadsGetLongestMatchingInfixSize() {
        const uint32_t infix_size = 5;
        const uint32_t payload_size = 100;
        const uint32_t infix_store_target_size = PayloadDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;

        PayloadDiva s(infix_size, seed, load_factor, payload_size);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        PayloadDiva::InfixStore store(total_slots, s.infix_size_, s.size_scalar_shrink_grow_sep, payload_size);

        const uint32_t rng_seed = 20;
        std::mt19937_64 rng(rng_seed);

        const std::vector<uint64_t> keys {0b000000010011000,
            0b000000010010100, 0b000000010010110,
            0b000000010010101, 0b000000010011111,
            0b000000010110101, 0b000000010110111,
            0b000000010111001, 0b000000011111011,
            0b000000100011011, 0b000000100011111,
            0b000000111100001, 0b000000111100011,
            0b000000111100111, 0b000100111110000,
            0b000100111110010, 0b000100111110011,
            0b111111010000001, 0b111111101100101,
            0b111111101100111, 0b111111110011111,
            0b111111110110101, 0b111111111011000,
            0b111111111010100, 0b111111111010110,
            0b111111111010101, 0b111111111011111,
            0b111111111100001, 0b111111111100011};
        uint64_t payloads[(keys.size() + 1) * (payload_size / 64 + 2)];
        for (uint32_t i = 0; i < (keys.size() + 1) * (payload_size / 64 + 2); i++)
            payloads[i] = rng();

        s.LoadListToInfixStore(store, keys.data(), keys.size(), infix_store_target_size, true, payloads);

        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110011, infix_store_target_size,
                    [] (const uint64_t *payload) { return true; }),
                infix_size - 1);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110011, infix_store_target_size,
                    [] (const uint64_t *payload) { return payload[0] == 0x8edaa78f2fc77d78; }),
                infix_size - 1);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110011, infix_store_target_size,
                    [] (const uint64_t *payload) { return payload[0] == 0x513480a1777ab79; }),
                infix_size - 2);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110011, infix_store_target_size,
                    [] (const uint64_t *payload) { return payload[0] == 0x88158666844a4c73; }),
                infix_size - 5);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110011, infix_store_target_size,
                    [] (const uint64_t *payload) { return false; }),
                -1);

        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110001, infix_store_target_size,
                    [] (const uint64_t *payload) { return true; }),
                infix_size - 2);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110001, infix_store_target_size,
                    [] (const uint64_t *payload) { return payload[0] == 0x513480a1777ab79; }),
                infix_size - 2);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110001, infix_store_target_size,
                    [] (const uint64_t *payload) { return payload[0] == 0x88158666844a4c73; }),
                infix_size - 5);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110001, infix_store_target_size,
                    [] (const uint64_t *payload) { return false; }),
                -1);

        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111100001, infix_store_target_size,
                    [] (const uint64_t *payload) { return true; }),
                infix_size - 5);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111100001, infix_store_target_size,
                    [] (const uint64_t *payload) { return payload[0] == 0x88158666844a4c73; }),
                infix_size - 5);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111100001, infix_store_target_size,
                    [] (const uint64_t *payload) { return false; }),
                -1);

        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b111111111111111, infix_store_target_size,
                    [] (const uint64_t *payload) { return true; }),
                -1);
    }


    static void PayloadsResize() {
        const uint32_t infix_size = 5;
        const uint32_t payload_size = 100;
        const uint32_t infix_store_target_size = PayloadDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);
        
        PayloadDiva s(infix_size, seed, load_factor, payload_size);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        PayloadDiva::InfixStore store(total_slots, s.infix_size_, s.size_scalar_shrink_grow_sep, payload_size);

        SUBCASE("expand") {
            const uint32_t n_keys = s.scaled_sizes_[s.size_scalar_shrink_grow_sep] - 1;
            for (int32_t i = 0; i < n_keys; i++) {
                uint64_t payload[payload_size / 64 + 2];
                for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                    payload[i] = rng();
                s.InsertRawIntoInfixStore(store, (rng() & BITMASK(Diva<>::base_implicit_size + infix_size)) | 1ULL,
                                          infix_store_target_size, payload);
            }
            s.ResizeInfixStore(store);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("payloads/resize/expand");
            const uint32_t payload_count = 2 * infix_store_target_size;
            uint64_t check_payloads_contents[payload_count][payload_size / 64 + 2] = {};
            uint64_t *check_payloads[payload_count];
            for (uint32_t i = 0; i < payload_count; i++)
                check_payloads[i] = &(check_payloads_contents[i][0]);
            ReadStorePayloadsFromFile("payloads/resize/expand", payload_size, check_payloads);
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }
        SUBCASE("contract") {
            const uint32_t n_keys = s.scaled_sizes_[s.size_scalar_shrink_grow_sep] - 500;
            for (int32_t i = 0; i < n_keys; i++) {
                uint64_t payload[payload_size / 64 + 2];
                for (uint32_t i = 0; i < payload_size / 64 + 2; i++)
                    payload[i] = rng();
                s.InsertRawIntoInfixStore(store, (rng() & BITMASK(Diva<>::base_implicit_size + infix_size)) | 1ULL,
                                          infix_store_target_size, payload);
            }
            s.ResizeInfixStore(store);

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("payloads/resize/contract");
            const uint32_t payload_count = infix_store_target_size + 100;
            uint64_t check_payloads_contents[infix_store_target_size + 100][payload_size / 64 + 2] = {};
            uint64_t *check_payloads[payload_count];
            for (uint32_t i = 0; i < payload_count; i++)
                check_payloads[i] = &(check_payloads_contents[i][0]);
            ReadStorePayloadsFromFile("payloads/resize/contract", payload_size, check_payloads);
            AssertStoreContents(s, store, occupieds_pos, checks, check_payloads);
        }
    }


    static void BinaryTrieLoadInfixVector() {
        const uint32_t N = 600;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 16;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size = BinaryTrieDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint8_t keys_contents[N][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N];
        uint64_t infixes[N];
        for (uint32_t i = 0; i < N; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
            infixes[i] = (rng() & BITMASK(highbit_pos(infix_store_target_size) + infix_size)) | 1;
        }
        std::sort(infixes, infixes + N);
        std::sort(keys, keys + N);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        for (uint32_t i = 0; i < N; i++) {
            uint32_t num_keys_in_infix = 1;
            while (i + num_keys_in_infix < N && infixes[i + num_keys_in_infix] == infixes[i])
                num_keys_in_infix++;
            num_keys_in_infix = std::max(num_keys_in_infix,
                                         std::min<uint32_t>(rng() % max_num_keys_in_infix + 1, N - i));
            infix_vec.emplace_back(infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + i, num_keys_in_infix, key_start_bit, infix_size);
            i += num_keys_in_infix - 1;
        }

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        const auto [occupieds_pos, checks] = 
            ReadStoreContentsFromFile("binary_trie/load_infix_vector");
        AssertStoreContents(s, store, occupieds_pos, checks);
    }


    static void BinaryTrieGetInfixVector() {
        const uint32_t N = 600;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 16;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size = BinaryTrieDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint8_t keys_contents[N][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N];
        uint64_t infixes[N];
        for (uint32_t i = 0; i < N; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
            infixes[i] = (rng() & BITMASK(highbit_pos(infix_store_target_size) + infix_size)) | 1;
        }
        std::sort(infixes, infixes + N);
        std::sort(keys, keys + N);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        for (uint32_t i = 0; i < N; i++) {
            uint32_t num_keys_in_infix = 1;
            while (i + num_keys_in_infix < N && infixes[i + num_keys_in_infix] == infixes[i])
                num_keys_in_infix++;
            num_keys_in_infix = std::max(num_keys_in_infix,
                                         std::min<uint32_t>(rng() % max_num_keys_in_infix + 1, N - i));
            infix_vec.emplace_back(infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + i, num_keys_in_infix, key_start_bit, infix_size);
            i += num_keys_in_infix - 1;
        }

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        auto reconstructed_infix_vec = s.GetInfixVector(store);
        REQUIRE_EQ(reconstructed_infix_vec.size(), infix_vec.size());
        for (int32_t i = 0; i < infix_vec.size(); i++)
            REQUIRE_EQ(reconstructed_infix_vec[i], infix_vec[i]);
    }


    static void BinaryTrieInsertRaw() {
        const uint32_t N_bulk = 6;
        const uint32_t N_bulk_keys = 60;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 10;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size = BinaryTrieDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint64_t bulk_infixes[N_bulk] = {0b0000000000000011,
            0b0000000001000011, 0b0011111111100011,
            0b0100000000101101, 0b0111111110011111,
            0b0111111111011111};
        uint8_t keys_contents[N_bulk_keys][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N_bulk_keys];
        for (int32_t i = 0; i < N_bulk_keys; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
        }
        std::sort(keys, keys + N_bulk_keys);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        int32_t key_ind = 0;
        for (int32_t i = 0; i < N_bulk; i++) {
            uint32_t num_keys_in_infix = std::min<uint32_t>(rng() % max_num_keys_in_infix + 1,
                                                            N_bulk_keys - key_ind);
            infix_vec.emplace_back(bulk_infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + key_ind, num_keys_in_infix,
                    key_start_bit, infix_size);
            key_ind += num_keys_in_infix;
        }

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        {
            uint8_t original_key[1] = {0b10000000};
            s.InsertRawIntoInfixStore(store, 0b0100000000101101, infix_store_target_size,
                                    nullptr,
                                    {original_key, 8 * sizeof(original_key)},
                                    0);
        }
        SUBCASE("inserting new suffix into a single run") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_suffix_into_single_run");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b0100000011001101);
        SUBCASE("inserting new suffix after run") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_suffix_after_run");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        {
            uint8_t original_key[1] = {0b00000000};
            s.InsertRawIntoInfixStore(store, 0b0011111111100011, infix_store_target_size,
                                    nullptr,
                                    {original_key, 8 * sizeof(original_key)},
                                    0);
        }
        SUBCASE("switch encoding and shift next run") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/switch_encoding_shift_next_run");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b0000000000000001);
        SUBCASE("inserting new infix before trie") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_infix_before_trie");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b0000000000011111);
        SUBCASE("inserting new infix after trie") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_infix_after_trie");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        {
            uint8_t original_key[2] = {0b00001000, 0b10101010};
            s.InsertRawIntoInfixStore(store, 0b0000000000000011, infix_store_target_size,
                                    nullptr,
                                    {original_key, 8 * sizeof(original_key)}, 0);
        }
        SUBCASE("inserting new suffix and shifting at the beginning") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_suffix_shifting_at_beginning");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b0000000000101011);
        SUBCASE("inserting new infix between touching runs") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_infix_between_touching_runs");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        {
            uint8_t original_key[1] = {0b10101010};
            s.InsertRawIntoInfixStore(store, 0b0000000000101011, infix_store_target_size,
                                    nullptr,
                                    {original_key, 8 * sizeof(original_key)},
                                    0);
        }
        SUBCASE("convert infix into trie") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/convert_infix_into_trie");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b0111111111110101);
        SUBCASE("inserting new infix after last run at the end") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_infix_after_last_run_at_end");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b0111110010111011);
        SUBCASE("inserting new infix just before end cluster") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_infix_before_end_cluster");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        {
            uint8_t original_key[2] = {0b00001000, 0b10101010};
            s.InsertRawIntoInfixStore(store, 0b0111111111110101, infix_store_target_size,
                                    nullptr,
                                    {original_key, 8 * sizeof(original_key)},
                                    0);
        }
        SUBCASE("convert infix to trie at the end") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/convert_infix_to_trie_at_end");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b0111111111111101);
        SUBCASE("inserting new infix after trie at the end") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_infix_after_trie_at_end");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b0111111111000101);
        SUBCASE("inserting new infix before trie at the end") {
            auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/insert/new_infix_before_trie_at_end");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }


    static void BinaryTriePointQuery() {
        const uint32_t N = 600;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 16;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size = BinaryTrieDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint8_t keys_contents[N][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N];
        uint64_t infixes[N];
        for (uint32_t i = 0; i < N; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
            infixes[i] = (rng() & BITMASK(highbit_pos(infix_store_target_size) + infix_size)) | 1;
        }
        std::sort(infixes, infixes + N);
        std::sort(keys, keys + N);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        for (uint32_t i = 0; i < N; i++) {
            uint32_t num_keys_in_infix = 1;
            while (i + num_keys_in_infix < N && infixes[i + num_keys_in_infix] == infixes[i])
                num_keys_in_infix++;
            num_keys_in_infix = std::max(num_keys_in_infix,
                                         std::min<uint32_t>(rng() % max_num_keys_in_infix + 1, N - i));
            infix_vec.emplace_back(infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + i, num_keys_in_infix, key_start_bit, infix_size);
            i += num_keys_in_infix - 1;
        }

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        SUBCASE("no false negatives") {
            int32_t key_ind = 0, infix_cnt = 0;
            for (auto& infix : infix_vec) {
                for (int32_t i = 0; i < infix.num_suffixes_; i++) {
                    REQUIRE(s.PointQueryInfixStore(store, infix.infix_,
                                infix_store_target_size,
                                keys[key_ind],
                                key_start_bit));
                    key_ind++;
                }
                infix_cnt++;
            }
        }

        SUBCASE("negatives") {
            {
                const uint8_t original_key[3] = {0b11111010, 0b11110000, 0b00110011};
                REQUIRE_FALSE(s.PointQueryInfixStore(store, 0b001000110001101,
                            infix_store_target_size,
                            {original_key, 8 * sizeof(original_key)},
                            0));
            }
            {
                const uint8_t original_key[3] = {0b01111111, 0b11110000, 0b00110011};
                REQUIRE_FALSE(s.PointQueryInfixStore(store, 0b000011000101101,
                            infix_store_target_size,
                            {original_key, 8 * sizeof(original_key)},
                            0));
            }
            {
                const uint8_t original_key[3] = {0b10010100, 0b00001011, 0b11111111};
                REQUIRE_FALSE(s.PointQueryInfixStore(store, 0b100000010011011,
                            infix_store_target_size,
                            {original_key, 8 * sizeof(original_key)},
                            0));
            }
        }

        SUBCASE("false positives") {
            {
                const uint8_t original_key[3] = {0b00101010, 0b11110000, 0b00110011};
                REQUIRE(s.PointQueryInfixStore(store, 0b001000110001101,
                            infix_store_target_size,
                            {original_key, 8 * sizeof(original_key)},
                            0));
            }
            {
                const uint8_t original_key[3] = {0b01111111, 0b11110000, 0b00110011};
                REQUIRE(s.PointQueryInfixStore(store, 0b011011000101101,
                            infix_store_target_size,
                            {original_key, 8 * sizeof(original_key)},
                            0));
            }
            {
                const uint8_t original_key[3] = {0b10010100, 0b10101011, 0b11111111};
                REQUIRE(s.PointQueryInfixStore(store, 0b100000010011011,
                            infix_store_target_size,
                            {original_key, 8 * sizeof(original_key)},
                            0));
            }
        }
    }


    static void BinaryTrieRangeQuery() {
        const uint32_t N = 600;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 16;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size = BinaryTrieDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint8_t keys_contents[N][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N];
        uint64_t infixes[N];
        for (uint32_t i = 0; i < N; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
            infixes[i] = (rng() & BITMASK(highbit_pos(infix_store_target_size) + infix_size)) | 1;
        }
        std::sort(infixes, infixes + N);
        std::sort(keys, keys + N);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        for (uint32_t i = 0; i < N; i++) {
            uint32_t num_keys_in_infix = 1;
            while (i + num_keys_in_infix < N && infixes[i + num_keys_in_infix] == infixes[i])
                num_keys_in_infix++;
            num_keys_in_infix = std::max(num_keys_in_infix,
                                         std::min<uint32_t>(rng() % max_num_keys_in_infix + 1, N - i));
            infix_vec.emplace_back(infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + i, num_keys_in_infix, key_start_bit, infix_size);
            i += num_keys_in_infix - 1;
        }

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        SUBCASE("no false negatives") {
            int32_t key_ind = 0;
            for (auto& infix : infix_vec) {
                for (int32_t i = 0; i < infix.num_suffixes_; i++) {
                    const uint32_t current_key_len = keys[key_ind].length / 8;
                    uint8_t key_l[current_key_len], key_r[current_key_len];
                    memcpy(key_l, keys[key_ind].str, current_key_len);
                    memcpy(key_r, keys[key_ind].str, current_key_len);
                    for (int32_t j = 0; j < current_key_len; j++) {
                        int32_t rand_pos = std::max((key_start_bit + 1) / 8, rand() % current_key_len);
                        if (std::numeric_limits<uint8_t>::min() < keys[key_ind].str[rand_pos] 
                                && keys[key_ind].str[rand_pos] < std::numeric_limits<uint8_t>::max()) {
                            key_l[rand_pos]--;
                            key_r[rand_pos]++;
                            break;
                        }
                    }
                    REQUIRE(s.RangeQueryInfixStore(store, infix.infix_, infix.infix_,
                                infix_store_target_size, 
                                {key_l, 8 * current_key_len},
                                {key_r, 8 * current_key_len},
                                key_start_bit));
                    key_ind++;
                }
            }

            SUBCASE("two runs true false") {
                const uint8_t original_key_l[2] = {0b00100001, 0b00011010};
                const uint8_t original_key_r[2] = {0b00100001, 0b00000000};
                REQUIRE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000111101010111,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("two runs false true") {
                const uint8_t original_key_l[2] = {0b00100101, 0b00011010};
                const uint8_t original_key_r[2] = {0b00100001, 0b10100100};
                REQUIRE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000111101010111,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("three non-empty runs") {
                const uint8_t original_key_l[2] = {0b00100101, 0b00011010};
                const uint8_t original_key_r[2] = {0b00111111, 0b11111111};
                REQUIRE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b0001000110001101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("two runs with single leaf trie in the second") {
                const uint8_t original_key_l[2] = {0b00100111, 0b11110101};
                const uint8_t original_key_r[2] = {0b01111111, 0b11111111};
                REQUIRE(s.RangeQueryInfixStore(store, 0b000111101010111, 0b0001000110001101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
        }

        SUBCASE("negatives") {
            SUBCASE("diverge left") {
                const uint8_t original_key_l[2] = {0b00100001, 0b00010000};
                const uint8_t original_key_r[2] = {0b00100001, 0b00010110};
                REQUIRE_FALSE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000110000101101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("diverge right") {
                const uint8_t original_key_l[2] = {0b00100000, 0b11010101};
                const uint8_t original_key_r[2] = {0b00100000, 0b11110000};
                REQUIRE_FALSE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000110000101101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("diverge both ways") {
                const uint8_t original_key_l[2] = {0b00100000, 0b11010101};
                const uint8_t original_key_r[2] = {0b00100001, 0b00010010};
                REQUIRE_FALSE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000110000101101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("diverge leftmost") {
                const uint8_t original_key_l[2] = {0b00010000, 0b11010101};
                const uint8_t original_key_r[2] = {0b00011000, 0b00010010};
                REQUIRE_FALSE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000110000101101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("diverge rightmost") {
                const uint8_t original_key_l[2] = {0b00100100, 0b11010101};
                const uint8_t original_key_r[2] = {0b00110010, 0b00010010};
                REQUIRE_FALSE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000110000101101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("two runs") {
                const uint8_t original_key_l[2] = {0b00100110, 0b11010101};
                const uint8_t original_key_r[2] = {0b00100000, 0b00010010};
                REQUIRE_FALSE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000111101010111,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
        }

        SUBCASE("false positives") {
            SUBCASE("one run") {
                const uint8_t original_key_l[2] = {0b00100000, 0b10111000};
                const uint8_t original_key_r[2] = {0b00100000, 0b10111111};
                REQUIRE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000110000101101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("two runs false true") {
                const uint8_t original_key_l[2] = {0b00100111, 0b11110101};
                const uint8_t original_key_r[2] = {0b00000000, 0b00000000};
                REQUIRE(s.RangeQueryInfixStore(store, 0b000111101010111, 0b0001000110001101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("two runs true false") {
                const uint8_t original_key_l[2] = {0b00100001, 0b00011111};
                const uint8_t original_key_r[2] = {0b00100000, 0b10000000};
                REQUIRE(s.RangeQueryInfixStore(store, 0b000110000101101, 0b000111101010111,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
            SUBCASE("two runs true true") {
                const uint8_t original_key_l[2] = {0b00100110, 0b11111111};
                const uint8_t original_key_r[2] = {0b00000000, 0b00000000};
                REQUIRE(s.RangeQueryInfixStore(store, 0b000111101010111, 0b0001000110001101,
                            infix_store_target_size,
                            {original_key_l, 8 * sizeof(original_key_l)},
                            {original_key_r, 8 * sizeof(original_key_r)},
                            0));
            }
        }
    }


    static void BinaryTrieDeleteRaw() {
        const uint32_t N_bulk = 6;
        const uint32_t N_bulk_keys = 61;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 10;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size = BinaryTrieDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint64_t bulk_infixes[N_bulk] = {0b0000000000000011,
            0b0000000001000011, 0b0011111111100111,
            0b0100000000101101, 0b0111111110011111,
            0b0111111111011111};
        uint8_t keys_contents[N_bulk_keys][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N_bulk_keys];
        for (int32_t i = 0; i < N_bulk_keys - 1; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
        }
        keys_contents[N_bulk_keys - 1][0] = 0b01000100;
        keys_contents[N_bulk_keys - 1][1] = 0b00110101;
        keys_contents[N_bulk_keys - 1][2] = 0b01001100;
        keys[N_bulk_keys - 1] = {keys_contents[N_bulk_keys - 1], 24};
        std::sort(keys, keys + N_bulk_keys);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        int32_t key_ind = 0;
        for (int32_t i = 0; i < N_bulk; i++) {
            uint32_t num_keys_in_infix = std::min<uint32_t>(rng() % max_num_keys_in_infix + 1,
                                                            N_bulk_keys - key_ind);
            num_keys_in_infix += (i == 3);      // Manually add the prefix key
            infix_vec.emplace_back(bulk_infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + key_ind, num_keys_in_infix,
                                                  key_start_bit, infix_size);
            key_ind += num_keys_in_infix;
        }
        infix_vec.insert(infix_vec.begin() + 2,  {0b0011111111100110});
        infix_vec.insert(infix_vec.begin() + 4,  {0b0100000000100101});
        infix_vec.insert(infix_vec.begin() + 6,  {0b0100000000111111});

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        SUBCASE("single match, shift left") {
            {
                const uint8_t original_key[2] = {0b00001000, 0b01000000};
                s.DeleteRawFromInfixStore(store, 0b0000000000000011,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("binary_trie/delete/single_match/shift_left/1");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            {
                const uint8_t original_key[2] = {0b00001000, 0b00000000};
                s.DeleteRawFromInfixStore(store, 0b0000000000000011,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("binary_trie/delete/single_match/shift_left/2");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }

        SUBCASE("multiple matches, shift left") {
            SUBCASE("1") {
                const uint8_t original_key[1] = {0b00000000};
                s.DeleteRawFromInfixStore(store, 0b0011111111100111,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("binary_trie/delete/multiple_matches/shift_left/1");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            SUBCASE("2") {
                const uint8_t original_key[1] = {0b11110000};
                s.DeleteRawFromInfixStore(store, 0b0011111111100111,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("binary_trie/delete/multiple_matches/shift_left/2");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            SUBCASE("3") {
                const uint8_t original_key[3] = {0b01000100, 0b00110101, 0b01001100};
                s.DeleteRawFromInfixStore(store, 0b0100000000101101,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("binary_trie/delete/multiple_matches/shift_left/3");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }

        SUBCASE("end of run, shift left") {
            const uint8_t original_key[3] = {0b01000100, 0b00110101, 0b01001100};
            s.DeleteRawFromInfixStore(store, 0b0100000000111111, 
                    infix_store_target_size,
                    nullptr, 
                    {original_key, 8 * sizeof(original_key)},
                    0);
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/delete/end_of_run/shift_left");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("destroy run, shift left") {
            for (int32_t i = 0; i < infix_vec[0].num_suffixes_; i++) {
                s.DeleteRawFromInfixStore(store, infix_vec[0].infix_,
                        infix_store_target_size,
                        nullptr, 
                        keys[i], 0);
            }
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/delete/destroy_run/shift_left");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("single match, shift right") {
            {
                const uint8_t original_key[2] = {0b01001010, 0b01000000};
                s.DeleteRawFromInfixStore(store, 0b0111111110011111,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("binary_trie/delete/single_match/shift_right/1");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            {
                uint8_t original_key[2] = {0b01001000, 0b00000000};
                s.DeleteRawFromInfixStore(store, 0b0111111110011111,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 *sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] = 
                    ReadStoreContentsFromFile("binary_trie/delete/single_match/shift_right/2");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }

        SUBCASE("destroy run, shift right") {
            int32_t key_offset = 0;
            for (int32_t i = 0; i < infix_vec.size() - 1; i++)
                key_offset += infix_vec[i].num_suffixes_ + infix_vec[i].GetNumPrefixKeys();
            for (int32_t i = 0; i < infix_vec.back().num_suffixes_ + infix_vec.back().GetNumPrefixKeys(); i++) {
                s.DeleteRawFromInfixStore(store, infix_vec.back().infix_, infix_store_target_size,
                        nullptr,
                        keys[key_offset + i], 0);
            }
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/delete/destroy_run/shift_right");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("end of run, shift right") {
            {
                const uint8_t original_key[3] = {0b01010110, 0b00000000, 0b00000000};
                s.DeleteRawFromInfixStore(store, 0b0111111110011111,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("binary_trie/delete/end_of_run/shift_right/1");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            {
                const uint8_t original_key[2] = {0b01010101, 0b00000000};
                s.DeleteRawFromInfixStore(store, 0b0111111110011111,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("binary_trie/delete/end_of_run/shift_right/2");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            {
                const uint8_t original_key[1] = {0b01010100};
                s.DeleteRawFromInfixStore(store, 0b0111111110011111,
                        infix_store_target_size,
                        nullptr, 
                        {original_key, 8 * sizeof(original_key)},
                        0);
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("binary_trie/delete/end_of_run/shift_right/3");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }

        SUBCASE("delete all") {
            int32_t key_ind = 0;
            std::vector<uint64_t> partial_cleanup;
            for (const auto& infix : infix_vec) {
                if ((infix.infix_ & 1) == 0) {  // Only remove full-length infixes, cleanup others later
                    partial_cleanup.push_back(infix.infix_);
                    continue;
                }
                if (infix.num_trie_bits_ > 0) {
                    for (int32_t i = 0; i < infix.num_suffixes_ + infix.GetNumPrefixKeys(); i++) {
                        s.DeleteRawFromInfixStore(store, infix.infix_,
                                infix_store_target_size,
                                nullptr,
                                keys[key_ind], key_start_bit);
                        key_ind++;
                    }
                }
                else 
                    s.DeleteRawFromInfixStore(store, infix.infix_);
            }
            {   // Check that non-partials were deleted correctly
                const auto [occupieds_pos, checks] =
                    ReadStoreContentsFromFile("binary_trie/delete/delete_all/non_partials");
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            for (auto& partial_infix : partial_cleanup)
                s.DeleteRawFromInfixStore(store, partial_infix | 1UL);
            const std::vector<uint32_t> occupieds_pos;
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks;
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }


    static void BinaryTrieGetLongestMatchingInfixSize() {
        const uint32_t N_bulk = 6;
        const uint32_t N_bulk_keys = 61;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 10;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size = BinaryTrieDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint64_t bulk_infixes[N_bulk] = {0b0000000000000011,
            0b0000000001000011, 0b0011111111100111,
            0b0100000000101101, 0b0111111110011111,
            0b0111111111011111};
        uint8_t keys_contents[N_bulk_keys][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N_bulk_keys];
        for (int32_t i = 0; i < N_bulk_keys - 1; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
        }
        keys_contents[N_bulk_keys - 1][0] = 0b01000100;
        keys_contents[N_bulk_keys - 1][1] = 0b00110101;
        keys_contents[N_bulk_keys - 1][2] = 0b01001100;
        keys[N_bulk_keys - 1] = {keys_contents[N_bulk_keys - 1], 24};
        std::sort(keys, keys + N_bulk_keys);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        int32_t key_ind = 0;
        for (int32_t i = 0; i < N_bulk; i++) {
            uint32_t num_keys_in_infix = std::min<uint32_t>(rng() % max_num_keys_in_infix + 1,
                                                            N_bulk_keys - key_ind);
            num_keys_in_infix += (i == 3);      // Manually add the prefix key
            infix_vec.emplace_back(bulk_infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + key_ind, num_keys_in_infix, key_start_bit, infix_size);
            key_ind += num_keys_in_infix;
        }
        infix_vec.insert(infix_vec.begin() + 2,  {0b0011111111100110});
        infix_vec.insert(infix_vec.begin() + 4,  {0b0100000000100101});
        infix_vec.insert(infix_vec.begin() + 6,  {0b0100000000111111});

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        SUBCASE("single infix, exact trie match") {
            const uint8_t original_key[2] = {0b00101011, 0b10100110};
            REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b0000000001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0), 
                    infix_size + 8 - 1);
        }

        SUBCASE("multiple infixes, exact trie match") {
            const uint8_t original_key[4] = {0b01000100, 0b00110101, 0b01001100, 0b10100111};
            REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b0100000000101101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0), 
                    infix_size + 25 - 1);
        }

        SUBCASE("multiple infixes, partial trie match") {
            const uint8_t original_key[4] = {0b01000100, 0b00110101, 0b01001100, 0b00111110};
            REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b0100000000101101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0), 
                    infix_size + 24 - 1);
        }

        SUBCASE("multiple infixes, exact infix match") {
            const uint8_t original_key[2] = {0b01100100, 0b00110101};
            REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b0100000000111111,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0), 
                    infix_size - 1);
        }

        SUBCASE("multiple infixes, partial infix match") {
            const uint8_t original_key[2] = {0b01100100, 0b00110101};
            REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b0011111111100101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0), 
                    infix_size - 1 - 1);
        }

        SUBCASE("no match") {
            const uint8_t original_key[4] = {0b01000100, 0b00110101, 0b01001100, 0b10100111};
            REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b0000000001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0), 
                    -1);
        }
    }


    static void BinaryTrieResize() {
        const uint32_t N = 662;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 16;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size = BinaryTrieDiva::infix_store_target_size;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint8_t keys_contents[N][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N];
        uint64_t infixes[N];
        for (uint32_t i = 0; i < N; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
            infixes[i] = (rng() & BITMASK(highbit_pos(infix_store_target_size) + infix_size)) | 1;
        }
        std::sort(infixes, infixes + N);
        std::sort(keys, keys + N);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        for (uint32_t i = 0; i < N; i++) {
            uint32_t num_keys_in_infix = 1;
            while (i + num_keys_in_infix < N && infixes[i + num_keys_in_infix] == infixes[i])
                num_keys_in_infix++;
            num_keys_in_infix = std::max(num_keys_in_infix,
                    std::min<uint32_t>(rng() % max_num_keys_in_infix + 1, N - i));
            infix_vec.emplace_back(infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + i, num_keys_in_infix,
                                                  key_start_bit, infix_size);
            i += num_keys_in_infix - 1;
        }

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);
        
        SUBCASE("expand") {
            {
                const uint8_t original_key[2] = {0b00010011, 0b10000000};
                s.InsertRawIntoInfixStore(store, 0b0000010110100001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
            }
            {
                const uint8_t original_key[2] = {0b00010011, 0b11111111};
                s.InsertRawIntoInfixStore(store, 0b0000010110100001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
            }

            {
                uint8_t original_key[2] = {0b00010011, 0b11111111};
                s.InsertRawIntoInfixStore(store, 0b0000100000001001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00010111;
                s.InsertRawIntoInfixStore(store, 0b0000100000001001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00011111;
                s.InsertRawIntoInfixStore(store, 0b0000100000001001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00111111;
                s.InsertRawIntoInfixStore(store, 0b0000100000001001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b01111111;
                s.InsertRawIntoInfixStore(store, 0b0000100000001001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b11111111;
                s.InsertRawIntoInfixStore(store, 0b0000100000001001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b10000111;
                s.InsertRawIntoInfixStore(store, 0b0000100000001001,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
            }

            {
                uint8_t original_key[3] = {0b00010001, 0b10111111, 0b00000000};
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[1] = 0b00111111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00010000;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);

                original_key[0] = 0b00010011;
                original_key[1] = 0b11111111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);

                original_key[0] = 0b00010110;
                original_key[1] = 0b00111111;
                original_key[2] = 0b00000000;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[1] = 0b01111111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[1] = 0b11111111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00010111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00011111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00111111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b01111111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b11111111;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);

                original_key[1] = 0b01010101;
                s.InsertRawIntoInfixStore(store, 0b0000100011110101,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
            }

            {
                uint8_t original_key[2] = {0b00010111, 0b00111111};
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[1] = 0b01111111;
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00010110;
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00010100;
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00010000;
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00110000;
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b01110000;
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b11110000;
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)}, 
                        0);
                original_key[0] = 0b11111111;
                s.InsertRawIntoInfixStore(store, 0b0000101001000011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
            }

            {
                uint8_t original_key[2] = {0b00100001, 0b00010000};
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[1] = 0b00000000;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[1] = 0b01000000;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[1] = 0b11000000;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00100011;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00100111;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00101111;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b00111111;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b01111111;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[0] = 0b11111111;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
                original_key[1] = 0b01010101;
                s.InsertRawIntoInfixStore(store, 0b0000110101111011,
                        infix_store_target_size,
                        nullptr,
                        {original_key, 8 * sizeof(original_key)},
                        0);
            }

            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/resize/expand");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("contract") {
            const uint32_t num_to_delete = 30;
            int32_t infix_ind = 0, last_keys_ind = 0, keys_ind = 0;
            for (int32_t i = 0; i < num_to_delete; i++) {
                s.DeleteRawFromInfixStore(store, infix_vec[infix_ind].infix_,
                        infix_store_target_size,
                        nullptr,
                        keys[keys_ind], key_start_bit);
                keys_ind++;
                if (keys_ind - last_keys_ind >=
                        infix_vec[infix_ind].num_suffixes_ + infix_vec[infix_ind].GetNumPrefixKeys()) {
                    last_keys_ind = keys_ind;
                    infix_ind++;
                }
            }
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/resize/contract");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }


    static void BinaryTrieAdapt() {
        const uint32_t N_bulk = 6;
        const uint32_t N_bulk_keys = 61;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 10;
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        uint64_t bulk_infixes[N_bulk] = {0b0000000000000011,
            0b0000000001000011, 0b0011111111100111,
            0b0100000000101101, 0b0111111110011111,
            0b0111111111011111};
        uint8_t keys_contents[N_bulk_keys][max_key_len + 1] = {};
        BinaryTrieDiva::InfiniteByteString keys[N_bulk_keys];
        for (int32_t i = 0; i < N_bulk_keys - 1; i++) {
            const uint32_t key_len = min_key_len + rng() % (max_key_len - min_key_len + 1);
            keys[i] = {keys_contents[i], 8 * key_len};
            for (uint32_t j = (key_start_bit + 7) / 8; j < key_len; j++)
                keys_contents[i][j] = rng();
        }
        keys_contents[N_bulk_keys - 1][0] = 0b01000100;
        keys_contents[N_bulk_keys - 1][1] = 0b00110101;
        keys_contents[N_bulk_keys - 1][2] = 0b01001100;
        keys[N_bulk_keys - 1] = {keys_contents[N_bulk_keys - 1], 24};
        std::sort(keys, keys + N_bulk_keys);

        std::vector<BinaryTrieDiva::Infix> infix_vec;
        int32_t key_ind = 0;
        for (int32_t i = 0; i < N_bulk; i++) {
            uint32_t num_keys_in_infix = std::min<uint32_t>(rng() % max_num_keys_in_infix + 1,
                                                            N_bulk_keys - key_ind);
            num_keys_in_infix += (i == 3);      // Manually add the prefix key
            infix_vec.emplace_back(bulk_infixes[i]);
            infix_vec.back().BuildTrieAndSuffixes(keys + key_ind, num_keys_in_infix,
                                                  key_start_bit, infix_size);
            key_ind += num_keys_in_infix;
        }
        infix_vec.insert(infix_vec.begin() + 2,  {0b0011111111100110});
        infix_vec.insert(infix_vec.begin() + 4,  {0b0100000000100101});
        infix_vec.insert(infix_vec.begin() + 6,  {0b0100000000111111});

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        SUBCASE("add suffix") {
            const uint8_t original_key[2] = {0b00101011, 0b00010000};
            s.AdaptRawInInfixStore(store, 0b0000000001000011,
                    {original_key, 8 * sizeof(original_key)},
                    0, 
                    infix_size - 1 + 11);
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/adapt/add_suffix");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("add multiple suffixes") {
            const uint8_t original_key[2] = {0b00101011, 0b00010000};
            s.AdaptRawInInfixStore(store, 0b0000000001000011,
                    {original_key, 8 * sizeof(original_key)},
                    0, 
                    infix_size - 1 + 16);
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/adapt/add_multiple_suffixes");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("add suffix in prefix trie") {
            const uint8_t original_key[4] = {0b01000100, 0b00110101, 0b01001100, 0b10101010};
            s.AdaptRawInInfixStore(store, 0b0100000000101101,
                    {original_key, 8 * sizeof(original_key)},
                    0, 
                    infix_size - 1 + 32);
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/adapt/add_suffix_in_prefix_trie");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("adapt prefix key") {
            const uint8_t original_key[4] = {0b01000100, 0b00110101, 0b01001100, 0b01010101};
            s.AdaptRawInInfixStore(store, 0b0100000000101101,
                    {original_key, 8 * sizeof(original_key)},
                    0, 
                    infix_size - 1 + 32);
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/adapt/prefix_key");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("adapt partial infix") {
            const uint8_t original_key[1] = {0b00000000};
            s.AdaptRawInInfixStore(store, 0b0011111111100101, 
                    {original_key, 8 * sizeof(original_key)},
                    0,
                    infix_size - 1);
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/adapt/partial_infix");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
        
        SUBCASE("create trie") {
            const uint8_t original_key[2] = {0b11001100, 0b00110011};
            s.AdaptRawInInfixStore(store, 0b0100000000111111,
                    {original_key, 8 * sizeof(original_key)},
                    0, 
                    infix_size - 1 + 16);
            const auto [occupieds_pos, checks] =
                ReadStoreContentsFromFile("binary_trie/adapt/create_trie");
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }


private:
    static void WriteStoreContentsToFile(std::string path,
                                         const std::vector<uint32_t> &occupieds_pos, 
                                         const std::vector<std::tuple<uint32_t, bool, uint64_t>> &checks,
                                         const uint64_t * const *payloads = nullptr,
                                         uint32_t payload_size = 0) {
        const std::string path_prefix = "./tests/data/infix_store/";
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
        const std::string path_prefix = "./tests/data/infix_store/";
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
        const std::string path_prefix = "./tests/data/infix_store/";
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

TEST_SUITE("infix_store") {
    TEST_CASE("allocation") {
        InfixStoreTests::Allocation(); 
    }

    TEST_CASE("shifting slots") {
        InfixStoreTests::ShiftingSlots(); 
    }

    TEST_CASE("shifting runends") {
        InfixStoreTests::ShiftingRunends(); 
    }

    TEST_CASE("insert") {
        InfixStoreTests::InsertRaw(); 
    }

    TEST_CASE("delete") {
        SUBCASE("delete raw") {
            InfixStoreTests::DeleteRaw(); 
        }
        SUBCASE("get longest matching infix size") {
            InfixStoreTests::GetLongestMatchingInfixSize();
        }
    }

    TEST_CASE("get infix list") {
        InfixStoreTests::GetInfixList();
    }

    TEST_CASE("load infix list") {
        InfixStoreTests::LoadInfixList();
    }

    TEST_CASE("point query") {
        InfixStoreTests::PointQuery();
    }

    TEST_CASE("range query") {
        InfixStoreTests::RangeQuery();
    }

    TEST_CASE("resize") {
        InfixStoreTests::Resize();
    }

    TEST_CASE("payloads") {
        SUBCASE("sanity") {
            InfixStoreTests::PayloadsSanity();
        }
        SUBCASE("insert raw") {
            InfixStoreTests::PayloadsInsertRaw();
        }
        SUBCASE("get infix list") {
            InfixStoreTests::PayloadsGetInfixList();
        }
        SUBCASE("load infix list") {
            InfixStoreTests::PayloadsLoadInfixList();
        }
        SUBCASE("delete") {
            SUBCASE("delete raw") {
                InfixStoreTests::PayloadsDeleteRaw();
            }
            SUBCASE("get longest matching infix size") {
                InfixStoreTests::PayloadsGetLongestMatchingInfixSize();
            }
        }
        SUBCASE("resize") {
            InfixStoreTests::PayloadsResize();
        }
    }

    TEST_CASE("binary trie") {
        SUBCASE("load infix vector") {
            InfixStoreTests::BinaryTrieLoadInfixVector(); 
        }
        SUBCASE("get infix vector") {
            InfixStoreTests::BinaryTrieGetInfixVector();
        }
        SUBCASE("insert raw") {
            InfixStoreTests::BinaryTrieInsertRaw();
        }
        SUBCASE("point query") {
            InfixStoreTests::BinaryTriePointQuery();
        }
        SUBCASE("range query") {
            InfixStoreTests::BinaryTrieRangeQuery();
        }
        SUBCASE("delete") {
            SUBCASE("delete raw") {
                InfixStoreTests::BinaryTrieDeleteRaw();
            }
            SUBCASE("get longest matching infix size") {
                InfixStoreTests::BinaryTrieGetLongestMatchingInfixSize();
            }
        }
        SUBCASE("resize") {
            InfixStoreTests::BinaryTrieResize();
        }
        SUBCASE("adapt") {
            InfixStoreTests::BinaryTrieAdapt();
        }
    }
}

} // namespace diva

//PrintStore(s, store);
//std::vector<uint32_t> occupieds_pos = {};
//std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
//WriteStoreContentsToFile("delete/lone_run_with_single_slot", occupieds_pos, checks);
//std::cerr << "WEEEEEEEEELP" << std::endl;
