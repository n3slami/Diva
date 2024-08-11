/**
 * @file art tests
 * @author ---
 *
 * @author Rafael Kallis <rk@rafaelkallis.com>
 */

#include <endian.h>
#include <limits>
#include <memory>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "steroids.hpp"
#include "util.hpp"

const std::string ansi_green = "\033[0;32m";
const std::string ansi_white = "\033[0;97m";

class SteroidsTests {
public:
    static void Insert() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);

        uint64_t value;
        uint8_t buf[9];
        memset(buf, 0, sizeof(buf));

        value = to_big_endian_order(0x0000000011111111UL);
        s.AddTreeKey(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        auto it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value));

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
            const std::vector<uint32_t> occupieds_pos = {2, 5, 8, 10, 13, 16,
                19, 21, 24, 27, 30, 32, 35, 38, 41, 43, 46, 49, 51, 54, 57, 60,
                62, 65, 68, 71, 73, 76, 79, 81, 84, 87, 90, 92, 95, 98, 101,
                103, 106, 109, 112, 114, 117, 120, 122, 125, 128, 131, 133,
                136, 139, 142, 144, 147, 150, 152, 155, 158, 161, 163, 166,
                169, 172, 174, 177, 180, 183, 185, 188, 191, 193, 196, 199,
                202, 204, 207, 210, 213, 215, 218, 221, 223, 226, 229, 232,
                234, 237, 239, 240, 243, 245, 248, 251, 254, 256, 259, 262,
                264, 267, 270};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
               {{3,1,0b11001}, {9,1,0b10001}, {15,1,0b01001}, {19,1,0b11111},
                   {25,1,0b10111}, {31,1,0b01111}, {37,1,0b00101},
                   {41,1,0b11101}, {47,1,0b10101}, {53,1,0b01011},
                   {59,1,0b00011}, {62,1,0b11011}, {68,1,0b10011},
                   {74,1,0b01001}, {80,1,0b00001}, {84,1,0b11001},
                   {90,1,0b01111}, {96,1,0b00111}, {100,1,0b11111},
                   {106,1,0b10101}, {112,1,0b01101}, {118,1,0b00101},
                   {121,1,0b11011}, {127,1,0b10011}, {133,1,0b01011},
                   {139,1,0b00011}, {143,1,0b11001}, {149,1,0b10001},
                   {155,1,0b01001}, {159,1,0b11111}, {165,1,0b10111},
                   {171,1,0b01111}, {177,1,0b00101}, {180,1,0b11101},
                   {186,1,0b10101}, {192,1,0b01011}, {198,1,0b00011},
                   {202,1,0b11011}, {208,1,0b10011}, {214,1,0b01001},
                   {220,1,0b00001}, {224,1,0b11001}, {230,1,0b01111},
                   {236,1,0b00111}, {239,1,0b11111}, {245,1,0b10101},
                   {251,1,0b01101}, {257,1,0b00101}, {261,1,0b11011},
                   {267,1,0b10011}, {273,1,0b01011}, {279,1,0b00001},
                   {283,1,0b11001}, {289,1,0b10001}, {295,1,0b01001},
                   {298,1,0b11111}, {304,1,0b10111}, {310,1,0b01111},
                   {316,1,0b00101}, {320,1,0b11101}, {326,1,0b10101},
                   {332,1,0b01011}, {338,1,0b00011}, {342,1,0b11011},
                   {348,1,0b10001}, {354,1,0b01001}, {359,1,0b00001},
                   {363,1,0b11001}, {369,1,0b01111}, {375,1,0b00111},
                   {379,1,0b11111}, {385,1,0b10101}, {391,1,0b01101},
                   {397,1,0b00101}, {401,1,0b11011}, {407,1,0b10011},
                   {413,1,0b01011}, {418,1,0b00001}, {422,1,0b11001},
                   {428,1,0b10001}, {434,1,0b01001}, {438,1,0b11111},
                   {444,1,0b10111}, {450,1,0b01111}, {456,1,0b00101},
                   {460,1,0b11101}, {466,1,0b10101}, {470,1,0b00001},
                   {472,1,0b01011}, {477,1,0b00011}, {481,1,0b11011},
                   {487,1,0b10001}, {493,1,0b01001}, {499,1,0b00001},
                   {503,1,0b10111}, {509,1,0b01111}, {515,1,0b00111},
                   {519,1,0b11111}, {525,1,0b10101}, {531,1,0b01101}};
            AssertStoreContents(s, it.ref(), occupieds_pos, checks);
        }

        for (int32_t i = 90; i >= 70; i -= 2) {
            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * i + r * (100 - i)) / 100;
            value = to_big_endian_order(interp);
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        }
        SUBCASE("overlapping interpolated reversed inserts with a stride of 2") {
            const std::vector<uint32_t> occupieds_pos = {2, 5, 8, 10, 13, 16,
                19, 21, 24, 27, 30, 32, 35, 38, 41, 43, 46, 49, 51, 54, 57, 60,
                62, 65, 68, 71, 73, 76, 79, 81, 84, 87, 90, 92, 95, 98, 101,
                103, 106, 109, 112, 114, 117, 120, 122, 125, 128, 131, 133,
                136, 139, 142, 144, 147, 150, 152, 155, 158, 161, 163, 166,
                169, 172, 174, 177, 180, 183, 185, 188, 191, 193, 196, 199,
                202, 204, 207, 210, 213, 215, 218, 221, 223, 226, 229, 232,
                234, 237, 239, 240, 243, 245, 248, 251, 254, 256, 259, 262,
                264, 267, 270};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                  {{3,1,0b11001}, {9,1,0b10001}, {15,1,0b01001},
                      {19,1,0b11111}, {25,1,0b10111}, {31,1,0b01111},
                      {37,1,0b00101}, {41,1,0b11101}, {47,1,0b10101},
                      {53,0,0b01011}, {54,1,0b01011}, {59,1,0b00011},
                      {62,0,0b11011}, {63,1,0b11011}, {68,1,0b10011},
                      {74,0,0b01001}, {75,1,0b01001}, {80,1,0b00001},
                      {84,0,0b11001}, {85,1,0b11001}, {90,1,0b01111},
                      {96,0,0b00111}, {97,1,0b00111}, {100,1,0b11111},
                      {106,0,0b10101}, {107,1,0b10101}, {112,1,0b01101},
                      {118,0,0b00101}, {119,1,0b00101}, {121,1,0b11011},
                      {127,0,0b10011}, {128,1,0b10011}, {133,1,0b01011},
                      {139,0,0b00011}, {140,1,0b00011}, {143,1,0b11001},
                      {149,0,0b10001}, {150,1,0b10001}, {155,1,0b01001},
                      {159,0,0b11111}, {160,1,0b11111}, {165,1,0b10111},
                      {171,1,0b01111}, {177,1,0b00101}, {180,1,0b11101},
                      {186,1,0b10101}, {192,1,0b01011}, {198,1,0b00011},
                      {202,1,0b11011}, {208,1,0b10011}, {214,1,0b01001},
                      {220,1,0b00001}, {224,1,0b11001}, {230,1,0b01111},
                      {236,1,0b00111}, {239,1,0b11111}, {245,1,0b10101},
                      {251,1,0b01101}, {257,1,0b00101}, {261,1,0b11011},
                      {267,1,0b10011}, {273,1,0b01011}, {279,1,0b00001},
                      {283,1,0b11001}, {289,1,0b10001}, {295,1,0b01001},
                      {298,1,0b11111}, {304,1,0b10111}, {310,1,0b01111},
                      {316,1,0b00101}, {320,1,0b11101}, {326,1,0b10101},
                      {332,1,0b01011}, {338,1,0b00011}, {342,1,0b11011},
                      {348,1,0b10001}, {354,1,0b01001}, {359,1,0b00001},
                      {363,1,0b11001}, {369,1,0b01111}, {375,1,0b00111},
                      {379,1,0b11111}, {385,1,0b10101}, {391,1,0b01101},
                      {397,1,0b00101}, {401,1,0b11011}, {407,1,0b10011},
                      {413,1,0b01011}, {418,1,0b00001}, {422,1,0b11001},
                      {428,1,0b10001}, {434,1,0b01001}, {438,1,0b11111},
                      {444,1,0b10111}, {450,1,0b01111}, {456,1,0b00101},
                      {460,1,0b11101}, {466,1,0b10101}, {470,1,0b00001},
                      {472,1,0b01011}, {477,1,0b00011}, {481,1,0b11011},
                      {487,1,0b10001}, {493,1,0b01001}, {499,1,0b00001},
                      {503,1,0b10111}, {509,1,0b01111}, {515,1,0b00111},
                      {519,1,0b11111}, {525,1,0b10101}, {531,1,0b01101}};
            AssertStoreContents(s, it.ref(), occupieds_pos, checks);
        }

        const uint32_t shamt = 16;
        for (int32_t i = 1; i < 50; i++) {
            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
            value = to_big_endian_order(interp);
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        }
        SUBCASE("overlapping interpolated consecutive inserts") {
            const std::vector<uint32_t> occupieds_pos = {2, 5, 8, 10, 13, 16,
                19, 21, 24, 27, 30, 32, 35, 38, 41, 43, 46, 49, 51, 54, 57, 60,
                62, 65, 68, 71, 73, 76, 79, 81, 84, 87, 90, 92, 95, 98, 101,
                103, 106, 109, 112, 114, 117, 120, 122, 125, 128, 131, 133,
                136, 139, 142, 144, 147, 150, 152, 155, 158, 161, 163, 166,
                169, 172, 174, 177, 180, 183, 185, 188, 191, 192, 193, 194,
                196, 199, 202, 204, 207, 210, 213, 215, 218, 221, 223, 226,
                229, 232, 234, 237, 239, 240, 243, 245, 248, 251, 254, 256,
                259, 262, 264, 267, 270};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{3,1,0b11001}, {9,1,0b10001}, {15,1,0b01001}, {19,1,0b11111},
                     {25,1,0b10111}, {31,1,0b01111}, {37,1,0b00101},
                     {41,1,0b11101}, {47,1,0b10101}, {53,0,0b01011},
                     {54,1,0b01011}, {59,1,0b00011}, {62,0,0b11011},
                     {63,1,0b11011}, {68,1,0b10011}, {74,0,0b01001},
                     {75,1,0b01001}, {80,1,0b00001}, {84,0,0b11001},
                     {85,1,0b11001}, {90,1,0b01111}, {96,0,0b00111},
                     {97,1,0b00111}, {100,1,0b11111}, {106,0,0b10101},
                     {107,1,0b10101}, {112,1,0b01101}, {118,0,0b00101},
                     {119,1,0b00101}, {121,1,0b11011}, {127,0,0b10011},
                     {128,1,0b10011}, {133,1,0b01011}, {139,0,0b00011},
                     {140,1,0b00011}, {143,1,0b11001}, {149,0,0b10001},
                     {150,1,0b10001}, {155,1,0b01001}, {159,0,0b11111},
                     {160,1,0b11111}, {165,1,0b10111}, {171,1,0b01111},
                     {177,1,0b00101}, {180,1,0b11101}, {186,1,0b10101},
                     {192,1,0b01011}, {198,1,0b00011}, {202,1,0b11011},
                     {208,1,0b10011}, {214,1,0b01001}, {220,1,0b00001},
                     {224,1,0b11001}, {230,1,0b01111}, {236,1,0b00111},
                     {239,1,0b11111}, {245,1,0b10101}, {251,1,0b01101},
                     {257,1,0b00101}, {261,1,0b11011}, {267,1,0b10011},
                     {273,1,0b01011}, {279,1,0b00001}, {283,1,0b11001},
                     {289,1,0b10001}, {295,1,0b01001}, {298,1,0b11111},
                     {304,1,0b10111}, {310,1,0b01111}, {316,1,0b00101},
                     {320,1,0b11101}, {326,1,0b10101}, {332,1,0b01011},
                     {338,1,0b00011}, {342,1,0b11011}, {348,1,0b10001},
                     {354,1,0b01001}, {359,1,0b00001}, {363,1,0b11001},
                     {369,1,0b01111}, {375,0,0b00111}, {376,0,0b01001},
                     {377,0,0b01011}, {378,0,0b01101}, {379,0,0b01111},
                     {380,0,0b10001}, {381,0,0b10011}, {382,0,0b10101},
                     {383,0,0b10111}, {384,0,0b11001}, {385,0,0b11011},
                     {386,0,0b11101}, {387,1,0b11111}, {388,0,0b00001},
                     {389,0,0b00011}, {390,0,0b00101}, {391,0,0b00111},
                     {392,0,0b01001}, {393,0,0b01011}, {394,0,0b01101},
                     {395,0,0b01111}, {396,0,0b10001}, {397,0,0b10011},
                     {398,0,0b10101}, {399,0,0b10111}, {400,0,0b11001},
                     {401,0,0b11011}, {402,0,0b11101}, {403,1,0b11111},
                     {404,0,0b00001}, {405,0,0b00011}, {406,0,0b00101},
                     {407,0,0b00111}, {408,0,0b01001}, {409,0,0b01011},
                     {410,0,0b01101}, {411,0,0b01111}, {412,0,0b10001},
                     {413,0,0b10011}, {414,0,0b10101}, {415,0,0b10111},
                     {416,0,0b11001}, {417,0,0b11011}, {418,0,0b11101},
                     {419,0,0b11111}, {420,1,0b11111}, {421,0,0b00001},
                     {422,0,0b00011}, {423,0,0b00101}, {424,0,0b00111},
                     {425,1,0b01001}, {426,1,0b10101}, {427,1,0b01101},
                     {428,1,0b00101}, {429,1,0b11011}, {430,1,0b10011},
                     {431,1,0b01011}, {432,1,0b00001}, {433,1,0b11001},
                     {434,1,0b10001}, {435,1,0b01001}, {438,1,0b11111},
                     {444,1,0b10111}, {450,1,0b01111}, {456,1,0b00101},
                     {460,1,0b11101}, {466,1,0b10101}, {470,1,0b00001},
                     {472,1,0b01011}, {477,1,0b00011}, {481,1,0b11011},
                     {487,1,0b10001}, {493,1,0b01001}, {499,1,0b00001},
                     {503,1,0b10111}, {509,1,0b01111}, {515,1,0b00111},
                     {519,1,0b11111}, {525,1,0b10101}, {531,1,0b01101}};
            AssertStoreContents(s, it.ref(), occupieds_pos, checks);
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value = to_big_endian_order(value);
        s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)});

        value = to_big_endian_order(0x0000000011111111UL);
        it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        SUBCASE("split infix store: left half") {
            const std::vector<uint32_t> occupieds_pos = {5, 11, 16, 21, 27, 32,
                38, 43, 49, 54, 60, 65, 71, 76, 82, 87, 92, 98, 103, 109, 114,
                120, 125, 131, 136, 142, 147, 153, 158, 163, 169, 174, 180,
                185, 191, 196, 202, 207, 213, 218, 224, 229, 234, 240, 245,
                251, 256, 262, 267, 273, 278, 284, 289, 295, 300, 305, 311,
                316, 322, 327, 333, 338, 344, 349, 355, 360, 366, 371, 376,
                382, 383};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{7,1,0b10010}, {15,1,0b00010}, {22,1,0b10010},
                     {29,1,0b11110}, {37,1,0b01110}, {44,1,0b11110},
                     {53,1,0b01010}, {60,1,0b11010}, {68,1,0b01010},
                     {75,0,0b10110}, {76,1,0b10110}, {84,1,0b00110},
                     {91,0,0b10110}, {92,1,0b10110}, {99,1,0b00110},
                     {106,0,0b10010}, {107,1,0b10010}, {115,1,0b00010},
                     {122,0,0b10010}, {123,1,0b10010}, {129,1,0b11110},
                     {137,0,0b01110}, {138,1,0b01110}, {144,1,0b11110},
                     {152,0,0b01010}, {153,1,0b01010}, {159,1,0b11010},
                     {168,0,0b01010}, {169,1,0b01010}, {175,1,0b10110},
                     {183,0,0b00110}, {184,1,0b00110}, {190,1,0b10110},
                     {199,0,0b00110}, {200,1,0b00110}, {206,1,0b10010},
                     {214,0,0b00010}, {215,1,0b00010}, {221,1,0b10010},
                     {228,0,0b11110}, {229,1,0b11110}, {237,1,0b01110},
                     {244,1,0b11110}, {252,1,0b01010}, {259,1,0b11010},
                     {268,1,0b01010}, {275,1,0b10110}, {283,1,0b00110},
                     {290,1,0b10110}, {298,1,0b00110}, {305,1,0b10010},
                     {314,1,0b00010}, {321,1,0b10010}, {328,1,0b11110},
                     {336,1,0b01110}, {343,1,0b11110}, {352,1,0b01010},
                     {359,1,0b11010}, {367,1,0b01010}, {374,1,0b10110},
                     {383,1,0b00110}, {390,1,0b10110}, {398,1,0b00010},
                     {405,1,0b10010}, {414,1,0b00010}, {421,1,0b10010},
                     {428,1,0b11110}, {436,1,0b01110}, {443,1,0b11110},
                     {451,1,0b01010}, {458,1,0b11010}, {467,1,0b01010},
                     {474,1,0b10110}, {482,1,0b00110}, {489,1,0b10110},
                     {498,1,0b00010}, {505,1,0b10010}, {513,1,0b00010},
                     {520,1,0b10010}, {527,1,0b11110}, {530,0,0b01110},
                     {531,0,0b10010}, {532,0,0b10110}, {533,0,0b11010},
                     {534,1,0b11110}, {535,0,0b00010}, {536,0,0b00110},
                     {537,1,0b01010}};
            auto store = it.ref();
            REQUIRE_FALSE(store.IsPartialKey());
            REQUIRE_EQ(store.GetInvalidBits(), 0);
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value &= ~BITMASK(shamt);
        value = to_big_endian_order(value);
        it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value) - shamt / 8);
        SUBCASE("split infix store: right half") {
            const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3, 4, 5, 6,
                7, 8, 9, 10, 11, 20, 31, 42, 53, 64, 75, 86, 97, 108, 119, 129,
                140, 151, 162, 173, 184, 190, 195, 206, 217, 228, 239, 250,
                260, 271, 282, 293, 304, 315};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{0,1,0b11011}, {1,0,0b00100}, {2,0,0b01100}, {3,0,0b10100},
                     {4,1,0b11100}, {5,0,0b00100}, {6,0,0b01100},
                     {7,0,0b10100}, {8,1,0b11100}, {9,0,0b00100},
                     {10,0,0b01100}, {11,0,0b10100}, {12,1,0b11100},
                     {13,0,0b00100}, {14,0,0b01100}, {15,0,0b10100},
                     {16,1,0b11100}, {17,0,0b00100}, {18,0,0b01100},
                     {19,0,0b10100}, {20,1,0b11100}, {21,0,0b00100},
                     {22,0,0b01100}, {23,0,0b10100}, {24,1,0b11100},
                     {25,0,0b00100}, {26,0,0b01100}, {27,0,0b10100},
                     {28,1,0b11100}, {29,0,0b00100}, {30,0,0b01100},
                     {31,0,0b10100}, {32,1,0b11100}, {33,0,0b00100},
                     {34,0,0b01100}, {35,0,0b10100}, {36,0,0b11100},
                     {37,1,0b11100}, {38,0,0b00100}, {39,0,0b01100},
                     {40,0,0b10100}, {41,1,0b11100}, {42,1,0b00100},
                     {43,1,0b10100}, {51,1,0b10100}, {69,1,0b10100},
                     {87,1,0b01100}, {105,1,0b01100}, {123,1,0b01100},
                     {141,1,0b00100}, {159,1,0b00100}, {177,1,0b00100},
                     {196,1,0b00100}, {212,1,0b11100}, {230,1,0b11100},
                     {248,1,0b11100}, {266,1,0b10100}, {285,1,0b10100},
                     {303,1,0b10100}, {313,1,0b00100}, {321,1,0b01100},
                     {339,1,0b01100}, {357,1,0b01100}, {375,1,0b00100},
                     {393,1,0b00100}, {412,1,0b00100}, {428,1,0b11100},
                     {446,1,0b11100}, {464,1,0b11100}, {482,1,0b11100},
                     {501,1,0b10100}, {519,1,0b10100}};
            auto store = it.ref();
            REQUIRE(store.IsPartialKey());
            REQUIRE_EQ(store.GetInvalidBits(), 0);
            AssertStoreContents(s, store, occupieds_pos, checks);
        }

        // Split an extension of a partial boundary key
        const std::string old_boundary = it.key();
        uint32_t extended_key_len = it.get_key_len() + 1;
        uint8_t extended_key[extended_key_len];
        memcpy(extended_key, it.key().c_str(), extended_key_len);
        extended_key[extended_key_len - 1] = 1;
        s.InsertSplit({extended_key, extended_key_len});

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value &= ~BITMASK(shamt);
        value = to_big_endian_order(value);
        it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value) - shamt / 8);

        SUBCASE("split infix store using an extension of a partial boundary key") {
            const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3, 4, 5, 6,
                7, 8, 9, 10, 11, 20, 31, 42, 53, 64, 75, 86, 97, 108, 119, 129,
                140, 151, 162, 173, 184, 190, 195, 206, 217, 228, 239, 250,
                260, 271, 282, 293, 304, 315};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{0,0,0b11001}, {1,1,0b11011}, {2,0,0b00100}, {3,0,0b01100},
                     {4,0,0b10100}, {5,1,0b11100}, {6,0,0b00100},
                     {7,0,0b01100}, {8,0,0b10100}, {9,1,0b11100},
                     {10,0,0b00100}, {11,0,0b01100}, {12,0,0b10100},
                     {13,1,0b11100}, {14,0,0b00100}, {15,0,0b01100},
                     {16,0,0b10100}, {17,1,0b11100}, {18,0,0b00100},
                     {19,0,0b01100}, {20,0,0b10100}, {21,1,0b11100},
                     {22,0,0b00100}, {23,0,0b01100}, {24,0,0b10100},
                     {25,1,0b11100}, {26,0,0b00100}, {27,0,0b01100},
                     {28,0,0b10100}, {29,1,0b11100}, {30,0,0b00100},
                     {31,0,0b01100}, {32,0,0b10100}, {33,1,0b11100},
                     {34,0,0b00100}, {35,0,0b01100}, {36,0,0b10100},
                     {37,0,0b11100}, {38,1,0b11100}, {39,0,0b00100},
                     {40,0,0b01100}, {41,0,0b10100}, {42,1,0b11100},
                     {43,1,0b00100}, {44,1,0b10100}, {51,1,0b10100},
                     {69,1,0b10100}, {87,1,0b01100}, {105,1,0b01100},
                     {123,1,0b01100}, {141,1,0b00100}, {159,1,0b00100},
                     {177,1,0b00100}, {196,1,0b00100}, {212,1,0b11100},
                     {230,1,0b11100}, {248,1,0b11100}, {266,1,0b10100},
                     {285,1,0b10100}, {303,1,0b10100}, {313,1,0b00100},
                     {321,1,0b01100}, {339,1,0b01100}, {357,1,0b01100},
                     {375,1,0b00100}, {393,1,0b00100}, {412,1,0b00100},
                     {428,1,0b11100}, {446,1,0b11100}, {464,1,0b11100},
                     {482,1,0b11100}, {501,1,0b10100}, {519,1,0b10100}};
            auto store = it.ref();
            REQUIRE(store.IsPartialKey());
            REQUIRE_EQ(store.GetInvalidBits(), 0);
            AssertStoreContents(s, store, occupieds_pos, checks);

            REQUIRE_EQ(old_boundary.size(), it.get_key_len());
            REQUIRE_EQ(memcmp(old_boundary.c_str(), it.key().c_str(), old_boundary.size()), 0);
            ++it;
            const uint64_t expected_next_boundary_key = 0x0000000022222222ULL;
            uint64_t current_key = 0;
            memcpy(&current_key, it.key().c_str(), it.get_key_len());
            REQUIRE_EQ(__bswap_64(current_key), expected_next_boundary_key);
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
        value = to_big_endian_order(value);
        s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)});

        value = to_big_endian_order(0b0000000000000000000000000000000000011101000100110000000000000000UL);
        it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value) - 2);
        --it;
        SUBCASE("split infix store, create void infixes") {
            const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3, 4, 5, 6,
                7, 8, 9, 10, 11, 12, 13, 14, 15, 32, 33, 34, 35, 36, 37, 38,
                39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
                55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
                71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
                87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101,
                102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
                114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
                126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
                138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
                150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161,
                162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173,
                174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185,
                186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
                198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
                210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221,
                222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
                234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245,
                246, 247, 248, 249, 250, 251, 252, 253, 254, 255};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                 {{0,1,0b10000}, {2,1,0b10000}, {4,1,0b10000}, {6,1,0b10000},
                     {8,1,0b10000}, {10,1,0b10000}, {12,1,0b10000},
                     {14,1,0b10000}, {16,1,0b10000}, {18,1,0b10000},
                     {20,1,0b10000}, {23,1,0b10000}, {25,1,0b10000},
                     {27,1,0b10000}, {29,1,0b10000}, {31,1,0b10000},
                     {67,1,0b10000}, {69,1,0b10000}, {71,1,0b10000},
                     {73,1,0b10000}, {75,1,0b10000}, {77,1,0b10000},
                     {79,1,0b10000}, {81,1,0b10000}, {83,1,0b10000},
                     {85,1,0b10000}, {88,1,0b10000}, {90,1,0b10000},
                     {92,1,0b10000}, {94,1,0b10000}, {96,1,0b10000},
                     {98,1,0b10000}, {100,1,0b10000}, {102,1,0b10000},
                     {104,1,0b10000}, {106,1,0b10000}, {109,1,0b10000},
                     {111,1,0b10000}, {113,1,0b10000}, {115,1,0b10000},
                     {117,1,0b10000}, {119,1,0b10000}, {121,1,0b10000},
                     {123,1,0b10000}, {125,1,0b10000}, {127,1,0b10000},
                     {130,1,0b10000}, {132,1,0b10000}, {134,1,0b10000},
                     {136,1,0b10000}, {138,1,0b10000}, {140,1,0b10000},
                     {142,1,0b10000}, {144,1,0b10000}, {146,1,0b10000},
                     {148,1,0b10000}, {150,1,0b10000}, {153,1,0b10000},
                     {155,1,0b10000}, {157,1,0b10000}, {159,1,0b10000},
                     {161,1,0b10000}, {163,1,0b10000}, {165,1,0b10000},
                     {167,1,0b10000}, {169,1,0b10000}, {171,1,0b10000},
                     {174,1,0b10000}, {176,1,0b10000}, {178,1,0b10000},
                     {180,1,0b10000}, {182,1,0b10000}, {184,1,0b10000},
                     {186,1,0b10000}, {188,1,0b10000}, {190,1,0b10000},
                     {192,1,0b10000}, {195,1,0b10000}, {197,1,0b10000},
                     {199,1,0b10000}, {201,1,0b10000}, {203,1,0b10000},
                     {205,1,0b10000}, {207,1,0b10000}, {209,1,0b10000},
                     {211,1,0b10000}, {213,1,0b10000}, {215,1,0b10000},
                     {218,1,0b10000}, {220,1,0b10000}, {222,1,0b10000},
                     {224,1,0b10000}, {226,1,0b10000}, {228,1,0b10000},
                     {230,1,0b10000}, {232,1,0b10000}, {234,1,0b10000},
                     {236,1,0b10000}, {239,1,0b10000}, {241,1,0b10000},
                     {243,1,0b10000}, {245,1,0b10000}, {247,1,0b10000},
                     {249,1,0b10000}, {251,1,0b10000}, {253,1,0b10000},
                     {255,1,0b10000}, {257,1,0b10000}, {260,1,0b10000},
                     {262,1,0b10000}, {264,1,0b10000}, {266,1,0b10000},
                     {268,1,0b10000}, {270,1,0b10000}, {272,1,0b10000},
                     {274,1,0b10000}, {276,1,0b10000}, {278,1,0b10000},
                     {280,1,0b10000}, {283,1,0b10000}, {285,1,0b10000},
                     {287,1,0b10000}, {289,1,0b10000}, {291,1,0b10000},
                     {293,1,0b10000}, {295,1,0b10000}, {297,1,0b10000},
                     {299,1,0b10000}, {301,1,0b10000}, {304,1,0b10000},
                     {306,1,0b10000}, {308,1,0b10000}, {310,1,0b10000},
                     {312,1,0b10000}, {314,1,0b10000}, {316,1,0b10000},
                     {318,1,0b10000}, {320,1,0b10000}, {322,1,0b10000},
                     {325,1,0b10000}, {327,1,0b10000}, {329,1,0b10000},
                     {331,1,0b10000}, {333,1,0b10000}, {335,1,0b10000},
                     {337,1,0b10000}, {339,1,0b10000}, {341,1,0b10000},
                     {343,1,0b10000}, {346,1,0b10000}, {348,1,0b10000},
                     {350,1,0b10000}, {352,1,0b10000}, {354,1,0b10000},
                     {356,1,0b10000}, {358,1,0b10000}, {360,1,0b10000},
                     {362,1,0b10000}, {364,1,0b10000}, {366,1,0b10000},
                     {369,1,0b10000}, {371,1,0b10000}, {373,1,0b10000},
                     {375,1,0b10000}, {377,1,0b10000}, {379,1,0b10000},
                     {381,1,0b10000}, {383,1,0b10000}, {385,1,0b10000},
                     {387,1,0b10000}, {390,1,0b10000}, {392,1,0b10000},
                     {394,1,0b10000}, {396,1,0b10000}, {398,1,0b10000},
                     {400,1,0b10000}, {402,1,0b10000}, {404,1,0b10000},
                     {406,1,0b10000}, {408,1,0b10000}, {411,1,0b10000},
                     {413,1,0b10000}, {415,1,0b10000}, {417,1,0b10000},
                     {419,1,0b10000}, {421,1,0b10000}, {423,1,0b10000},
                     {425,1,0b10000}, {427,1,0b10000}, {429,1,0b10000},
                     {431,1,0b10000}, {434,1,0b10000}, {436,1,0b10000},
                     {438,1,0b10000}, {440,1,0b10000}, {442,1,0b10000},
                     {444,1,0b10000}, {446,1,0b10000}, {448,1,0b10000},
                     {450,1,0b10000}, {452,1,0b10000}, {455,1,0b10000},
                     {457,1,0b10000}, {459,1,0b10000}, {461,1,0b10000},
                     {463,1,0b10000}, {465,1,0b10000}, {467,1,0b10000},
                     {469,1,0b10000}, {471,1,0b10000}, {473,1,0b10000},
                     {476,1,0b10000}, {478,1,0b10000}, {480,1,0b10000},
                     {482,1,0b10000}, {484,1,0b10000}, {486,1,0b10000},
                     {488,1,0b10000}, {490,1,0b10000}, {492,1,0b10000},
                     {494,1,0b10000}, {496,1,0b10000}, {499,1,0b10000},
                     {501,1,0b10000}, {503,1,0b10000}, {505,1,0b10000},
                     {507,1,0b10000}, {509,1,0b10000}, {511,1,0b10000},
                     {513,1,0b10000}, {515,1,0b10000}, {517,1,0b10000},
                     {520,1,0b10000}, {522,1,0b10000}, {524,1,0b10000},
                     {526,1,0b10000}, {528,1,0b10000}, {530,1,0b10000},
                     {532,1,0b10000}, {534,1,0b10000}};
            auto store = it.ref();
            REQUIRE(store.IsPartialKey());
            REQUIRE_EQ(store.GetInvalidBits(), 0);
            AssertStoreContents(s, store, occupieds_pos, checks);

            REQUIRE_EQ(old_boundary.size(), it.get_key_len());
            REQUIRE_EQ(memcmp(old_boundary.c_str(), it.key().c_str(), old_boundary.size()), 0);
            ++it;
            REQUIRE_EQ(sizeof(value) - 2, it.get_key_len());
            REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), it.key().c_str(), sizeof(value) - 2), 0);
        }

        value = to_big_endian_order(0x0000000033333333UL);
        it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        uint32_t new_extended_key_len = it.get_key_len() + 3;
        uint8_t new_extended_key[extended_key_len];
        memcpy(new_extended_key, it.key().c_str(), it.get_key_len());
        memset(new_extended_key + it.get_key_len(), 0, new_extended_key_len - 1 - it.get_key_len());
        new_extended_key[new_extended_key_len - 1] = 1;
        s.InsertSplit({new_extended_key, new_extended_key_len});

        value = to_big_endian_order(0x0000000033333333UL);
        it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        SUBCASE("split infix store using an extension of a full boundary key: left half") {
            const std::vector<uint32_t> occupieds_pos = {};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
            auto store = it.ref();
            REQUIRE(!store.IsPartialKey());
            AssertStoreContents(s, store, occupieds_pos, checks);

            REQUIRE_EQ(sizeof(value), it.get_key_len());
            REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), it.key().c_str(), sizeof(value)), 0);
        }

        ++it;
        SUBCASE("split infix store using an extension of a full boundary key: right half") {
            const std::vector<uint32_t> occupieds_pos = {205};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{403,1,0b00001}};
            auto store = it.ref();
            REQUIRE(!store.IsPartialKey());
            AssertStoreContents(s, store, occupieds_pos, checks);

            REQUIRE_EQ(new_extended_key_len, it.get_key_len());
            REQUIRE_EQ(memcmp(new_extended_key, it.key().c_str(), new_extended_key_len), 0);
        }
    }


    static void PointQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

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
            keys.insert(0b0000000000000000000000000000000000011101000010110000000000000000UL);
            partial_keys.insert(0b0000000000000000000000000000000000011101000010111000000000000000UL);

            value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
            value = to_big_endian_order(value);
            s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
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


    static void RangeQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

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
            keys.insert(0b0000000000000000000000000000000000011101000010110000000000000000UL);
            partial_keys.insert(0b0000000000000000000000000000000000011101000010111000000000000000UL);

            value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
            value = to_big_endian_order(value);
            s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
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


    static void ShrinkInfixSize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids::base_implicit_size - s.infix_size_;

            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
            const uint64_t value = to_big_endian_order(interp);
            s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
            const uint64_t rev_value = __bswap_64(value);
            keys.insert(rev_value);
            partial_keys.insert((rev_value & (~BITMASK(bits_to_zero_out)) | (1ULL << bits_to_zero_out)));
        }
        
        SUBCASE("shrink by one") {
            s.ShrinkInfixSize(infix_size - 1);
            REQUIRE_EQ(s.infix_size_, infix_size - 1);

            auto it = s.tree_.begin();
            {
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
                auto store = it.ref();
                const uint64_t value = 0;
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), it.key().c_str(), sizeof(value)), 0);
                REQUIRE_FALSE(store.IsPartialKey());
                REQUIRE_EQ(store.GetInvalidBits(), 0);
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            ++it;
            {
                const std::vector<uint32_t> occupieds_pos = {2, 5, 8, 10, 13,
                    16, 19, 21, 24, 27, 30, 32, 35, 38, 41, 43, 46, 49, 51, 54,
                    57, 60, 62, 65, 68, 71, 73, 76, 79, 81, 84, 87, 90, 92, 95,
                    98, 101, 103, 106, 109, 112, 114, 117, 120, 122, 125, 128,
                    131, 133, 136, 139, 142, 144, 147, 150, 152, 155, 158, 161,
                    163, 166, 169, 172, 174, 177, 180, 183, 185, 188, 191, 192,
                    193, 194, 196, 199, 202, 204, 207, 210, 213, 215, 218, 221,
                    223, 226, 229, 232, 234, 237, 240, 243, 245, 248, 251, 254,
                    256, 259, 262, 264, 267, 270};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = 
                    {{3,1,0b1101}, {9,1,0b1001}, {15,1,0b0101}, {19,1,0b1111},
                        {25,1,0b1011}, {31,1,0b0111}, {37,1,0b0011},
                        {41,1,0b1111}, {47,1,0b1011}, {53,0,0b0101},
                        {54,1,0b0101}, {59,1,0b0001}, {62,0,0b1101},
                        {63,1,0b1101}, {68,1,0b1001}, {74,0,0b0101},
                        {75,1,0b0101}, {80,1,0b0001}, {84,0,0b1101},
                        {85,1,0b1101}, {90,1,0b0111}, {96,0,0b0011},
                        {97,1,0b0011}, {100,1,0b1111}, {106,0,0b1011},
                        {107,1,0b1011}, {112,1,0b0111}, {118,0,0b0011},
                        {119,1,0b0011}, {121,1,0b1101}, {127,0,0b1001},
                        {128,1,0b1001}, {133,1,0b0101}, {139,0,0b0001},
                        {140,1,0b0001}, {143,1,0b1101}, {149,0,0b1001},
                        {150,1,0b1001}, {155,1,0b0101}, {159,0,0b1111},
                        {160,1,0b1111}, {165,1,0b1011}, {171,1,0b0111},
                        {177,1,0b0011}, {180,1,0b1111}, {186,1,0b1011},
                        {192,1,0b0101}, {198,1,0b0001}, {202,1,0b1101},
                        {208,1,0b1001}, {214,1,0b0101}, {220,1,0b0001},
                        {224,1,0b1101}, {230,1,0b0111}, {236,1,0b0011},
                        {239,1,0b1111}, {245,1,0b1011}, {251,1,0b0111},
                        {257,1,0b0011}, {261,1,0b1101}, {267,1,0b1001},
                        {273,1,0b0101}, {279,1,0b0001}, {283,1,0b1101},
                        {289,1,0b1001}, {295,1,0b0101}, {298,1,0b1111},
                        {304,1,0b1011}, {310,1,0b0111}, {316,1,0b0011},
                        {320,1,0b1111}, {326,1,0b1011}, {332,1,0b0101},
                        {338,1,0b0001}, {342,1,0b1101}, {348,1,0b1001},
                        {354,1,0b0101}, {359,1,0b0001}, {363,1,0b1101},
                        {369,1,0b0111}, {375,0,0b0011}, {376,0,0b0101},
                        {377,0,0b0101}, {378,0,0b0111}, {379,0,0b0111},
                        {380,0,0b1001}, {381,0,0b1001}, {382,0,0b1011},
                        {383,0,0b1011}, {384,0,0b1101}, {385,0,0b1101},
                        {386,0,0b1111}, {387,1,0b1111}, {388,0,0b0001},
                        {389,0,0b0001}, {390,0,0b0011}, {391,0,0b0011},
                        {392,0,0b0101}, {393,0,0b0101}, {394,0,0b0111},
                        {395,0,0b0111}, {396,0,0b1001}, {397,0,0b1001},
                        {398,0,0b1011}, {399,0,0b1011}, {400,0,0b1101},
                        {401,0,0b1101}, {402,0,0b1111}, {403,1,0b1111},
                        {404,0,0b0001}, {405,0,0b0001}, {406,0,0b0011},
                        {407,0,0b0011}, {408,0,0b0101}, {409,0,0b0101},
                        {410,0,0b0111}, {411,0,0b0111}, {412,0,0b1001},
                        {413,0,0b1001}, {414,0,0b1011}, {415,0,0b1011},
                        {416,0,0b1101}, {417,0,0b1101}, {418,0,0b1111},
                        {419,0,0b1111}, {420,1,0b1111}, {421,0,0b0001},
                        {422,0,0b0001}, {423,0,0b0011}, {424,0,0b0011},
                        {425,1,0b0101}, {426,1,0b1011}, {427,1,0b0111},
                        {428,1,0b0011}, {429,1,0b1101}, {430,1,0b1001},
                        {431,1,0b0101}, {432,1,0b0001}, {433,1,0b1101},
                        {434,1,0b1001}, {435,1,0b0101}, {438,1,0b1111},
                        {444,1,0b1011}, {450,1,0b0111}, {456,1,0b0011},
                        {460,1,0b1111}, {466,1,0b1011}, {472,1,0b0101},
                        {477,1,0b0001}, {481,1,0b1101}, {487,1,0b1001},
                        {493,1,0b0101}, {499,1,0b0001}, {503,1,0b1011},
                        {509,1,0b0111}, {515,1,0b0011}, {519,1,0b1111},
                        {525,1,0b1011}, {531,1,0b0111}};
                auto store = it.ref();
                const uint64_t value = to_big_endian_order(0b00010001000100010001000100010001UL);
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), it.key().c_str(), sizeof(value)), 0);
                REQUIRE_FALSE(store.IsPartialKey());
                REQUIRE_EQ(store.GetInvalidBits(), 0);
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }

        SUBCASE("shrink by two") {
            s.ShrinkInfixSize(infix_size - 2);
            REQUIRE_EQ(s.infix_size_, infix_size - 2);

            auto it = s.tree_.begin();
            {
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
                auto store = it.ref();
                const uint64_t value = 0;
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), it.key().c_str(), sizeof(value)), 0);
                REQUIRE_FALSE(store.IsPartialKey());
                REQUIRE_EQ(store.GetInvalidBits(), 0);
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
            ++it;
            {
                const std::vector<uint32_t> occupieds_pos = {2, 5, 8, 10, 13,
                    16, 19, 21, 24, 27, 30, 32, 35, 38, 41, 43, 46, 49, 51, 54,
                    57, 60, 62, 65, 68, 71, 73, 76, 79, 81, 84, 87, 90, 92, 95,
                    98, 101, 103, 106, 109, 112, 114, 117, 120, 122, 125, 128,
                    131, 133, 136, 139, 142, 144, 147, 150, 152, 155, 158, 161,
                    163, 166, 169, 172, 174, 177, 180, 183, 185, 188, 191, 192,
                    193, 194, 196, 199, 202, 204, 207, 210, 213, 215, 218, 221,
                    223, 226, 229, 232, 234, 237, 240, 243, 245, 248, 251, 254,
                    256, 259, 262, 264, 267, 270};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = 
                    {{3,1,0b111}, {9,1,0b101}, {15,1,0b011}, {19,1,0b111},
                        {25,1,0b101}, {31,1,0b011}, {37,1,0b001}, {41,1,0b111},
                        {47,1,0b101}, {53,0,0b011}, {54,1,0b011}, {59,1,0b001},
                        {62,0,0b111}, {63,1,0b111}, {68,1,0b101}, {74,0,0b011},
                        {75,1,0b011}, {80,1,0b001}, {84,0,0b111}, {85,1,0b111},
                        {90,1,0b011}, {96,0,0b001}, {97,1,0b001},
                        {100,1,0b111}, {106,0,0b101}, {107,1,0b101},
                        {112,1,0b011}, {118,0,0b001}, {119,1,0b001},
                        {121,1,0b111}, {127,0,0b101}, {128,1,0b101},
                        {133,1,0b011}, {139,0,0b001}, {140,1,0b001},
                        {143,1,0b111}, {149,0,0b101}, {150,1,0b101},
                        {155,1,0b011}, {159,0,0b111}, {160,1,0b111},
                        {165,1,0b101}, {171,1,0b011}, {177,1,0b001},
                        {180,1,0b111}, {186,1,0b101}, {192,1,0b011},
                        {198,1,0b001}, {202,1,0b111}, {208,1,0b101},
                        {214,1,0b011}, {220,1,0b001}, {224,1,0b111},
                        {230,1,0b011}, {236,1,0b001}, {239,1,0b111},
                        {245,1,0b101}, {251,1,0b011}, {257,1,0b001},
                        {261,1,0b111}, {267,1,0b101}, {273,1,0b011},
                        {279,1,0b001}, {283,1,0b111}, {289,1,0b101},
                        {295,1,0b011}, {298,1,0b111}, {304,1,0b101},
                        {310,1,0b011}, {316,1,0b001}, {320,1,0b111},
                        {326,1,0b101}, {332,1,0b011}, {338,1,0b001},
                        {342,1,0b111}, {348,1,0b101}, {354,1,0b011},
                        {359,1,0b001}, {363,1,0b111}, {369,1,0b011},
                        {375,0,0b001}, {376,0,0b011}, {377,0,0b011},
                        {378,0,0b011}, {379,0,0b011}, {380,0,0b101},
                        {381,0,0b101}, {382,0,0b101}, {383,0,0b101},
                        {384,0,0b111}, {385,0,0b111}, {386,0,0b111},
                        {387,1,0b111}, {388,0,0b001}, {389,0,0b001},
                        {390,0,0b001}, {391,0,0b001}, {392,0,0b011},
                        {393,0,0b011}, {394,0,0b011}, {395,0,0b011},
                        {396,0,0b101}, {397,0,0b101}, {398,0,0b101},
                        {399,0,0b101}, {400,0,0b111}, {401,0,0b111},
                        {402,0,0b111}, {403,1,0b111}, {404,0,0b001},
                        {405,0,0b001}, {406,0,0b001}, {407,0,0b001},
                        {408,0,0b011}, {409,0,0b011}, {410,0,0b011},
                        {411,0,0b011}, {412,0,0b101}, {413,0,0b101},
                        {414,0,0b101}, {415,0,0b101}, {416,0,0b111},
                        {417,0,0b111}, {418,0,0b111}, {419,0,0b111},
                        {420,1,0b111}, {421,0,0b001}, {422,0,0b001},
                        {423,0,0b001}, {424,0,0b001}, {425,1,0b011},
                        {426,1,0b101}, {427,1,0b011}, {428,1,0b001},
                        {429,1,0b111}, {430,1,0b101}, {431,1,0b011},
                        {432,1,0b001}, {433,1,0b111}, {434,1,0b101},
                        {435,1,0b011}, {438,1,0b111}, {444,1,0b101},
                        {450,1,0b011}, {456,1,0b001}, {460,1,0b111},
                        {466,1,0b101}, {472,1,0b011}, {477,1,0b001},
                        {481,1,0b111}, {487,1,0b101}, {493,1,0b011},
                        {499,1,0b001}, {503,1,0b101}, {509,1,0b011},
                        {515,1,0b001}, {519,1,0b111}, {525,1,0b101},
                        {531,1,0b011}};
                auto store = it.ref();
                const uint64_t value = to_big_endian_order(0b00010001000100010001000100010001UL);
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), it.key().c_str(), sizeof(value)), 0);
                REQUIRE_FALSE(store.IsPartialKey());
                REQUIRE_EQ(store.GetInvalidBits(), 0);
                AssertStoreContents(s, store, occupieds_pos, checks);
            }
        }
    }

private:
    static void AssertStoreContents(const Steroids& s, const Steroids::InfixStore& store,
                                    const std::vector<uint32_t>& occupieds_pos,
                                    const std::vector<std::tuple<uint32_t, bool, uint64_t>>& checks) {
        REQUIRE_NE(store.ptr, nullptr);
        REQUIRE_EQ(store.GetElemCount(), checks.size());
        const uint64_t *occupieds = store.ptr;
        const uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;
        uint32_t ind = 0;
        for (uint32_t i = 0; i < Steroids::infix_store_target_size; i++) {
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


    static void PrintStore(const Steroids& s, const Steroids::InfixStore& store) {
        const uint32_t size_grade = store.GetSizeGrade();
        const uint64_t *occupieds = store.ptr;
        const uint64_t *runends = store.ptr + Steroids::infix_store_target_size / 64;

        std::cerr << "is_partial=" << store.IsPartialKey() << " invalid_bits=" << store.GetInvalidBits();
        std::cerr << " size_grade=" << size_grade << " elem_count=" << store.GetElemCount();
        std::cerr << " --- ptr=" << store.ptr << std::endl;
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

TEST_SUITE("steroids") {
    TEST_CASE("insert") {
        SteroidsTests::Insert();
    }

    TEST_CASE("point query") {
        SteroidsTests::PointQuery();
    }

    TEST_CASE("range query") {
        SteroidsTests::RangeQuery();
    }

    TEST_CASE("shrink infix size") {
        SteroidsTests::ShrinkInfixSize();
    }
}

