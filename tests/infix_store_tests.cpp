/**
 * @file infix store tests
 * @author ---
 */

#include <cstddef>
#include <filesystem>
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
        std::sort(keys.begin(), keys.end(), [=](uint64_t a, uint64_t b) {
                    return (a ^ (a & -a)) < (b ^ (b & -b));
                });
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

        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110011), infix_size);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111110001), infix_size - 1);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b000100111100001), infix_size - 4);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b111111111111111), 0);
    }

    static void BinaryTrieLoadInfixList() {
        const uint32_t N = 600;
        const uint32_t key_start_bit = 0;
        const uint32_t min_key_len = 6;
        const uint32_t max_key_len = 17;
        const uint32_t max_num_keys_in_infix = 16;
        const uint32_t infix_size = 5;
        const uint32_t infix_store_target_size =
            BinaryTrieDiva::infix_store_target_size;
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
            infix_vec.back().BuildTrie(keys + i, num_keys_in_infix, key_start_bit, infix_size);
            i += num_keys_in_infix - 1;
        }

        BinaryTrieDiva s(infix_size, seed, load_factor);
        const uint32_t total_slots = s.scaled_sizes_[s.size_scalar_shrink_grow_sep];
        BinaryTrieDiva::InfixStore store(total_slots, s.infix_size_,
                s.size_scalar_shrink_grow_sep);
        s.LoadVectorToInfixStore(store, infix_vec);

        const auto [occupieds_pos, checks] =
            ReadStoreContentsFromFile("binary_trie/load_infix_list");
        AssertStoreContents(s, store, occupieds_pos, checks);
    }

private:
    static void WriteStoreContentsToFile(std::string path,
                                         const std::vector<uint32_t> &occupieds_pos, 
                                         const std::vector<std::tuple<uint32_t, bool, uint64_t>> &checks) {
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

    template <DivaType diva_type, PayloadType payload_type>
    static void AssertStoreContents(const Diva<diva_type, payload_type> &s,
                                    const typename Diva<diva_type, payload_type>::InfixStore &store,
                                    const std::vector<uint32_t> &occupieds_pos,
                                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> &checks,
                                    const uint64_t **check_payloads = nullptr) {
        REQUIRE_NE(store.ptr, nullptr);
        if constexpr (diva_type != DivaType::BinaryTrie)
            REQUIRE_EQ(store.GetFullSlotCount(), checks.size());
        if constexpr (payload_type == PayloadType::FixedLength)
            assert(check_payloads != nullptr);
        const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + Diva<>::num_metadata_offset_words;
        const uint64_t *runends = store.ptr + Diva<>::num_metadata_offset_words +
            Diva<>::infix_store_target_size / 64;
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
            check_popcnts[1] += __builtin_popcountll(runends[i]);
        }
        REQUIRE_EQ(popcnts[0], check_popcnts[0]);
        REQUIRE_EQ(popcnts[1], check_popcnts[1]);
    }

    template <DivaType diva_type, PayloadType payload_type>
    static void PrintStore(const Diva<diva_type, payload_type> &s,
                           const typename Diva<diva_type, payload_type>::InfixStore &store) {
        const uint32_t size_grade = store.GetSizeGrade();
        const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + Diva<>::num_metadata_offset_words;
        const uint64_t *runends = store.ptr + Diva<>::num_metadata_offset_words +
            Diva<>::infix_store_target_size / 64;

        std::cerr << "is_partial=" << store.IsPartialKey() << " invalid_bits=" << store.GetInvalidBits();
        std::cerr << " size_grade=" << size_grade << " elem_count=" << store.GetFullSlotCount() << std::endl;
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
    // InfixStoreTests::GetInfixList();
  }

  TEST_CASE("load infix list") {
    // InfixStoreTests::LoadInfixList();
  }

  TEST_CASE("point query") {
    // InfixStoreTests::PointQuery();
  }

  TEST_CASE("range query") {
    // InfixStoreTests::RangeQuery();
  }

  TEST_CASE("resize") {
    // InfixStoreTests::Resize();
  }

  TEST_CASE("payloads") {
    SUBCASE("sanity") {
      // InfixStoreTests::PayloadsSanity();
    }
    SUBCASE("insert raw") {
      // InfixStoreTests::PayloadsInsertRaw();
    }
    SUBCASE("get infix list") {
      // InfixStoreTests::PayloadsGetInfixList();
    }
    SUBCASE("load infix list") {
      // InfixStoreTests::PayloadsLoadInfixList();
    }
    SUBCASE("delete raw") {
      // InfixStoreTests::PayloadsDeleteRaw();
      // InfixStoreTests::PayloadsGetLongestMatchingInfixSize();
    }
    SUBCASE("resize") {
      // InfixStoreTests::PayloadsResize();
    }
  }

  TEST_CASE("binary trie") {
    SUBCASE("load infix list") {
        InfixStoreTests::BinaryTrieLoadInfixList(); 
    }
    SUBCASE("get infix list") {
      // InfixStoreTests::BinaryTrieGetInfixList();
    }
    SUBCASE("resize") {
      // InfixStoreTests::BinaryTrieResize();
    }
    SUBCASE("insert raw") {
      // InfixStoreTests::BinaryTrieInsertRaw();
    }
    SUBCASE("point query") {
      // InfixStoreTests::BinaryTriePointQuery();
    }
    SUBCASE("range query") {
      // InfixStoreTests::BinaryTrieRangeQuery();
    }
    SUBCASE("delete raw") {
      // InfixStoreTests::BinaryTrieDeleteRaw();
      // InfixStoreTests::BinaryTrieGetLongestMatchingInfixSize();
    }
    SUBCASE("adapt") {
      // InfixStoreTests::BinaryTrieAdapt();
    }
  }
}

} // namespace diva
