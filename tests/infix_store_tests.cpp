/**
 * @file infix store tests
 * @author ---
 */

#include <random>
#include <utility>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "steroids.hpp"
#include "util.hpp"

const char ansi_green[] = "\033[0;32m";
const char ansi_white[] = "\033[0;97m";

class InfixStoreTests {
public:
    static void Allocation() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const uint32_t total_words = (Steroids<false>::infix_store_target_size + (s.infix_size_ + 1) * s.scaled_sizes_[0] + 63) / 64;
        for (int32_t i = 0; i < total_words; i++)
            REQUIRE_EQ(store.ptr[i], 0);
    }


    static void ShiftingSlots() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const uint32_t total_slots = s.scaled_sizes_[0];
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
            REQUIRE_EQ(s.GetSlot(store, total_slots - 1), ((total_slots - 1) & BITMASK(s.infix_size_)));
            REQUIRE_EQ(s.GetSlot(store, total_slots - 2), ((total_slots - 2) & BITMASK(s.infix_size_)));
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
            REQUIRE_EQ(s.GetSlot(store, total_slots - 1), ((total_slots - 1) & BITMASK(s.infix_size_)));
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
            REQUIRE_EQ(s.GetSlot(store, total_slots - 1), ((total_slots - 1) & BITMASK(s.infix_size_)));
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
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        uint64_t *runends = store.ptr + Steroids<false>::infix_store_target_size / 64;
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
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);
        uint64_t *runends = store.ptr + Steroids<false>::infix_store_target_size / 64;
        uint64_t inserts[100];

        inserts[0] = 0b010000000001100;
        inserts[1] = 0b010000000001011;
        inserts[2] = 0b010000000001101;
        inserts[3] = 0b010000000001110;
        s.InsertRawIntoInfixStore(store, inserts[0]);
        s.InsertRawIntoInfixStore(store, inserts[2]);
        s.InsertRawIntoInfixStore(store, inserts[1]);
        s.InsertRawIntoInfixStore(store, inserts[3]);
        SUBCASE("insertion and shifting of a single run") {
            for (int32_t i = 0; i < 4; i++)
                REQUIRE_EQ(s.GetSlot(store, 269 + i), (inserts[i] & BITMASK(infix_size)));
            for (int32_t i = 269; i < 272; i++)
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            REQUIRE_EQ(get_bitmap_bit(runends, 272), 1);
        }

        inserts[4] = 0b001111111100001;
        inserts[5] = 0b001111111100010;
        inserts[6] = 0b001111111100100;
        inserts[7] = 0b001111111100011;
        s.InsertRawIntoInfixStore(store, inserts[4]);
        s.InsertRawIntoInfixStore(store, inserts[5]);
        s.InsertRawIntoInfixStore(store, inserts[6]);
        s.InsertRawIntoInfixStore(store, inserts[7]);
        SUBCASE("insertion and shifting of a new run that shifts an old run") {
            for (int32_t i = 268; i < 271; i++) { 
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 268 + 4] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 271), (inserts[7] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 271), 1);
            for (int32_t i = 272; i < 275; i++) { 
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 272] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 275), (inserts[3] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 275), 1);
        }

        inserts[0] = 0b000000000000001;
        inserts[1] = 0b000000000000001;
        inserts[2] = 0b000000000000010;
        inserts[3] = 0b000000000000011;
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

        inserts[5] = 0b000000001111111;
        s.InsertRawIntoInfixStore(store, inserts[5]);
        SUBCASE("inserting new runs in between two touching runs: 1 slots added") {
            REQUIRE_EQ(s.GetSlot(store, 4), (inserts[5] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 4), 1);
            REQUIRE_EQ(s.GetSlot(store, 5), 0b00000);
            REQUIRE_EQ(get_bitmap_bit(runends, 5), 0);
        }

        inserts[4] = 0b000000000110101;
        s.InsertRawIntoInfixStore(store, inserts[4]);
        SUBCASE("inserting new runs in between two touching runs: 2 slots added") {
            REQUIRE_EQ(s.GetSlot(store, 4), (inserts[4] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 4), 1);
            REQUIRE_EQ(s.GetSlot(store, 5), (inserts[5] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 5), 1);
        }

        inserts[0] = 0b011111110100001;
        inserts[1] = 0b011111110100010;
        inserts[2] = 0b011111110100011;
        inserts[3] = 0b011111110100111;
        inserts[4] = 0b011111111100001;
        inserts[5] = 0b011111111100001;
        inserts[6] = 0b011111111100010;
        s.InsertRawIntoInfixStore(store, inserts[0]);
        s.InsertRawIntoInfixStore(store, inserts[4]);
        s.InsertRawIntoInfixStore(store, inserts[5]);
        s.InsertRawIntoInfixStore(store, inserts[1]);
        s.InsertRawIntoInfixStore(store, inserts[6]);
        s.InsertRawIntoInfixStore(store, inserts[2]);
        s.InsertRawIntoInfixStore(store, inserts[3]);
        SUBCASE("insertion and shifting at the very end of the array") {
            for (int32_t i = 531; i < 534; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 531] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 534), (inserts[3] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 534), 1);
            for (int32_t i = 535; i < 537; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 535 + 4] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 537), (inserts[6] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 537), 1);
        }

        inserts[7] = 0b011111111011101;
        inserts[8] = 0b011111111011110;
        inserts[9] = 0b011111111011111;
        s.InsertRawIntoInfixStore(store, inserts[9]);
        s.InsertRawIntoInfixStore(store, inserts[7]);
        s.InsertRawIntoInfixStore(store, inserts[8]);
        SUBCASE("insertion and shifting in between touching runs at the very end of the array") {
            for (int32_t i = 528; i < 531; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 528] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 531), (inserts[3] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 537), 1);
            for (int32_t i = 532; i < 534; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 532 + 7] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 534), (inserts[9] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 534), 1);
            for (int32_t i = 535; i < 537; i++) {
                REQUIRE_EQ(s.GetSlot(store, i), (inserts[i - 535 + 4] & BITMASK(infix_size)));
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
            REQUIRE_EQ(s.GetSlot(store, 537), (inserts[6] & BITMASK(infix_size)));
            REQUIRE_EQ(get_bitmap_bit(runends, 537), 1);
        }
    }

    static void DeleteRaw() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const std::vector<uint64_t> keys {0b00000010011000, 0b00000010010100,
            0b00000010010110, 0b00000010010101, 0b00000010011111,
            0b00000010110101, 0b00000010110111, 0b00000010111001,
            0b00000011111011, 0b00000100011011, 0b00000100011111,
            0b00000111100001, 0b00000111100011, 0b00000111100111,
            0b11111010000001, 0b11111101100101, 0b11111101100111,
            0b11111110011111, 0b11111110110101, 0b11111111011000,
            0b11111111010100, 0b11111111010110, 0b11111111010101,
            0b11111111011111, 0b11111111100001, 0b11111111100011};
        s.LoadListToInfixStore(store, keys.data(), keys.size());

        SUBCASE("single match, shift left") {
            s.DeleteRawFromInfixStore(store, 0b00000010011111);
            const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15, 500,
                507, 508, 509, 510, 511};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110}, {7,1,0b10101},
                     {8,0,0b10101}, {9,0,0b10111}, {10,1,0b11001},
                     {11,1,0b11011}, {12,0,0b11011}, {13,1,0b11111},
                     {15,0,0b00001}, {16,0,0b00011}, {17,1,0b00111},
                     {526,1,0b00001}, {527,0,0b00101}, {528,1,0b00111},
                     {529,1,0b11111}, {530,1,0b10101}, {531,0,0b11000},
                     {532,0,0b10100}, {533,0,0b10110}, {534,0,0b10101},
                     {535,1,0b11111}, {536,0,0b00001}, {537,1,0b00011}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("multiple matches, shift left") {
            s.DeleteRawFromInfixStore(store, 0b00000010010101);
            {
                const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15,
                    500, 507, 508, 509, 510, 511};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                     {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110},
                         {7,1,0b11111}, {8,0,0b10101}, {9,0,0b10111},
                         {10,1,0b11001}, {11,1,0b11011}, {12,0,0b11011},
                         {13,1,0b11111}, {15,0,0b00001}, {16,0,0b00011},
                         {17,1,0b00111}, {526,1,0b00001}, {527,0,0b00101},
                         {528,1,0b00111}, {529,1,0b11111}, {530,1,0b10101},
                         {531,0,0b11000}, {532,0,0b10100}, {533,0,0b10110},
                         {534,0,0b10101}, {535,1,0b11111}, {536,0,0b00001},
                         {537,1,0b00011}};
                AssertStoreContents(s, store, occupieds_pos, checks);
            }

            s.DeleteRawFromInfixStore(store, 0b00000010010101);
            {
                const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15,
                    500, 507, 508, 509, 510, 511};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                     {{4,0,0b11000}, {5,0,0b10100}, {6,1,0b11111},
                         {7,0,0b10101}, {8,0,0b10111}, {9,1,0b11001},
                         {10,1,0b11011}, {11,0,0b11011}, {12,1,0b11111},
                         {15,0,0b00001}, {16,0,0b00011}, {17,1,0b00111},
                         {526,1,0b00001}, {527,0,0b00101}, {528,1,0b00111},
                         {529,1,0b11111}, {530,1,0b10101}, {531,0,0b11000},
                         {532,0,0b10100}, {533,0,0b10110}, {534,0,0b10101},
                         {535,1,0b11111}, {536,0,0b00001}, {537,1,0b00011}};
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }

        SUBCASE("destroy run, shift left") {
            s.DeleteRawFromInfixStore(store, 0b00000011111011);
            const std::vector<uint32_t> occupieds_pos = {4, 5, 8, 15, 500, 507,
                508, 509, 510, 511};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110}, {7,0,0b10101},
                     {8,1,0b11111}, {9,0,0b10101}, {10,0,0b10111},
                     {11,1,0b11001}, {12,0,0b11011}, {13,1,0b11111},
                     {15,0,0b00001}, {16,0,0b00011}, {17,1,0b00111},
                     {526,1,0b00001}, {527,0,0b00101}, {528,1,0b00111},
                     {529,1,0b11111}, {530,1,0b10101}, {531,0,0b11000},
                     {532,0,0b10100}, {533,0,0b10110}, {534,0,0b10101},
                     {535,1,0b11111}, {536,0,0b00001}, {537,1,0b00011}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("end of run, shift left") {
            s.DeleteRawFromInfixStore(store, 0b00000010011111);
            const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15, 500,
                507, 508, 509, 510, 511};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110}, {7,1,0b10101},
                     {8,0,0b10101}, {9,0,0b10111}, {10,1,0b11001},
                     {11,1,0b11011}, {12,0,0b11011}, {13,1,0b11111},
                     {15,0,0b00001}, {16,0,0b00011}, {17,1,0b00111},
                     {526,1,0b00001}, {527,0,0b00101}, {528,1,0b00111},
                     {529,1,0b11111}, {530,1,0b10101}, {531,0,0b11000},
                     {532,0,0b10100}, {533,0,0b10110}, {534,0,0b10101},
                     {535,1,0b11111}, {536,0,0b00001}, {537,1,0b00011}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("single match, shift right") {
            s.DeleteRawFromInfixStore(store, 0b11111111011111);
            const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15, 500,
                507, 508, 509, 510, 511};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110}, {7,0,0b10101},
                     {8,1,0b11111}, {9,0,0b10101}, {10,0,0b10111},
                     {11,1,0b11001}, {12,1,0b11011}, {13,0,0b11011},
                     {14,1,0b11111}, {15,0,0b00001}, {16,0,0b00011},
                     {17,1,0b00111}, {526,1,0b00001}, {528,0,0b00101},
                     {529,1,0b00111}, {530,1,0b11111}, {531,1,0b10101},
                     {532,0,0b11000}, {533,0,0b10100}, {534,0,0b10110},
                     {535,1,0b10101}, {536,0,0b00001}, {537,1,0b00011}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("multiple matches, shift right") {
            s.DeleteRawFromInfixStore(store, 0b11111111010101);
            {
                const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15,
                    500, 507, 508, 509, 510, 511};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                     {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110},
                         {7,0,0b10101}, {8,1,0b11111}, {9,0,0b10101},
                         {10,0,0b10111}, {11,1,0b11001}, {12,1,0b11011},
                         {13,0,0b11011}, {14,1,0b11111}, {15,0,0b00001},
                         {16,0,0b00011}, {17,1,0b00111}, {526,1,0b00001},
                         {528,0,0b00101}, {529,1,0b00111}, {530,1,0b11111},
                         {531,1,0b10101}, {532,0,0b11000}, {533,0,0b10100},
                         {534,0,0b10110}, {535,1,0b11111}, {536,0,0b00001},
                         {537,1,0b00011}};
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            s.DeleteRawFromInfixStore(store, 0b11111111010101);
            {
                const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15,
                    500, 507, 508, 509, 510, 511};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                     {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110},
                         {7,0,0b10101}, {8,1,0b11111}, {9,0,0b10101},
                         {10,0,0b10111}, {11,1,0b11001}, {12,1,0b11011},
                         {13,0,0b11011}, {14,1,0b11111}, {15,0,0b00001},
                         {16,0,0b00011}, {17,1,0b00111}, {526,1,0b00001},
                         {529,0,0b00101}, {530,1,0b00111}, {531,1,0b11111},
                         {532,1,0b10101}, {533,0,0b11000}, {534,0,0b10100},
                         {535,1,0b11111}, {536,0,0b00001}, {537,1,0b00011}};
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }

        SUBCASE("destroy run, shift right") {
            s.DeleteRawFromInfixStore(store, 0b11111110110101);
            const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15, 500,
                507, 508, 510, 511};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110}, {7,0,0b10101},
                     {8,1,0b11111}, {9,0,0b10101}, {10,0,0b10111},
                     {11,1,0b11001}, {12,1,0b11011}, {13,0,0b11011},
                     {14,1,0b11111}, {15,0,0b00001}, {16,0,0b00011},
                     {17,1,0b00111}, {526,1,0b00001}, {528,0,0b00101},
                     {529,1,0b00111}, {530,1,0b11111}, {531,0,0b11000},
                     {532,0,0b10100}, {533,0,0b10110}, {534,0,0b10101},
                     {535,1,0b11111}, {536,0,0b00001}, {537,1,0b00011}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        SUBCASE("end of run, shift right") {
            s.DeleteRawFromInfixStore(store, 0b11111111100011);
            const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15, 500,
                507, 508, 509, 510, 511};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110}, {7,0,0b10101},
                     {8,1,0b11111}, {9,0,0b10101}, {10,0,0b10111},
                     {11,1,0b11001}, {12,1,0b11011}, {13,0,0b11011},
                     {14,1,0b11111}, {15,0,0b00001}, {16,0,0b00011},
                     {17,1,0b00111}, {526,1,0b00001}, {528,0,0b00101},
                     {529,1,0b00111}, {530,1,0b11111}, {531,1,0b10101},
                     {532,0,0b11000}, {533,0,0b10100}, {534,0,0b10110},
                     {535,0,0b10101}, {536,1,0b11111}, {537,1,0b00001}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.InsertRawIntoInfixStore(store, 0b10000000010101);
        SUBCASE("lone run with single slot") {
            s.DeleteRawFromInfixStore(store, 0b10000000010101);
            const std::vector<uint32_t> occupieds_pos = {4, 5, 7, 8, 15, 500,
                507, 508, 509, 510, 511};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{4,0,0b11000}, {5,0,0b10100}, {6,0,0b10110}, {7,0,0b10101},
                     {8,1,0b11111}, {9,0,0b10101}, {10,0,0b10111},
                     {11,1,0b11001}, {12,1,0b11011}, {13,0,0b11011},
                     {14,1,0b11111}, {15,0,0b00001}, {16,0,0b00011},
                     {17,1,0b00111}, {526,1,0b00001}, {527,0,0b00101},
                     {528,1,0b00111}, {529,1,0b11111}, {530,1,0b10101},
                     {531,0,0b11000}, {532,0,0b10100}, {533,0,0b10110},
                     {534,0,0b10101}, {535,1,0b11111}, {536,0,0b00001},
                     {537,1,0b00011}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }


    static void GetLongestMatchingInfixSize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const std::vector<uint64_t> keys {0b00000010011000, 0b00000010010100,
            0b00000010010110, 0b00000010010101, 0b00000010011111,
            0b00000010110101, 0b00000010110111, 0b00000010111001,
            0b00000011111011, 0b00000100011011, 0b00000100011111,
            0b00000111100001, 0b00000111100011, 0b00000111100111, 0b00100111110000, 0b00100111110010, 0b00100111110011,
            0b11111010000001, 0b11111101100101, 0b11111101100111,
            0b11111110011111, 0b11111110110101, 0b11111111011000,
            0b11111111010100, 0b11111111010110, 0b11111111010101,
            0b11111111011111, 0b11111111100001, 0b11111111100011};
        s.LoadListToInfixStore(store, keys.data(), keys.size());

        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b00100111110011), infix_size);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b00100111110001), infix_size - 1);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b00100111100001), infix_size - 4);
        REQUIRE_EQ(s.GetLongestMatchingInfixSize(store, 0b11111111111111), 0);
    }

    static void GetInfixList() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const std::vector<uint64_t> keys {0b00000000000001, 0b00000000000101,
            0b00000000010101, 0b00000000100001, 0b00000000100011,
            0b00000000100101, 0b01000000100001, 0b01000000100011,
            0b01000000100101, 0b01000000100110, 0b01000000100110,
            0b01000000100110, 0b01000001100001, 0b01000001100011,
            0b01000001100101, 0b01111111000001, 0b01111111000010,
            0b01111111000010, 0b01111111100001, 0b01111111100010,
            0b01111111100010};
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
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        SUBCASE("fetch") {
            const std::vector<uint64_t> keys {0b000000000000001, 0b000000000000101,
                0b000000000010101, 0b000000000100001, 0b000000000100011,
                0b000000000100101, 0b001000000100001, 0b001000000100011,
                0b001000000100101, 0b001000000100110, 0b001000000100110,
                0b001000000100110, 0b001000001100001, 0b001000001100011,
                0b001000001100101, 0b001111111000001, 0b001111111000010,
                0b001111111000010, 0b001111111100001, 0b001111111100010,
                0b001111111100010, 0b011111111000001, 0b011111111000010,
                0b011111111000010, 0b011111111100001, 0b011111111100010,
                0b011111111100010};
            s.LoadListToInfixStore(store, keys.data(), keys.size());
            uint64_t res[keys.size() + 1];
            const uint32_t len = s.GetInfixList(store, res);
            REQUIRE_EQ(len, keys.size());
            for (int32_t i = 0; i < keys.size(); i++)
                REQUIRE_EQ(res[i], keys[i]);
        }

        SUBCASE("vs. insert one by one") {
            const uint32_t n_keys = 512;
            const uint32_t rng_seed = 1;
            std::mt19937_64 rng(rng_seed);
            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++)
                keys.push_back((rng() & BITMASK(Steroids<false>::base_implicit_size + infix_size)) | 1ULL);
            auto comp = [](uint64_t a, uint64_t b) {
                            const uint64_t a_lb = a & (-a), b_lb = b & (-b);
                            const uint64_t a_nolb = a - a_lb;
                            const uint64_t b_nolb = b - b_lb;
                            return a_nolb < b_nolb || (a_nolb == b_nolb && a_lb > b_lb);
                        };
            std::sort(keys.begin(), keys.end(), comp);
            s.LoadListToInfixStore(store, keys.data(), keys.size());

            const std::vector<uint32_t> occupieds_pos = {0, 1, 4, 7, 10, 11,
                13, 15, 16, 17, 20, 21, 24, 26, 29, 30, 31, 32, 33, 34, 35, 36,
                37, 39, 42, 43, 44, 45, 46, 49, 50, 51, 54, 55, 57, 58, 60, 63,
                64, 68, 69, 71, 74, 75, 76, 78, 79, 82, 83, 84, 85, 86, 87, 88,
                90, 91, 92, 93, 95, 98, 99, 101, 102, 103, 104, 105, 106, 109,
                111, 112, 115, 116, 117, 118, 120, 121, 123, 124, 127, 128,
                129, 130, 132, 134, 135, 136, 137, 138, 140, 141, 144, 145,
                146, 147, 148, 149, 152, 153, 154, 155, 156, 158, 159, 160,
                161, 162, 163, 165, 166, 168, 169, 170, 171, 172, 173, 174,
                176, 177, 178, 180, 181, 182, 183, 184, 187, 188, 189, 190,
                191, 194, 195, 196, 198, 201, 203, 208, 209, 211, 212, 213,
                214, 215, 216, 217, 218, 219, 221, 222, 223, 226, 228, 229,
                230, 231, 232, 233, 234, 237, 240, 241, 243, 246, 248, 249,
                250, 252, 253, 254, 257, 259, 261, 262, 263, 264, 266, 271,
                272, 275, 276, 277, 278, 282, 285, 287, 289, 290, 293, 294,
                295, 296, 297, 300, 301, 302, 304, 305, 307, 311, 312, 313,
                314, 315, 316, 317, 318, 319, 321, 322, 324, 326, 327, 333,
                335, 337, 338, 339, 341, 342, 344, 346, 348, 349, 350, 354,
                355, 356, 364, 365, 366, 367, 369, 371, 372, 373, 376, 378,
                379, 381, 382, 385, 388, 391, 392, 394, 396, 397, 398, 399,
                400, 401, 402, 403, 404, 405, 407, 408, 410, 411, 413, 414,
                416, 417, 419, 421, 423, 424, 426, 427, 430, 431, 432, 434,
                435, 436, 438, 440, 441, 442, 443, 444, 445, 447, 449, 451,
                452, 453, 454, 455, 456, 457, 459, 462, 463, 466, 467, 468,
                469, 470, 472, 475, 476, 477, 478, 479, 480, 481, 484, 485,
                487, 490, 491, 494, 495, 496, 497, 498, 499, 500, 501, 503,
                504, 506, 507, 511};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{0,0,0b00011}, {1,1,0b10001}, {2,1,0b00011}, {4,1,0b01111},
                     {7,1,0b11111}, {10,0,0b00101}, {11,1,0b11011},
                     {12,0,0b00101}, {13,0,0b00101}, {14,0,0b10011},
                     {15,1,0b10111}, {16,1,0b01011}, {17,0,0b01011},
                     {18,1,0b01101}, {19,0,0b01111}, {20,1,0b11011},
                     {21,1,0b01011}, {22,1,0b11111}, {23,0,0b00001},
                     {24,0,0b01011}, {25,0,0b10001}, {26,1,0b11001},
                     {27,1,0b10011}, {28,1,0b01101}, {30,0,0b00111},
                     {31,1,0b01101}, {32,0,0b11101}, {33,1,0b11111},
                     {34,0,0b00101}, {35,0,0b01111}, {36,1,0b11011},
                     {37,0,0b00111}, {38,0,0b10111}, {39,1,0b11001},
                     {40,1,0b01011}, {41,1,0b01111}, {42,0,0b00101},
                     {43,1,0b10101}, {44,1,0b00111}, {45,1,0b10111},
                     {46,0,0b01111}, {47,0,0b10001}, {48,1,0b10101},
                     {49,1,0b01001}, {50,1,0b10011}, {51,1,0b11011},
                     {52,1,0b11001}, {53,1,0b11101}, {54,1,0b10001},
                     {55,1,0b11001}, {56,1,0b00111}, {57,1,0b00111},
                     {58,1,0b00111}, {59,1,0b11111}, {61,1,0b11011},
                     {63,0,0b01111}, {64,0,0b10001}, {65,1,0b11001},
                     {66,1,0b10001}, {67,1,0b01111}, {71,1,0b10111},
                     {72,0,0b00001}, {73,0,0b00011}, {74,0,0b00111},
                     {75,0,0b10001}, {76,0,0b10101}, {77,0,0b11011},
                     {78,1,0b11101}, {79,1,0b00011}, {80,1,0b01001},
                     {81,0,0b01001}, {82,1,0b01011}, {83,1,0b00011},
                     {84,0,0b01011}, {85,1,0b01111}, {86,0,0b01011},
                     {87,1,0b11011}, {88,1,0b11011}, {89,1,0b10101},
                     {90,1,0b01001}, {91,1,0b10111}, {92,0,0b10111},
                     {93,1,0b11111}, {94,0,0b01101}, {95,1,0b10001},
                     {96,0,0b01001}, {97,0,0b01001}, {98,1,0b10111},
                     {99,1,0b00001}, {100,0,0b00011}, {101,0,0b01111},
                     {102,1,0b10011}, {103,1,0b01111}, {104,1,0b11111},
                     {105,0,0b01001}, {106,1,0b11101}, {107,0,0b00111},
                     {108,0,0b00111}, {109,0,0b01111}, {110,1,0b11111},
                     {111,1,0b10011}, {112,0,0b00111}, {113,1,0b11101},
                     {114,1,0b10011}, {115,1,0b01111}, {116,0,0b11001},
                     {117,1,0b11011}, {118,0,0b00101}, {119,0,0b10001},
                     {120,0,0b10101}, {121,1,0b10111}, {122,0,0b01001},
                     {123,1,0b11001}, {124,0,0b10001}, {125,1,0b11011},
                     {126,0,0b00001}, {127,1,0b11101}, {128,0,0b01111},
                     {129,1,0b10011}, {130,1,0b01111}, {131,1,0b10001},
                     {132,0,0b00111}, {133,0,0b10011}, {134,1,0b11111},
                     {135,1,0b11111}, {136,1,0b00011}, {137,0,0b00111},
                     {138,1,0b01111}, {139,1,0b10001}, {140,1,0b01101},
                     {141,1,0b01001}, {142,1,0b01001}, {143,1,0b11101},
                     {144,0,0b00011}, {145,1,0b00111}, {146,0,0b01111},
                     {147,1,0b01111}, {148,0,0b01111}, {149,1,0b11001},
                     {150,1,0b00111}, {151,0,0b01011}, {152,0,0b11001},
                     {153,1,0b11011}, {154,1,0b10011}, {155,0,0b00001},
                     {156,0,0b01001}, {157,1,0b10011}, {158,0,0b10111},
                     {159,1,0b11001}, {160,1,0b10101}, {161,1,0b10001},
                     {162,1,0b01111}, {163,1,0b01111}, {164,1,0b01001},
                     {165,0,0b00011}, {166,0,0b10111}, {167,1,0b11011},
                     {168,1,0b01101}, {169,0,0b10101}, {170,1,0b11101},
                     {171,0,0b11011}, {172,1,0b11111}, {173,0,0b01011},
                     {174,1,0b11011}, {175,0,0b00001}, {176,1,0b11101},
                     {177,1,0b10011}, {178,1,0b10101}, {179,1,0b11111},
                     {180,1,0b10011}, {181,1,0b00101}, {182,1,0b01001},
                     {183,1,0b10111}, {184,0,0b01011}, {185,0,0b01111},
                     {186,1,0b11011}, {187,1,0b01001}, {188,1,0b10111},
                     {189,1,0b01101}, {190,1,0b11011}, {191,1,0b10111},
                     {192,1,0b11011}, {193,1,0b00001}, {194,0,0b10111},
                     {195,0,0b11011}, {196,0,0b11101}, {197,1,0b11111},
                     {198,1,0b01011}, {199,1,0b00111}, {200,1,0b00111},
                     {201,0,0b00101}, {202,1,0b01101}, {203,0,0b11011},
                     {204,1,0b11111}, {205,1,0b10011}, {206,1,0b11101},
                     {207,1,0b00111}, {208,0,0b01011}, {209,1,0b11011},
                     {210,0,0b00001}, {211,0,0b10001}, {212,1,0b11011},
                     {213,1,0b01011}, {214,1,0b11001}, {215,1,0b00011},
                     {216,1,0b10001}, {217,1,0b01101}, {218,1,0b00001},
                     {219,1,0b00001}, {220,1,0b00011}, {221,1,0b01001},
                     {222,0,0b01101}, {223,0,0b10011}, {224,1,0b11101},
                     {225,1,0b01111}, {226,0,0b00011}, {227,0,0b00111},
                     {228,1,0b11111}, {229,1,0b00001}, {230,1,0b00111},
                     {231,1,0b00001}, {232,0,0b01001}, {233,1,0b01011},
                     {234,1,0b00001}, {235,0,0b01011}, {236,1,0b10011},
                     {237,1,0b01111}, {238,1,0b00011}, {239,1,0b11011},
                     {240,1,0b00011}, {241,0,0b01011}, {242,1,0b10001},
                     {243,1,0b00101}, {244,0,0b01101}, {245,1,0b10001},
                     {246,1,0b00011}, {247,0,0b00111}, {248,1,0b10001},
                     {249,0,0b00011}, {250,0,0b11011}, {251,1,0b11111},
                     {252,1,0b11001}, {253,1,0b11111}, {254,1,0b00111},
                     {255,0,0b00001}, {256,1,0b01101}, {257,0,0b00111},
                     {258,0,0b01001}, {259,1,0b11111}, {260,0,0b00001},
                     {261,1,0b00111}, {262,1,0b00101}, {263,1,0b10101},
                     {264,0,0b00101}, {265,0,0b00111}, {266,1,0b10001},
                     {267,1,0b11101}, {268,1,0b11001}, {269,0,0b00111},
                     {270,1,0b01011}, {271,1,0b10001}, {272,0,0b00011},
                     {273,0,0b01001}, {274,1,0b01101}, {275,1,0b01001},
                     {276,1,0b01011}, {277,1,0b11001}, {278,1,0b01111},
                     {279,0,0b01001}, {280,1,0b01101}, {281,0,0b00001},
                     {282,1,0b11011}, {283,1,0b01011}, {285,1,0b11101},
                     {286,1,0b00101}, {289,0,0b01011}, {290,1,0b01011},
                     {291,0,0b11011}, {292,1,0b11011}, {293,1,0b11011},
                     {294,1,0b11111}, {296,0,0b00011}, {297,0,0b10101},
                     {298,1,0b11111}, {299,0,0b01101}, {300,1,0b10111},
                     {302,1,0b00001}, {304,1,0b11011}, {305,1,0b00001},
                     {308,0,0b01001}, {309,1,0b01111}, {310,1,0b11111},
                     {311,1,0b11111}, {312,1,0b00001}, {313,1,0b10111},
                     {315,1,0b00001}, {316,0,0b10101}, {317,0,0b11101},
                     {318,0,0b11101}, {319,1,0b11101}, {320,0,0b01011},
                     {321,1,0b10011}, {322,0,0b00111}, {323,0,0b10001},
                     {324,1,0b11001}, {325,1,0b10111}, {326,0,0b00011},
                     {327,1,0b00101}, {328,1,0b10111}, {329,0,0b01111},
                     {330,1,0b10101}, {331,0,0b10011}, {332,1,0b11001},
                     {333,1,0b01101}, {334,0,0b01011}, {335,1,0b10011},
                     {336,1,0b11011}, {337,1,0b01001}, {338,1,0b00111},
                     {339,1,0b11011}, {340,1,0b10111}, {341,1,0b01001},
                     {342,0,0b01101}, {343,1,0b10111}, {344,1,0b01101},
                     {345,0,0b00001}, {346,0,0b00101}, {347,1,0b01101},
                     {350,1,0b01011}, {352,1,0b10101}, {354,1,0b10111},
                     {355,1,0b10111}, {356,0,0b00011}, {357,0,0b00011},
                     {358,0,0b00101}, {359,1,0b01001}, {360,1,0b00001},
                     {361,0,0b11001}, {362,0,0b11011}, {363,1,0b11111},
                     {364,0,0b01101}, {365,1,0b11001}, {366,0,0b00011},
                     {367,0,0b00111}, {368,0,0b10011}, {369,0,0b11101},
                     {370,1,0b11111}, {371,1,0b00001}, {372,0,0b10001},
                     {373,0,0b10001}, {374,1,0b11001}, {375,1,0b11001},
                     {376,1,0b01011}, {377,1,0b11001}, {378,1,0b01011},
                     {381,0,0b00001}, {382,0,0b11001}, {383,1,0b11111},
                     {384,0,0b00011}, {385,1,0b00011}, {386,1,0b10001},
                     {387,0,0b00011}, {388,0,0b10001}, {389,1,0b11001},
                     {390,1,0b00011}, {391,0,0b00001}, {392,0,0b01101},
                     {393,1,0b10001}, {394,0,0b11101}, {395,1,0b11111},
                     {396,1,0b00101}, {397,1,0b00001}, {398,0,0b01001},
                     {399,1,0b11101}, {400,0,0b00101}, {401,1,0b01001},
                     {402,1,0b10011}, {403,0,0b00101}, {404,0,0b00101},
                     {405,1,0b01111}, {406,0,0b01011}, {407,1,0b10001},
                     {408,1,0b01011}, {409,0,0b00111}, {410,1,0b11111},
                     {411,1,0b01101}, {412,0,0b01011}, {413,1,0b10001},
                     {414,1,0b10011}, {415,0,0b11111}, {416,1,0b11111},
                     {417,1,0b11111}, {418,1,0b10101}, {419,0,0b00111},
                     {420,0,0b10001}, {421,1,0b11111}, {422,1,0b10011},
                     {423,1,0b11111}, {424,1,0b00011}, {425,0,0b10101},
                     {426,1,0b11001}, {427,0,0b01111}, {428,0,0b10001},
                     {429,1,0b11011}, {430,0,0b00101}, {431,0,0b01111},
                     {432,0,0b10001}, {433,1,0b10001}, {434,1,0b11011},
                     {435,0,0b10011}, {436,1,0b10011}, {437,0,0b01101},
                     {438,1,0b11101}, {439,1,0b01011}, {440,1,0b00111},
                     {441,1,0b00101}, {442,1,0b10111}, {443,0,0b11011},
                     {444,1,0b11111}, {445,0,0b00001}, {446,0,0b00011},
                     {447,0,0b01001}, {448,1,0b11101}, {449,1,0b00001},
                     {450,0,0b00001}, {451,0,0b00101}, {452,1,0b10011},
                     {453,0,0b00001}, {454,0,0b01011}, {455,1,0b10001},
                     {456,0,0b10001}, {457,1,0b11101}, {458,1,0b10111},
                     {459,1,0b10011}, {460,1,0b01101}, {461,1,0b10111},
                     {462,1,0b10111}, {463,1,0b10101}, {464,1,0b00001},
                     {465,1,0b10111}, {466,0,0b10001}, {467,1,0b11011},
                     {468,1,0b11011}, {469,1,0b00011}, {470,0,0b10101},
                     {471,1,0b11001}, {472,0,0b01011}, {473,0,0b10111},
                     {474,0,0b10111}, {475,1,0b11011}, {476,0,0b01011},
                     {477,1,0b11001}, {478,1,0b10101}, {479,1,0b11111},
                     {480,0,0b00101}, {481,1,0b10011}, {482,1,0b00101},
                     {483,0,0b00101}, {484,0,0b11101}, {485,1,0b11111},
                     {486,1,0b11101}, {487,1,0b01011}, {488,1,0b00001},
                     {489,1,0b11111}, {490,1,0b00001}, {491,0,0b00011},
                     {492,0,0b00101}, {493,1,0b10001}, {494,1,0b01111},
                     {495,0,0b00011}, {496,1,0b10001}, {497,1,0b01011},
                     {498,0,0b10101}, {499,1,0b11011}, {500,1,0b00001},
                     {501,1,0b10111}, {502,0,0b01011}, {503,0,0b01111},
                     {504,1,0b11111}, {505,0,0b01111}, {506,1,0b11111},
                     {507,0,0b10101}, {508,0,0b10111}, {509,1,0b11001},
                     {510,0,0b00011}, {511,1,0b11001}, {512,1,0b01011},
                     {513,1,0b00011}, {514,1,0b11011}, {515,1,0b10001},
                     {516,1,0b10101}, {517,1,0b11011}, {518,0,0b00111},
                     {519,1,0b01111}, {520,1,0b01111}, {521,1,0b10111},
                     {522,1,0b10001}, {523,1,0b00011}, {524,1,0b11001},
                     {525,1,0b00001}, {526,0,0b00011}, {527,0,0b01001},
                     {528,1,0b11001}, {529,1,0b11011}, {530,0,0b01101},
                     {531,1,0b11111}, {532,1,0b11001}, {533,0,0b00101},
                     {534,1,0b10111}, {535,1,0b01101}, {536,1,0b00111},
                     {537,1,0b01001}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }


    static void PointQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const std::vector<uint64_t> keys {0b00000000000001, 0b00000000000101,
            0b00000000010101, 0b00000000100001, 0b00000000101000,
            0b00000000101011, 0b01000000100001, 0b01000000100011,
            0b01000000100101, 0b01000000100110, 0b01000000100110,
            0b01000000100110, 0b01000001110000, 0b01000001100011,
            0b01000001100101, 0b01111111000001, 0b01111111000011,
            0b01111111000111, 0b01111111100001, 0b01111111100010,
            0b01111111100010};
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
            for (uint64_t query_key = 0; query_key < (1ULL << (s.infix_size_ + Steroids<false>::base_implicit_size)); query_key++) {
                
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
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);
        
        const uint32_t n_queries = 100000;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        const std::vector<uint64_t> keys {0b00000000000001, 0b00000000000101,
            0b00000000010101, 0b00000000100001, 0b00000000101000,
            0b00000000101011, 0b01000000100001, 0b01000000100011,
            0b01000000100101, 0b01000000100110, 0b01000000100110,
            0b01000000100110, 0b01000001110000, 0b01000001100011,
            0b01000001100101, 0b01111111000001, 0b01111111000011,
            0b01111111000111, 0b01111111100001, 0b01111111100010,
            0b01111111100010};
        for (uint64_t key : keys)
            s.InsertRawIntoInfixStore(store, key);

        SUBCASE("no false negatives") {
            std::vector<std::pair<uint64_t, uint64_t>> queries;
            while (queries.size() < n_queries) {
                uint64_t l = rng() & BITMASK(s.infix_size_ + Steroids<false>::base_implicit_size);
                uint64_t r = rng() & BITMASK(s.infix_size_ + Steroids<false>::base_implicit_size);
                if (l > r)
                    std::swap(l, r);

                bool valid = false;
                for (uint64_t key : keys) {
                    const uint64_t key_l = key - (key & -key);
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
                uint64_t l = rng() & BITMASK(s.infix_size_ + Steroids<false>::base_implicit_size);
                uint64_t r = rng() & BITMASK(s.infix_size_ + Steroids<false>::base_implicit_size);
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

    static void ShrinkInfixSize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);
        
        const std::vector<uint64_t> keys {0b00000000000001, 0b00000000000101,
            0b00000000010101, 0b00000000100001, 0b00000000101000,
            0b00000000101011, 0b01000000100001, 0b01000000100011,
            0b01000000100101, 0b01000000100110, 0b01000000100110,
            0b01000000100110, 0b01000001110000, 0b01000001100011,
            0b01000001100101, 0b01111111000001, 0b01111111000011,
            0b01111111000111, 0b01111111100001, 0b01111111100010,
            0b01111111100010};
        for (uint64_t key : keys)
            s.InsertRawIntoInfixStore(store, key);

        SUBCASE("shrink by one") {
            s.ShrinkInfixStoreInfixSize(store, s.infix_size_ - 1);
            s.infix_size_--;
            const std::vector<uint64_t> check {0b0000000000001,
                0b0000000000011, 0b0000000001011, 0b0000000010001,
                0b0000000010100, 0b0000000010101, 0b0100000010001,
                0b0100000010001, 0b0100000010011, 0b0100000010011,
                0b0100000010011, 0b0100000010011, 0b0100000111000,
                0b0100000110001, 0b0100000110011, 0b0111111100001,
                0b0111111100001, 0b0111111100011, 0b0111111110001,
                0b0111111110001, 0b0111111110001};
            uint64_t res[check.size() + 1];
            const uint32_t len = s.GetInfixList(store, res);
            REQUIRE_EQ(len, check.size());
            for (int32_t i = 0; i < check.size(); i++)
                REQUIRE_EQ(res[i], check[i]);
        }

        SUBCASE("shrink by two") {
            s.ShrinkInfixStoreInfixSize(store, s.infix_size_ - 2);
            s.infix_size_ -= 2;
            const std::vector<uint64_t> check {0b000000000001, 0b000000000001,
                0b000000000101, 0b000000001001, 0b000000001010, 0b000000001011,
                0b010000001001, 0b010000001001, 0b010000001001, 0b010000001001,
                0b010000001001, 0b010000001001, 0b010000011100, 0b010000011001,
                0b010000011001, 0b011111110001, 0b011111110001, 0b011111110001,
                0b011111111001, 0b011111111001, 0b011111111001};
            uint64_t res[check.size() + 1];
            const uint32_t len = s.GetInfixList(store, res);
            REQUIRE_EQ(len, check.size());
            for (int32_t i = 0; i < check.size(); i++)
                REQUIRE_EQ(res[i], check[i]);
        }
    }


    static void Resize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);
        Steroids<false>::InfixStore store(s.scaled_sizes_[0], s.infix_size_);
        
        const uint32_t n_keys = 512;
        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);
        for (int32_t i = 0; i < n_keys; i++)
            s.InsertRawIntoInfixStore(store, (rng() & BITMASK(Steroids<false>::base_implicit_size + infix_size)) | 1ULL);

        s.ResizeInfixStore(store, true);

        SUBCASE("expand") {
            const std::vector<uint32_t> occupieds_pos = {0, 1, 3, 5, 8, 9, 10,
                11, 12, 14, 16, 17, 18, 19, 20, 23, 24, 25, 26, 29, 31, 33, 36,
                37, 38, 43, 44, 45, 47, 48, 50, 51, 53, 55, 56, 57, 59, 61, 62,
                64, 67, 69, 70, 72, 73, 76, 77, 78, 80, 82, 83, 84, 85, 86, 87,
                91, 92, 93, 96, 99, 100, 102, 104, 106, 107, 110, 111, 113,
                114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 127,
                128, 129, 130, 132, 133, 134, 135, 136, 138, 139, 140, 141,
                143, 144, 145, 146, 147, 149, 150, 151, 153, 154, 156, 157,
                158, 159, 163, 164, 168, 171, 172, 173, 174, 176, 178, 182,
                183, 184, 185, 186, 191, 192, 196, 197, 198, 199, 200, 201,
                202, 204, 206, 208, 210, 211, 212, 213, 215, 216, 218, 219,
                220, 221, 223, 224, 225, 226, 227, 229, 230, 234, 235, 236,
                237, 239, 240, 241, 242, 244, 246, 247, 249, 250, 251, 253,
                254, 255, 256, 257, 259, 260, 261, 263, 264, 268, 269, 270,
                272, 273, 276, 278, 279, 280, 281, 282, 285, 286, 287, 288,
                289, 290, 291, 292, 293, 295, 296, 297, 298, 299, 300, 303,
                304, 305, 306, 307, 309, 314, 316, 317, 318, 321, 322, 323,
                324, 326, 329, 330, 331, 335, 337, 338, 340, 342, 343, 344,
                347, 348, 350, 351, 352, 356, 358, 359, 360, 362, 364, 369,
                371, 373, 375, 377, 378, 379, 382, 384, 387, 392, 394, 395,
                396, 398, 399, 401, 403, 404, 407, 408, 410, 411, 412, 413,
                417, 418, 419, 420, 423, 425, 426, 427, 429, 430, 431, 433,
                434, 435, 436, 439, 441, 444, 445, 446, 450, 451, 453, 454,
                455, 456, 457, 458, 460, 461, 462, 463, 466, 467, 469, 470,
                471, 473, 474, 475, 478, 480, 482, 484, 486, 488, 490, 491,
                492, 494, 495, 497, 498, 499, 500, 501, 503, 504, 505, 507,
                509, 510};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{0,1,0b10111}, {1,1,0b10001}, {3,1,0b11111}, {5,1,0b00111},
                     {8,1,0b01001}, {9,0,0b00001}, {10,1,0b11001},
                     {11,0,0b00111}, {12,0,0b10101}, {13,1,0b10111},
                     {14,1,0b01011}, {15,0,0b00001}, {16,1,0b10111},
                     {17,0,0b01111}, {18,1,0b10001}, {19,0,0b01101},
                     {20,1,0b10111}, {21,1,0b01001}, {22,1,0b01011},
                     {23,0,0b11011}, {24,1,0b11101}, {25,0,0b00001},
                     {26,0,0b00111}, {27,0,0b10101}, {28,1,0b11011},
                     {29,0,0b00001}, {30,1,0b10101}, {31,1,0b11001},
                     {32,0,0b10011}, {33,1,0b11011}, {34,0,0b00011},
                     {35,0,0b10111}, {36,1,0b11111}, {37,1,0b01111},
                     {38,0,0b00001}, {39,0,0b00101}, {40,0,0b01001},
                     {41,1,0b01101}, {42,1,0b10101}, {43,1,0b00011},
                     {44,0,0b00001}, {45,1,0b00101}, {46,1,0b00111},
                     {47,0,0b01011}, {48,1,0b01011}, {49,1,0b10111},
                     {50,1,0b01111}, {52,1,0b11101}, {53,1,0b11001},
                     {55,1,0b11101}, {56,1,0b00101}, {58,0,0b01111},
                     {59,1,0b10111}, {60,1,0b01101}, {62,1,0b01111},
                     {63,0,0b00001}, {64,1,0b01101}, {65,0,0b00001},
                     {66,1,0b01111}, {67,1,0b11101}, {68,0,0b01001},
                     {69,1,0b11001}, {70,0,0b00011}, {71,1,0b10101},
                     {74,0,0b10101}, {75,0,0b11011}, {76,1,0b11111},
                     {77,1,0b11011}, {78,1,0b00001}, {79,1,0b10111},
                     {80,0,0b10111}, {81,1,0b11111}, {84,1,0b10101},
                     {85,1,0b00111}, {86,1,0b11011}, {88,1,0b01011},
                     {90,0,0b00011}, {91,0,0b00101}, {92,1,0b01101},
                     {93,0,0b10001}, {94,1,0b11001}, {95,1,0b11001},
                     {96,0,0b10001}, {97,1,0b10011}, {98,1,0b11111},
                     {99,0,0b00101}, {100,1,0b11001}, {101,1,0b00011},
                     {102,0,0b00101}, {103,1,0b11001}, {104,0,0b00011},
                     {105,0,0b00101}, {106,1,0b01111}, {107,0,0b00111},
                     {108,0,0b01111}, {109,0,0b10001}, {110,1,0b11001},
                     {111,0,0b00001}, {112,0,0b11101}, {113,1,0b11101},
                     {114,1,0b00011}, {115,0,0b00101}, {116,0,0b01001},
                     {117,0,0b10011}, {118,1,0b11111}, {119,1,0b10001},
                     {120,1,0b01111}, {121,0,0b00011}, {122,1,0b10001},
                     {123,0,0b01101}, {124,1,0b11111}, {125,1,0b10001},
                     {126,1,0b00011}, {127,0,0b00001}, {128,1,0b11101},
                     {129,1,0b01101}, {130,1,0b11011}, {131,1,0b00011},
                     {132,0,0b00101}, {133,0,0b01001}, {134,0,0b01111},
                     {135,1,0b11101}, {136,0,0b00001}, {137,1,0b01101},
                     {138,0,0b11001}, {139,1,0b11111}, {140,1,0b00111},
                     {141,0,0b01001}, {142,0,0b10011}, {143,1,0b11011},
                     {144,0,0b01101}, {145,0,0b01111}, {146,1,0b11001},
                     {147,1,0b00101}, {148,1,0b10011}, {149,1,0b01111},
                     {150,1,0b11011}, {151,0,0b00001}, {152,1,0b01101},
                     {153,1,0b10001}, {154,1,0b01111}, {155,1,0b11011},
                     {156,0,0b01111}, {157,1,0b11001}, {158,0,0b01011},
                     {159,1,0b11101}, {160,0,0b00101}, {161,1,0b01011},
                     {162,1,0b00101}, {163,1,0b00111}, {164,0,0b01001},
                     {165,1,0b11111}, {166,1,0b11001}, {167,0,0b01101},
                     {168,0,0b11101}, {169,1,0b11101}, {170,0,0b01001},
                     {171,1,0b01011}, {172,1,0b11001}, {173,1,0b01111},
                     {174,1,0b01011}, {175,1,0b00001}, {176,0,0b00101},
                     {177,1,0b01111}, {178,1,0b00111}, {179,1,0b01001},
                     {180,0,0b11101}, {181,0,0b11111}, {182,1,0b11111},
                     {183,0,0b00001}, {184,1,0b10001}, {185,1,0b01111},
                     {186,0,0b00001}, {187,0,0b01011}, {188,1,0b11001},
                     {189,0,0b01011}, {190,1,0b01111}, {191,1,0b01011},
                     {192,0,0b11001}, {193,1,0b11111}, {194,0,0b11011},
                     {195,1,0b11111}, {196,1,0b10001}, {197,1,0b11011},
                     {198,0,0b00111}, {199,1,0b10101}, {200,1,0b00101},
                     {201,0,0b01101}, {202,1,0b10011}, {203,1,0b00111},
                     {204,1,0b01101}, {205,1,0b11001}, {206,0,0b00011},
                     {207,1,0b01011}, {208,1,0b10011}, {211,1,0b00011},
                     {212,1,0b01101}, {217,1,0b00001}, {218,1,0b00001},
                     {219,1,0b11101}, {220,1,0b10101}, {221,1,0b01011},
                     {222,1,0b10111}, {223,0,0b01111}, {224,0,0b10001},
                     {225,1,0b11001}, {226,0,0b10011}, {227,1,0b10101},
                     {228,1,0b00101}, {230,1,0b11101}, {232,0,0b01101},
                     {233,0,0b10101}, {234,1,0b11101}, {235,1,0b01111},
                     {236,1,0b11101}, {237,1,0b00101}, {238,1,0b01011},
                     {239,0,0b00001}, {240,1,0b10101}, {241,1,0b10011},
                     {242,1,0b00011}, {243,1,0b10101}, {244,0,0b00111},
                     {245,0,0b01001}, {246,1,0b10101}, {247,1,0b10011},
                     {248,1,0b10101}, {249,0,0b00101}, {250,1,0b10111},
                     {251,1,0b11011}, {252,1,0b11101}, {253,0,0b00011},
                     {254,1,0b10111}, {255,0,0b00101}, {256,1,0b10011},
                     {259,0,0b00001}, {260,1,0b01101}, {261,1,0b10111},
                     {262,0,0b01011}, {263,0,0b10111}, {264,1,0b10111},
                     {265,0,0b01011}, {266,0,0b11011}, {267,1,0b11111},
                     {268,1,0b01101}, {269,0,0b00101}, {270,1,0b01101},
                     {271,1,0b00001}, {272,1,0b11001}, {273,1,0b11111},
                     {274,0,0b00001}, {275,0,0b00011}, {276,0,0b00101},
                     {277,0,0b00101}, {278,1,0b01101}, {279,0,0b00011},
                     {280,1,0b00011}, {281,0,0b01111}, {282,1,0b10001},
                     {283,1,0b00111}, {284,0,0b01101}, {285,1,0b10111},
                     {286,1,0b11111}, {287,0,0b11011}, {288,1,0b11011},
                     {289,1,0b10101}, {290,1,0b11011}, {291,1,0b10011},
                     {292,0,0b01111}, {293,0,0b10101}, {294,1,0b11001},
                     {295,1,0b11111}, {296,1,0b11011}, {297,1,0b11011},
                     {298,0,0b00001}, {299,1,0b01111}, {300,0,0b10101},
                     {301,1,0b11111}, {302,1,0b00111}, {303,1,0b00001},
                     {304,0,0b10101}, {305,1,0b10111}, {306,0,0b00011},
                     {307,1,0b11101}, {308,1,0b01111}, {309,0,0b00011},
                     {310,1,0b11001}, {311,1,0b01011}, {312,1,0b01011},
                     {313,1,0b00101}, {314,0,0b00111}, {315,0,0b10001},
                     {316,1,0b10011}, {317,0,0b01101}, {318,0,0b11011},
                     {319,1,0b11111}, {320,0,0b01011}, {321,0,0b10001},
                     {322,1,0b11101}, {323,1,0b01011}, {324,0,0b00011},
                     {325,1,0b10001}, {326,0,0b00101}, {327,1,0b01011},
                     {328,0,0b00011}, {329,1,0b01011}, {330,1,0b00011},
                     {331,1,0b01111}, {332,1,0b11011}, {333,0,0b00001},
                     {334,0,0b01011}, {335,0,0b01101}, {336,1,0b10011},
                     {337,1,0b11001}, {338,1,0b01001}, {339,0,0b00111},
                     {340,0,0b10001}, {341,1,0b10101}, {342,1,0b00001},
                     {343,0,0b01101}, {344,0,0b11101}, {345,1,0b11111},
                     {346,1,0b00001}, {347,1,0b00001}, {348,1,0b10011},
                     {349,0,0b00111}, {350,0,0b00111}, {351,0,0b10001},
                     {352,1,0b11111}, {353,0,0b01111}, {354,0,0b10001},
                     {355,1,0b11111}, {356,1,0b00111}, {357,0,0b10011},
                     {358,1,0b11011}, {359,1,0b00001}, {360,1,0b00011},
                     {361,0,0b00011}, {362,1,0b10011}, {363,0,0b01111},
                     {364,0,0b10101}, {365,0,0b11011}, {366,1,0b11011},
                     {367,1,0b01001}, {368,1,0b00101}, {369,0,0b01111},
                     {370,1,0b10101}, {371,1,0b11001}, {372,1,0b11011},
                     {373,1,0b00011}, {374,1,0b11111}, {375,0,0b10011},
                     {376,1,0b11111}, {377,1,0b01001}, {378,1,0b10001},
                     {379,0,0b00001}, {380,1,0b00111}, {381,1,0b11011},
                     {382,1,0b11001}, {383,0,0b01001}, {384,1,0b11001},
                     {385,0,0b10101}, {386,1,0b10101}, {387,0,0b00001},
                     {388,0,0b00101}, {389,1,0b10111}, {390,0,0b00101},
                     {391,1,0b11101}, {392,0,0b00001}, {393,0,0b01101},
                     {394,0,0b10111}, {395,1,0b11011}, {396,1,0b01001},
                     {397,1,0b11001}, {398,1,0b00111}, {399,0,0b11011},
                     {400,1,0b11101}, {401,1,0b10101}, {402,1,0b01111},
                     {403,1,0b00001}, {408,0,0b00001}, {409,1,0b10011},
                     {411,0,0b11101}, {412,1,0b11111}, {413,0,0b00101},
                     {414,1,0b01101}, {415,1,0b11001}, {417,1,0b11011},
                     {418,0,0b10001}, {419,1,0b11001}, {420,1,0b00111},
                     {423,1,0b10001}, {425,1,0b00001}, {428,1,0b11011},
                     {434,1,0b01001}, {436,1,0b00001}, {437,1,0b00011},
                     {438,1,0b10001}, {440,0,0b01111}, {441,1,0b11111},
                     {442,1,0b10001}, {444,0,0b00101}, {445,0,0b00101},
                     {446,0,0b01011}, {447,0,0b10001}, {448,1,0b10101},
                     {449,0,0b00111}, {450,0,0b01011}, {451,0,0b10011},
                     {452,1,0b10111}, {453,0,0b11101}, {454,1,0b11101},
                     {455,1,0b10001}, {456,1,0b01001}, {457,1,0b01101},
                     {458,0,0b10001}, {459,1,0b10011}, {460,1,0b11011},
                     {461,0,0b01101}, {462,1,0b11001}, {463,1,0b10011},
                     {464,0,0b01111}, {465,0,0b10101}, {466,1,0b11001},
                     {467,1,0b01111}, {468,1,0b00001}, {469,1,0b10011},
                     {470,1,0b11011}, {472,0,0b00101}, {473,1,0b01001},
                     {474,0,0b00001}, {475,1,0b01101}, {476,0,0b01001},
                     {477,0,0b10001}, {478,1,0b11101}, {479,0,0b01101},
                     {480,1,0b11101}, {481,1,0b00011}, {482,1,0b10111},
                     {483,0,0b00111}, {484,0,0b11101}, {485,1,0b11111},
                     {486,1,0b10001}, {487,1,0b01001}, {488,0,0b00011},
                     {489,0,0b00101}, {490,1,0b11101}, {491,1,0b00101},
                     {492,0,0b10111}, {493,1,0b11011}, {494,1,0b01101},
                     {495,0,0b10101}, {496,1,0b11011}, {498,1,0b01111},
                     {499,1,0b01011}, {501,0,0b00001}, {502,1,0b11111},
                     {503,1,0b00111}, {504,0,0b01111}, {505,1,0b11001},
                     {506,0,0b01111}, {507,0,0b10011}, {508,1,0b11111},
                     {509,1,0b00011}, {510,1,0b00111}, {511,0,0b11011},
                     {512,1,0b11111}, {513,1,0b11111}, {514,1,0b11001},
                     {515,1,0b00111}, {516,1,0b00011}, {517,1,0b11111},
                     {519,0,0b00111}, {520,0,0b10001}, {521,1,0b11001},
                     {522,1,0b10001}, {523,0,0b00101}, {524,0,0b10011},
                     {525,1,0b10111}, {526,0,0b10001}, {527,1,0b11011},
                     {528,1,0b01001}, {529,1,0b10001}, {530,1,0b00011},
                     {531,1,0b01101}, {534,1,0b01001}, {536,1,0b00001},
                     {538,1,0b11001}, {540,1,0b11111}, {542,1,0b10001},
                     {544,1,0b01001}, {545,1,0b11101}, {547,0,0b00101},
                     {548,1,0b01001}, {549,0,0b00011}, {550,1,0b11011},
                     {551,0,0b01001}, {552,1,0b10111}, {553,1,0b00111},
                     {554,1,0b00111}, {555,1,0b00001}, {556,0,0b00011},
                     {557,1,0b01001}, {558,0,0b00101}, {559,0,0b00111},
                     {560,1,0b10001}, {561,1,0b10101}, {562,1,0b00011},
                     {563,1,0b01101}, {564,0,0b10011}, {565,1,0b11111},
                     {566,1,0b01111}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        s.ResizeInfixStore(store, false);

        SUBCASE("contract") {
            const std::vector<uint32_t> occupieds_pos = {0, 1, 3, 5, 8, 9, 10,
                11, 12, 14, 16, 17, 18, 19, 20, 23, 24, 25, 26, 29, 31, 33, 36,
                37, 38, 43, 44, 45, 47, 48, 50, 51, 53, 55, 56, 57, 59, 61, 62,
                64, 67, 69, 70, 72, 73, 76, 77, 78, 80, 82, 83, 84, 85, 86, 87,
                91, 92, 93, 96, 99, 100, 102, 104, 106, 107, 110, 111, 113,
                114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 127,
                128, 129, 130, 132, 133, 134, 135, 136, 138, 139, 140, 141,
                143, 144, 145, 146, 147, 149, 150, 151, 153, 154, 156, 157,
                158, 159, 163, 164, 168, 171, 172, 173, 174, 176, 178, 182,
                183, 184, 185, 186, 191, 192, 196, 197, 198, 199, 200, 201,
                202, 204, 206, 208, 210, 211, 212, 213, 215, 216, 218, 219,
                220, 221, 223, 224, 225, 226, 227, 229, 230, 234, 235, 236,
                237, 239, 240, 241, 242, 244, 246, 247, 249, 250, 251, 253,
                254, 255, 256, 257, 259, 260, 261, 263, 264, 268, 269, 270,
                272, 273, 276, 278, 279, 280, 281, 282, 285, 286, 287, 288,
                289, 290, 291, 292, 293, 295, 296, 297, 298, 299, 300, 303,
                304, 305, 306, 307, 309, 314, 316, 317, 318, 321, 322, 323,
                324, 326, 329, 330, 331, 335, 337, 338, 340, 342, 343, 344,
                347, 348, 350, 351, 352, 356, 358, 359, 360, 362, 364, 369,
                371, 373, 375, 377, 378, 379, 382, 384, 387, 392, 394, 395,
                396, 398, 399, 401, 403, 404, 407, 408, 410, 411, 412, 413,
                417, 418, 419, 420, 423, 425, 426, 427, 429, 430, 431, 433,
                434, 435, 436, 439, 441, 444, 445, 446, 450, 451, 453, 454,
                455, 456, 457, 458, 460, 461, 462, 463, 466, 467, 469, 470,
                471, 473, 474, 475, 478, 480, 482, 484, 486, 488, 490, 491,
                492, 494, 495, 497, 498, 499, 500, 501, 503, 504, 505, 507,
                509, 510};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{0,1,0b10111}, {1,1,0b10001}, {3,1,0b11111}, {5,1,0b00111},
                     {8,1,0b01001}, {9,0,0b00001}, {10,1,0b11001},
                     {11,0,0b00111}, {12,0,0b10101}, {13,1,0b10111},
                     {14,1,0b01011}, {15,0,0b00001}, {16,1,0b10111},
                     {17,0,0b01111}, {18,1,0b10001}, {19,0,0b01101},
                     {20,1,0b10111}, {21,1,0b01001}, {22,1,0b01011},
                     {23,0,0b11011}, {24,1,0b11101}, {25,0,0b00001},
                     {26,0,0b00111}, {27,0,0b10101}, {28,1,0b11011},
                     {29,0,0b00001}, {30,1,0b10101}, {31,1,0b11001},
                     {32,0,0b10011}, {33,1,0b11011}, {34,0,0b00011},
                     {35,0,0b10111}, {36,1,0b11111}, {37,1,0b01111},
                     {38,0,0b00001}, {39,0,0b00101}, {40,0,0b01001},
                     {41,1,0b01101}, {42,1,0b10101}, {43,1,0b00011},
                     {44,0,0b00001}, {45,1,0b00101}, {46,1,0b00111},
                     {47,0,0b01011}, {48,1,0b01011}, {49,1,0b10111},
                     {50,1,0b01111}, {51,1,0b11101}, {52,1,0b11001},
                     {53,1,0b11101}, {54,1,0b00101}, {55,0,0b01111},
                     {56,1,0b10111}, {57,1,0b01101}, {58,1,0b01111},
                     {59,0,0b00001}, {60,1,0b01101}, {62,0,0b00001},
                     {63,1,0b01111}, {64,1,0b11101}, {65,0,0b01001},
                     {66,1,0b11001}, {67,0,0b00011}, {68,1,0b10101},
                     {70,0,0b10101}, {71,0,0b11011}, {72,1,0b11111},
                     {73,1,0b11011}, {74,1,0b00001}, {75,1,0b10111},
                     {76,0,0b10111}, {77,1,0b11111}, {79,1,0b10101},
                     {81,1,0b00111}, {82,1,0b11011}, {84,1,0b01011},
                     {86,0,0b00011}, {87,0,0b00101}, {88,1,0b01101},
                     {89,0,0b10001}, {90,1,0b11001}, {91,1,0b11001},
                     {92,0,0b10001}, {93,1,0b10011}, {94,1,0b11111},
                     {95,0,0b00101}, {96,1,0b11001}, {97,1,0b00011},
                     {98,0,0b00101}, {99,1,0b11001}, {100,0,0b00011},
                     {101,0,0b00101}, {102,1,0b01111}, {103,0,0b00111},
                     {104,0,0b01111}, {105,0,0b10001}, {106,1,0b11001},
                     {107,0,0b00001}, {108,0,0b11101}, {109,1,0b11101},
                     {110,1,0b00011}, {111,0,0b00101}, {112,0,0b01001},
                     {113,0,0b10011}, {114,1,0b11111}, {115,1,0b10001},
                     {116,1,0b01111}, {117,0,0b00011}, {118,1,0b10001},
                     {119,0,0b01101}, {120,1,0b11111}, {121,1,0b10001},
                     {122,1,0b00011}, {123,0,0b00001}, {124,1,0b11101},
                     {125,1,0b01101}, {126,1,0b11011}, {127,1,0b00011},
                     {128,0,0b00101}, {129,0,0b01001}, {130,0,0b01111},
                     {131,1,0b11101}, {132,0,0b00001}, {133,1,0b01101},
                     {134,0,0b11001}, {135,1,0b11111}, {136,1,0b00111},
                     {137,0,0b01001}, {138,0,0b10011}, {139,1,0b11011},
                     {140,0,0b01101}, {141,0,0b01111}, {142,1,0b11001},
                     {143,1,0b00101}, {144,1,0b10011}, {145,1,0b01111},
                     {146,1,0b11011}, {147,0,0b00001}, {148,1,0b01101},
                     {149,1,0b10001}, {150,1,0b01111}, {151,1,0b11011},
                     {152,0,0b01111}, {153,1,0b11001}, {154,0,0b01011},
                     {155,1,0b11101}, {156,0,0b00101}, {157,1,0b01011},
                     {158,1,0b00101}, {159,1,0b00111}, {160,0,0b01001},
                     {161,1,0b11111}, {162,1,0b11001}, {163,0,0b01101},
                     {164,0,0b11101}, {165,1,0b11101}, {166,0,0b01001},
                     {167,1,0b01011}, {168,1,0b11001}, {169,1,0b01111},
                     {170,1,0b01011}, {171,1,0b00001}, {172,0,0b00101},
                     {173,1,0b01111}, {174,1,0b00111}, {175,1,0b01001},
                     {176,0,0b11101}, {177,0,0b11111}, {178,1,0b11111},
                     {179,0,0b00001}, {180,1,0b10001}, {181,1,0b01111},
                     {182,0,0b00001}, {183,0,0b01011}, {184,1,0b11001},
                     {185,0,0b01011}, {186,1,0b01111}, {187,1,0b01011},
                     {188,0,0b11001}, {189,1,0b11111}, {190,0,0b11011},
                     {191,1,0b11111}, {192,1,0b10001}, {193,1,0b11011},
                     {194,0,0b00111}, {195,1,0b10101}, {196,1,0b00101},
                     {197,0,0b01101}, {198,1,0b10011}, {199,1,0b00111},
                     {200,1,0b01101}, {201,1,0b11001}, {202,0,0b00011},
                     {203,1,0b01011}, {204,1,0b10011}, {205,1,0b00011},
                     {206,1,0b01101}, {207,1,0b00001}, {208,1,0b00001},
                     {209,1,0b11101}, {210,1,0b10101}, {211,1,0b01011},
                     {212,1,0b10111}, {213,0,0b01111}, {214,0,0b10001},
                     {215,1,0b11001}, {216,0,0b10011}, {217,1,0b10101},
                     {218,1,0b00101}, {219,1,0b11101}, {221,0,0b01101},
                     {222,0,0b10101}, {223,1,0b11101}, {224,1,0b01111},
                     {225,1,0b11101}, {226,1,0b00101}, {227,1,0b01011},
                     {228,0,0b00001}, {229,1,0b10101}, {230,1,0b10011},
                     {231,1,0b00011}, {232,1,0b10101}, {233,0,0b00111},
                     {234,0,0b01001}, {235,1,0b10101}, {236,1,0b10011},
                     {237,1,0b10101}, {238,0,0b00101}, {239,1,0b10111},
                     {240,1,0b11011}, {241,1,0b11101}, {242,0,0b00011},
                     {243,1,0b10111}, {244,0,0b00101}, {245,1,0b10011},
                     {246,0,0b00001}, {247,1,0b01101}, {248,1,0b10111},
                     {249,0,0b01011}, {250,0,0b10111}, {251,1,0b10111},
                     {252,0,0b01011}, {253,0,0b11011}, {254,1,0b11111},
                     {255,1,0b01101}, {256,0,0b00101}, {257,1,0b01101},
                     {258,1,0b00001}, {259,1,0b11001}, {260,1,0b11111},
                     {261,0,0b00001}, {262,0,0b00011}, {263,0,0b00101},
                     {264,0,0b00101}, {265,1,0b01101}, {266,0,0b00011},
                     {267,1,0b00011}, {268,0,0b01111}, {269,1,0b10001},
                     {270,1,0b00111}, {271,0,0b01101}, {272,1,0b10111},
                     {273,1,0b11111}, {274,0,0b11011}, {275,1,0b11011},
                     {276,1,0b10101}, {277,1,0b11011}, {278,1,0b10011},
                     {279,0,0b01111}, {280,0,0b10101}, {281,1,0b11001},
                     {282,1,0b11111}, {283,1,0b11011}, {284,1,0b11011},
                     {285,0,0b00001}, {286,1,0b01111}, {287,0,0b10101},
                     {288,1,0b11111}, {289,1,0b00111}, {290,1,0b00001},
                     {291,0,0b10101}, {292,1,0b10111}, {293,0,0b00011},
                     {294,1,0b11101}, {295,1,0b01111}, {296,0,0b00011},
                     {297,1,0b11001}, {298,1,0b01011}, {299,1,0b01011},
                     {300,1,0b00101}, {301,0,0b00111}, {302,0,0b10001},
                     {303,1,0b10011}, {304,0,0b01101}, {305,0,0b11011},
                     {306,1,0b11111}, {307,0,0b01011}, {308,0,0b10001},
                     {309,1,0b11101}, {310,1,0b01011}, {311,0,0b00011},
                     {312,1,0b10001}, {313,0,0b00101}, {314,1,0b01011},
                     {315,0,0b00011}, {316,1,0b01011}, {317,1,0b00011},
                     {318,1,0b01111}, {319,1,0b11011}, {320,0,0b00001},
                     {321,0,0b01011}, {322,0,0b01101}, {323,1,0b10011},
                     {324,1,0b11001}, {325,1,0b01001}, {326,0,0b00111},
                     {327,0,0b10001}, {328,1,0b10101}, {329,1,0b00001},
                     {330,0,0b01101}, {331,0,0b11101}, {332,1,0b11111},
                     {333,1,0b00001}, {334,1,0b00001}, {335,1,0b10011},
                     {336,0,0b00111}, {337,0,0b00111}, {338,0,0b10001},
                     {339,1,0b11111}, {340,0,0b01111}, {341,0,0b10001},
                     {342,1,0b11111}, {343,1,0b00111}, {344,0,0b10011},
                     {345,1,0b11011}, {346,1,0b00001}, {347,1,0b00011},
                     {348,0,0b00011}, {349,1,0b10011}, {350,0,0b01111},
                     {351,0,0b10101}, {352,0,0b11011}, {353,1,0b11011},
                     {354,1,0b01001}, {355,1,0b00101}, {356,0,0b01111},
                     {357,1,0b10101}, {358,1,0b11001}, {359,1,0b11011},
                     {360,1,0b00011}, {361,1,0b11111}, {362,0,0b10011},
                     {363,1,0b11111}, {364,1,0b01001}, {365,1,0b10001},
                     {366,0,0b00001}, {367,1,0b00111}, {368,1,0b11011},
                     {369,1,0b11001}, {370,0,0b01001}, {371,1,0b11001},
                     {372,0,0b10101}, {373,1,0b10101}, {374,0,0b00001},
                     {375,0,0b00101}, {376,1,0b10111}, {377,0,0b00101},
                     {378,1,0b11101}, {379,0,0b00001}, {380,0,0b01101},
                     {381,0,0b10111}, {382,1,0b11011}, {383,1,0b01001},
                     {384,1,0b11001}, {385,1,0b00111}, {386,0,0b11011},
                     {387,1,0b11101}, {388,1,0b10101}, {389,1,0b01111},
                     {390,1,0b00001}, {391,0,0b00001}, {392,1,0b10011},
                     {393,0,0b11101}, {394,1,0b11111}, {395,0,0b00101},
                     {396,1,0b01101}, {397,1,0b11001}, {398,1,0b11011},
                     {399,0,0b10001}, {400,1,0b11001}, {401,1,0b00111},
                     {402,1,0b10001}, {404,1,0b00001}, {407,1,0b11011},
                     {412,1,0b01001}, {414,1,0b00001}, {415,1,0b00011},
                     {416,1,0b10001}, {418,0,0b01111}, {419,1,0b11111},
                     {420,1,0b10001}, {422,0,0b00101}, {423,0,0b00101},
                     {424,0,0b01011}, {425,0,0b10001}, {426,1,0b10101},
                     {427,0,0b00111}, {428,0,0b01011}, {429,0,0b10011},
                     {430,1,0b10111}, {431,0,0b11101}, {432,1,0b11101},
                     {433,1,0b10001}, {434,1,0b01001}, {435,1,0b01101},
                     {436,0,0b10001}, {437,1,0b10011}, {438,1,0b11011},
                     {439,0,0b01101}, {440,1,0b11001}, {441,1,0b10011},
                     {442,0,0b01111}, {443,0,0b10101}, {444,1,0b11001},
                     {445,1,0b01111}, {446,1,0b00001}, {447,1,0b10011},
                     {448,1,0b11011}, {449,0,0b00101}, {450,1,0b01001},
                     {451,0,0b00001}, {452,1,0b01101}, {453,0,0b01001},
                     {454,0,0b10001}, {455,1,0b11101}, {456,0,0b01101},
                     {457,1,0b11101}, {458,1,0b00011}, {459,1,0b10111},
                     {460,0,0b00111}, {461,0,0b11101}, {462,1,0b11111},
                     {463,1,0b10001}, {464,1,0b01001}, {465,0,0b00011},
                     {466,0,0b00101}, {467,1,0b11101}, {468,1,0b00101},
                     {469,0,0b10111}, {470,1,0b11011}, {471,1,0b01101},
                     {472,0,0b10101}, {473,1,0b11011}, {474,1,0b01111},
                     {475,1,0b01011}, {476,0,0b00001}, {477,1,0b11111},
                     {478,1,0b00111}, {479,0,0b01111}, {480,1,0b11001},
                     {481,0,0b01111}, {482,0,0b10011}, {483,1,0b11111},
                     {484,1,0b00011}, {485,1,0b00111}, {486,0,0b11011},
                     {487,1,0b11111}, {488,1,0b11111}, {489,1,0b11001},
                     {490,1,0b00111}, {491,1,0b00011}, {492,1,0b11111},
                     {493,0,0b00111}, {494,0,0b10001}, {495,1,0b11001},
                     {496,1,0b10001}, {497,0,0b00101}, {498,0,0b10011},
                     {499,1,0b10111}, {500,0,0b10001}, {501,1,0b11011},
                     {502,1,0b01001}, {503,1,0b10001}, {504,1,0b00011},
                     {505,1,0b01101}, {507,1,0b01001}, {509,1,0b00001},
                     {511,1,0b11001}, {513,1,0b11111}, {515,1,0b10001},
                     {516,1,0b01001}, {517,1,0b11101}, {518,0,0b00101},
                     {519,1,0b01001}, {520,0,0b00011}, {521,1,0b11011},
                     {522,0,0b01001}, {523,1,0b10111}, {524,1,0b00111},
                     {525,1,0b00111}, {526,1,0b00001}, {527,0,0b00011},
                     {528,1,0b01001}, {529,0,0b00101}, {530,0,0b00111},
                     {531,1,0b10001}, {532,1,0b10101}, {533,1,0b00011},
                     {534,1,0b01101}, {535,0,0b10011}, {536,1,0b11111},
                     {537,1,0b01111}};
            AssertStoreContents(s, store, occupieds_pos, checks);
        }
    }

private:
    static void AssertStoreContents(const Steroids<false>& s, const Steroids<false>::InfixStore& store,
                                    const std::vector<uint32_t>& occupieds_pos,
                                    const std::vector<std::tuple<uint32_t, bool, uint64_t>>& checks) {
        REQUIRE_NE(store.ptr, nullptr);
        REQUIRE_EQ(store.GetElemCount(), checks.size());
        const uint64_t *occupieds = store.ptr;
        const uint64_t *runends = store.ptr + Steroids<false>::infix_store_target_size / 64;
        uint32_t ind = 0;
        for (uint32_t i = 0; i < Steroids<false>::infix_store_target_size; i++) {
            if (ind < occupieds_pos.size() && i == occupieds_pos[ind]) {
                REQUIRE_EQ(get_bitmap_bit(occupieds, i), 1);
                ind++;
            }
            else 
                REQUIRE_EQ(get_bitmap_bit(occupieds, i), 0);
        }

        const uint32_t total_size = s.scaled_sizes_[store.GetSizeGrade()];
        ind = 0;
        uint32_t runend_count = 0;
        for (int32_t i = 0; i < total_size; i++) {
            const uint64_t slot = s.GetSlot(store, i);
            if (ind < checks.size()) {
                const auto [pos, runend, value] = checks[ind];
                if (i == pos) {
                    REQUIRE_EQ(value, slot);
                    REQUIRE_EQ(get_bitmap_bit(runends, i), runend);
                    runend_count += runend;
                    ind++;
                }
                else {
                    REQUIRE_EQ(slot, 0ULL);
                    REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
                }
            }
            else {
                REQUIRE_EQ(slot, 0ULL);
                REQUIRE_EQ(get_bitmap_bit(runends, i), 0);
            }
        }
        REQUIRE_EQ(occupieds_pos.size(), runend_count);
    }

    static void PrintStore(const Steroids<false>& s, const Steroids<false>::InfixStore& store) {
        const uint32_t size_grade = store.GetSizeGrade();
        const uint64_t *occupieds = store.ptr;
        const uint64_t *runends = store.ptr + Steroids<false>::infix_store_target_size / 64;

        std::cerr << "is_partial=" << store.IsPartialKey() << " invalid_bits=" << store.GetInvalidBits();
        std::cerr << " size_grade=" << size_grade << " elem_count=" << store.GetElemCount() << std::endl;
        std::cerr << "occupieds: ";
        for (int32_t i = 0; i < Steroids<false>::infix_store_target_size; i++) {
            if ((store.ptr[i / 64] >> (i % 64)) & 1ULL)
                std::cerr << i << ' ';
        }
        std::cerr << std::endl << "runends + slots:" << std::endl;;
        int32_t cnt = 0;
        for (int32_t i = 0; i < s.scaled_sizes_[size_grade]; i++) {
            const uint64_t value = s.GetSlot(store, i);
            if (value == 0)
                continue;
            std::cerr << std::setfill(' ') << std::setw(3) << i;
            std::cerr << ',' << ((runends[i / 64] >> (i % 64)) & 1ULL) << ',';
            for (int32_t j = s.infix_size_ - 1; j >= 0; j--)
                std::cerr << ((value >> (j % 64)) & 1ULL);
            std::cerr << "   ";
            if (cnt % 8 == 7)
                std::cerr << std::endl;
            cnt++;
        }
        std::cerr << std::endl;
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

    TEST_CASE("shrink infix size") {
        InfixStoreTests::ShrinkInfixSize();
    }

    TEST_CASE("resize") {
        InfixStoreTests::Resize();
    }
}

