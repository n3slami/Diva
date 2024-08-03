/**
 * @file art tests
 * @author ---
 *
 * @author Rafael Kallis <rk@rafaelkallis.com>
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

#include "steroids.hpp"
#include "util.hpp"

const char ansi_green[] = "\033[0;32m";
const char ansi_white[] = "\033[0;97m";

class InfixStoreTests {
public:
    static void TestAllocation() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const uint32_t total_words = (Steroids::infix_store_target_size + (s.infix_size_ + 1) * s.scaled_sizes_[0] + 63) / 64;
        for (int32_t i = 0; i < total_words; i++)
            REQUIRE_EQ(store.ptr[i], 0);
    }


    static void TestShiftingSlots() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

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


    static void TestShiftingRunends() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;
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

    static void TestInsertRaw() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore store(s.scaled_sizes_[0], s.infix_size_);
        uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;
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

    static void TestGetInfixList() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const uint64_t keys[21] = {0b00000000000001, 0b00000000000101, 0b00000000010101,
                                   0b00000000100001, 0b00000000100011, 0b00000000100101,
                                   0b01000000100001, 0b01000000100011, 0b01000000100101,
                                   0b01000000100110, 0b01000000100110, 0b01000000100110,
                                   0b01000001100001, 0b01000001100011, 0b01000001100101,
                                   0b01111111000001, 0b01111111000010, 0b01111111000010,
                                   0b01111111100001, 0b01111111100010, 0b01111111100010};
        for (int32_t i = 0; i < 21; i++)
            s.InsertRawIntoInfixStore(store, keys[i]);
        uint64_t res[22];
        const uint32_t len = s.GetInfixList(store, res);
        REQUIRE_EQ(len, 21);
        for (int32_t i = 0; i < 21; i++)
            REQUIRE_EQ(res[i], keys[i]);
    }

    static void TestLoadInfixList() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        const uint64_t keys[27] = {0b000000000000001, 0b000000000000101, 0b000000000010101,
                                   0b000000000100001, 0b000000000100011, 0b000000000100101,
                                   0b001000000100001, 0b001000000100011, 0b001000000100101,
                                   0b001000000100110, 0b001000000100110, 0b001000000100110,
                                   0b001000001100001, 0b001000001100011, 0b001000001100101,
                                   0b001111111000001, 0b001111111000010, 0b001111111000010,
                                   0b001111111100001, 0b001111111100010, 0b001111111100010,
                                   0b011111111000001, 0b011111111000010, 0b011111111000010,
                                   0b011111111100001, 0b011111111100010, 0b011111111100010};
        s.LoadListToInfixStore(store, keys, 27);
        uint64_t res[28];
        const uint32_t len = s.GetInfixList(store, res);
        REQUIRE_EQ(len, 27);
        for (int32_t i = 0; i < 27; i++)
            REQUIRE_EQ(res[i], keys[i]);
    }

private:
    static void PrintStore(const Steroids& s, const Steroids::InfixStore& store) {
        const uint32_t size_grade = store.GetSizeGrade();
        const uint64_t *occupieds = store.ptr;
        const uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;

        std::cerr << "is_partial=" << store.IsPartialKey() << " invalid_bits=" << store.GetInvalidBits();
        std::cerr << " size_grade=" << size_grade << " elem_count=" << store.GetElemCount() << std::endl;
        std::cerr << "occupieds: ";
        for (int32_t i = 0; i < Steroids::infix_store_target_size; i++) {
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
        InfixStoreTests::TestAllocation();
    }

    TEST_CASE("shifting slots") {
        InfixStoreTests::TestShiftingSlots();
    }

    TEST_CASE("shifting runends") {
        InfixStoreTests::TestShiftingRunends();
    }

    TEST_CASE("raw inserts") {
        InfixStoreTests::TestInsertRaw();
    }

    TEST_CASE("get infix list") {
        InfixStoreTests::TestGetInfixList();
    }

    TEST_CASE("load infix list") {
        InfixStoreTests::TestLoadInfixList();
    }
}

