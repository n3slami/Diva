#include <cstdint>
#include <iomanip>
#include <iostream>
#include <assert.h>
#include <string>

#include "steroids.hpp"
#include "util.hpp"

const std::string ansi_green = "\033[0;32m";
const std::string ansi_white = "\033[0;97m";

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
            assert(store.ptr[i] == 0);

        //PrintStore(s, store);
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
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));

        // ==== Right Shifting ====
        // Short Segment + Short Shift
        s.ShiftSlotsRight(store, 2, 4, 2);
        assert(s.GetSlot(store, 0) == 0);
        assert(s.GetSlot(store, 1) == 1);
        assert(s.GetSlot(store, 2) == 0);
        assert(s.GetSlot(store, 3) == 0);
        assert(s.GetSlot(store, 4) == 2);
        assert(s.GetSlot(store, 5) == 3);
        for (int32_t i = 6; i < total_slots; i++)
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));
        
        // Long Segment + Short Shift
        for (int32_t i = 0; i < total_slots; i++)
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
        s.ShiftSlotsRight(store, 1, 100, 2);
        assert(s.GetSlot(store, 0) == 0);
        assert(s.GetSlot(store, 1) == 0);
        assert(s.GetSlot(store, 2) == 0);
        for (int32_t i = 1; i < 100; i++)
            assert(s.GetSlot(store, 2 + i) == (i & BITMASK(s.infix_size_)));
        for (int32_t i = 102; i < total_slots; i++)
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));

        // Short Segment + Long Shift
        for (int32_t i = 0; i < total_slots; i++)
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
        s.ShiftSlotsRight(store, 3, 7, 150);
        assert(s.GetSlot(store, 0) == 0);
        assert(s.GetSlot(store, 1) == 1);
        assert(s.GetSlot(store, 2) == 2);
        for (int32_t i = 3; i < 153; i++)
            assert(s.GetSlot(store, i) == 0);
        assert(s.GetSlot(store, 153) == 3);
        assert(s.GetSlot(store, 154) == 4);
        assert(s.GetSlot(store, 155) == 5);
        assert(s.GetSlot(store, 156) == 6);
        for (int32_t i = 157; i < total_slots; i++)
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));

        // Long Segment + Long Shift
        for (int32_t i = 0; i < total_slots; i++)
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
        s.ShiftSlotsRight(store, 0, 110, 200);
        for (int32_t i = 0; i < 200; i++)
            assert(s.GetSlot(store, i) == 0);
        for (int32_t i = 0; i < 110; i++)
            assert(s.GetSlot(store, i + 200) == (i & BITMASK(s.infix_size_)));
        for (int32_t i = 310; i < total_slots; i++)
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));

        // ==== Left Shifting ====
        // Short Segment + Short Shift
        for (int32_t i = 0; i < total_slots; i++)
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
        s.ShiftSlotsLeft(store, total_slots - 10, total_slots, 2);
        assert(s.GetSlot(store, total_slots - 1) == 0);
        assert(s.GetSlot(store, total_slots - 2) == 0);
        for (int32_t i = total_slots - 10; i < total_slots; i++)
            assert(s.GetSlot(store, i - 2) == (i & BITMASK(s.infix_size_)));
        for (int32_t i = 0; i < total_slots - 12; i++)
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));

        // Short Segment + Long Shift
        for (int32_t i = 0; i < total_slots; i++)
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
        s.ShiftSlotsLeft(store, total_slots - 15, total_slots - 2, 210);
        assert(s.GetSlot(store, total_slots - 1) == ((total_slots - 1) & BITMASK(s.infix_size_)));
        assert(s.GetSlot(store, total_slots - 2) == ((total_slots - 2) & BITMASK(s.infix_size_)));
        for (int32_t i = total_slots - 212; i < total_slots - 2; i++)
            assert(s.GetSlot(store, i) == 0);
        for (int32_t i = total_slots - 15; i < total_slots - 2; i++)
            assert(s.GetSlot(store, i - 210) == (i & BITMASK(s.infix_size_)));
        for (int32_t i = 0; i < total_slots - 225; i++)
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));

        // Long Segment + Short Shift
        for (int32_t i = 0; i < total_slots; i++)
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
        s.ShiftSlotsLeft(store, total_slots - 250, total_slots - 1, 1);
        assert(s.GetSlot(store, total_slots - 1) == ((total_slots - 1) & BITMASK(s.infix_size_)));
        assert(s.GetSlot(store, total_slots - 2) == 0);
        for (int32_t i = total_slots - 250; i < total_slots - 1; i++)
            assert(s.GetSlot(store, i - 1) == (i & BITMASK(s.infix_size_)));
        for (int32_t i = 0; i < total_slots - 251; i++)
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));

        // Long Segment + Long Shift
        for (int32_t i = 0; i < total_slots; i++)
            s.SetSlot(store, i, i & BITMASK(s.infix_size_));
        s.ShiftSlotsLeft(store, total_slots - 100, total_slots - 1, 200);
        assert(s.GetSlot(store, total_slots - 1) == ((total_slots - 1) & BITMASK(s.infix_size_)));
        for (int32_t i = total_slots - 201; i < total_slots - 1; i++)
            assert(s.GetSlot(store, i) == 0);
        for (int32_t i = total_slots - 100; i < total_slots - 1; i++)
            assert(s.GetSlot(store, i - 200) == (i & BITMASK(s.infix_size_)));
        for (int32_t i = 0; i < total_slots - 300; i++)
            assert(s.GetSlot(store, i) == (i & BITMASK(s.infix_size_)));
    }


    static void TestShiftingRunends() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore store(s.scaled_sizes_[0], s.infix_size_);

        uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;
        // ==== Right Shifting ====
        // Short Segment + Short Shift
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;
        s.ShiftRunendsRight(store, 62, 65, 5);
        assert(runends[0] == 0b0000100010001000100010001000100010001000100010001000100010001000);
        assert(runends[1] == 0b0101010101010101010101010101010101010101010101010101010101110000);
        assert(runends[2] == 0b1010101010101010101010101010101010101010101010101010101010101010);
        assert(runends[3] == 0b1111111111111111111111111111111100000000000000000000000000000000);

        // Short Segment + Long Shift
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;
        s.ShiftRunendsRight(store, 60, 64, 125);
        assert(runends[0] == 0b0000100010001000100010001000100010001000100010001000100010001000);
        assert(runends[1] == 0b0000000000000000000000000000000000000000000000000000000000000000);
        assert(runends[2] == 0b1011000000000000000000000000000000000000000000000000000000000000);
        assert(runends[3] == 0b1111111111111111111111111111111100000000000000000000000000000000);

        // Long Segment + Short Shift
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;
        s.ShiftRunendsRight(store, 1, 123, 3);
        assert(runends[0] == 0b0100010001000100010001000100010001000100010001000100010001000000);
        assert(runends[1] == 0b0110101010101010101010101010101010101010101010101010101010101100);
        assert(runends[2] == 0b1010101010101010101010101010101010101010101010101010101010101010);
        assert(runends[3] == 0b1111111111111111111111111111111100000000000000000000000000000000);

        // Long Segment + Long Shift
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;
        s.ShiftRunendsRight(store, 2, 127, 100);
        assert(runends[0] == 0b0000000000000000000000000000000000000000000000000000000000000000);
        assert(runends[1] == 0b1000100010001000100010001000000000000000000000000000000000000000);
        assert(runends[2] == 0b0101010101010101010101010101100010001000100010001000100010001000);
        assert(runends[3] == 0b1111111111111111111111111111110101010101010101010101010101010101);

        // ==== Left Shifting ====
        // Short Segment + Short Shift
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;
        s.ShiftRunendsLeft(store, 190, 194, 4);
        assert(runends[0] == 0b1000100010001000100010001000100010001000100010001000100010001000);
        assert(runends[1] == 0b0101010101010101010101010101010101010101010101010101010101010101);
        assert(runends[2] == 0b0000101010101010101010101010101010101010101010101010101010101010);
        assert(runends[3] == 0b1111111111111111111111111111111100000000000000000000000000000000);

        // Short Segment + Long Shift
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;
        s.ShiftRunendsLeft(store, 190, 194, 130);
        assert(runends[0] == 0b0010100010001000100010001000100010001000100010001000100010001000);
        assert(runends[1] == 0b0000000000000000000000000000000000000000000000000000000000000000);
        assert(runends[2] == 0b0000000000000000000000000000000000000000000000000000000000000000);
        assert(runends[3] == 0b1111111111111111111111111111111100000000000000000000000000000000);

        // Long Segment + Short Shift
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;
        s.ShiftRunendsLeft(store, 120, 250, 3);
        assert(runends[0] == 0b1000100010001000100010001000100010001000100010001000100010001000);
        assert(runends[1] == 0b0100101010110101010101010101010101010101010101010101010101010101);
        assert(runends[2] == 0b0001010101010101010101010101010101010101010101010101010101010101);
        assert(runends[3] == 0b1111110001111111111111111111111111100000000000000000000000000000);

        // Long Segment + Long Shift
        runends[0] = 0b1000100010001000100010001000100010001000100010001000100010001000;
        runends[1] = 0b0101010101010101010101010101010101010101010101010101010101010101;
        runends[2] = 0b1010101010101010101010101010101010101010101010101010101010101010;
        runends[3] = 0b1111111111111111111111111111111100000000000000000000000000000000;
        s.ShiftRunendsLeft(store, 120, 250, 100);
        assert(runends[0] == 0b1010101010101010101010101010101010100101010110001000100010001000);
        assert(runends[1] == 0b1111000000000000000000000000000000001010101010101010101010101010);
        assert(runends[2] == 0b0000000000000000000000000000000000000000001111111111111111111111);
        assert(runends[3] == 0b1111110000000000000000000000000000000000000000000000000000000000);
    }

    static void TestInsertRaw() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);
        Steroids::InfixStore store(s.scaled_sizes_[0], s.infix_size_);
        uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;

        // Insertion and shifting of a single run
        assert(s.GetSlot(store, 269) == 0b00000);
        s.InsertRawIntoInfixStore(store, 0b010000000001011);
        assert(s.GetSlot(store, 269) == 0b01011);
        s.InsertRawIntoInfixStore(store, 0b010000000001100);
        s.InsertRawIntoInfixStore(store, 0b010000000001101);
        s.InsertRawIntoInfixStore(store, 0b010000000001110);
        assert(s.GetSlot(store, 269) == 0b01011);
        assert(s.GetSlot(store, 270) == 0b01100);
        assert(s.GetSlot(store, 271) == 0b01101);
        assert(s.GetSlot(store, 272) == 0b01110);
        for (int32_t i = 269; i < 272; i++)
            assert(get_bitmap_bit(runends, i) == 0);
        assert(get_bitmap_bit(runends, 272) == 1);

        // Insertion and shifting of a new run that shifts an old run
        assert(s.GetSlot(store, 268) == 0b00000);
        s.InsertRawIntoInfixStore(store, 0b001111111100001);
        assert(s.GetSlot(store, 268) == 0b00001);
        s.InsertRawIntoInfixStore(store, 0b001111111100010);
        s.InsertRawIntoInfixStore(store, 0b001111111100011);
        s.InsertRawIntoInfixStore(store, 0b001111111100100);
        for (int32_t i = 268; i < 271; i++) { 
            assert(s.GetSlot(store, i) == i - 267);
            assert(get_bitmap_bit(runends, i) == 0);
        }
        assert(s.GetSlot(store, 271) == 0b00100);
        assert(get_bitmap_bit(runends, 271) == 1);
        for (int32_t i = 272; i < 275; i++) { 
            assert(s.GetSlot(store, i) == 0b01011 + i - 272);
            assert(get_bitmap_bit(runends, i) == 0);
        }
        assert(s.GetSlot(store, 275) == 0b01110);
        assert(get_bitmap_bit(runends, 275) == 1);

        // Insertion and shifting at the very beginning of the array
        s.InsertRawIntoInfixStore(store, 0b000000000000001);
        s.InsertRawIntoInfixStore(store, 0b000000000000011);
        s.InsertRawIntoInfixStore(store, 0b000000000000010);
        s.InsertRawIntoInfixStore(store, 0b000000000000001);
        for (int32_t i = 0; i < 3; i++) {
            assert(s.GetSlot(store, i) == std::max(i, 1));
            assert(get_bitmap_bit(runends, i) == 0);
        }
        assert(s.GetSlot(store, 3) == 0b00011);
        assert(get_bitmap_bit(runends, 3) == 1);

        // Inserting new runs in between two touching runs
        assert(s.GetSlot(store, 4) == 0b00000);
        assert(get_bitmap_bit(runends, 4) == 0);
        assert(s.GetSlot(store, 5) == 0b00000);
        assert(get_bitmap_bit(runends, 5) == 0);

        s.InsertRawIntoInfixStore(store, 0b000000001111111);

        assert(s.GetSlot(store, 4) == 0b11111);
        assert(get_bitmap_bit(runends, 4) == 1);
        assert(s.GetSlot(store, 5) == 0b00000);
        assert(get_bitmap_bit(runends, 5) == 0);

        s.InsertRawIntoInfixStore(store, 0b000000000110101);

        assert(s.GetSlot(store, 4) == 0b10101);
        assert(get_bitmap_bit(runends, 4) == 1);
        assert(s.GetSlot(store, 5) == 0b11111);
        assert(get_bitmap_bit(runends, 5) == 1);

        // Insertion and shifting at the very end of the array
        s.InsertRawIntoInfixStore(store, 0b011111110100011);
        s.InsertRawIntoInfixStore(store, 0b011111110100001);
        s.InsertRawIntoInfixStore(store, 0b011111110100010);
        s.InsertRawIntoInfixStore(store, 0b011111110100111);
        s.InsertRawIntoInfixStore(store, 0b011111111100010);
        s.InsertRawIntoInfixStore(store, 0b011111111100001);
        s.InsertRawIntoInfixStore(store, 0b011111111100001);
        for (int32_t i = 531; i < 534; i++) {
            assert(s.GetSlot(store, i) == i - 530);
            assert(get_bitmap_bit(runends, i) == 0);
        }
        assert(s.GetSlot(store, 534) == 0b00111);
        assert(get_bitmap_bit(runends, 534) == 1);
        for (int32_t i = 535; i < 537; i++) {
            assert(s.GetSlot(store, i) == 0b00001);
            assert(get_bitmap_bit(runends, i) == 0);
        }
        assert(s.GetSlot(store, 537) == 0b00010);
        assert(get_bitmap_bit(runends, 537) == 1);

        // Insertion and shifting in between touching runs at the very end of the array
        s.InsertRawIntoInfixStore(store, 0b011111111011111);
        s.InsertRawIntoInfixStore(store, 0b011111111011101);
        s.InsertRawIntoInfixStore(store, 0b011111111011110);
        for (int32_t i = 528; i < 531; i++) {
            assert(s.GetSlot(store, i) == i - 527);
            assert(get_bitmap_bit(runends, i) == 0);
        }
        assert(s.GetSlot(store, 531) == 0b00111);
        assert(get_bitmap_bit(runends, 537) == 1);
        for (int32_t i = 532; i < 534; i++) {
            assert(s.GetSlot(store, i) == i - 532 + 0b11101);
            assert(get_bitmap_bit(runends, i) == 0);
        }
        assert(s.GetSlot(store, 534) == 0b11111);
        assert(get_bitmap_bit(runends, 534) == 1);
        for (int32_t i = 535; i < 537; i++) {
            assert(s.GetSlot(store, i) == 0b00001);
            assert(get_bitmap_bit(runends, i) == 0);
        }
        assert(s.GetSlot(store, 537) == 0b00010);
        assert(get_bitmap_bit(runends, 537) == 1);
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
        assert(len == 21);
        for (int32_t i = 0; i < 21; i++)
            assert(res[i] == keys[i]);
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
        PrintStore(s, store);
        uint64_t res[28];
        const uint32_t len = s.GetInfixList(store, res);
        assert(len == 27);
        for (int32_t i = 0; i < 27; i++)
            assert(res[i] == keys[i]);
    }

private:
    static void PrintStore(Steroids &s, Steroids::InfixStore &store) {
        const uint32_t size_grade = store.size_grade_elem_count >> s.infix_size_;
        const uint64_t *occupieds = store.ptr;
        const uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;

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

int main(int argc, char **argv) {
    std::cerr << ansi_green << "===== [ Running Tests ]" << ansi_white << std::endl;
    InfixStoreTests::TestAllocation();
    InfixStoreTests::TestShiftingSlots();
    InfixStoreTests::TestShiftingRunends();
    InfixStoreTests::TestInsertRaw();
    InfixStoreTests::TestGetInfixList();
    InfixStoreTests::TestLoadInfixList();
    std::cerr << ansi_green << "===== [ Done ]" << ansi_white << std::endl;

    return 0;
}

