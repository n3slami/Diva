/**
 * @file steroids tests
 * @author ---
 */

#include "wormhole/wh.h"
#include <endian.h>
#include <limits>
#include <random>
#include <x86intrin.h>
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
        Steroids<false> s(infix_size, seed, load_factor);

        uint64_t value;
        uint8_t buf[9];
        memset(buf, 0, sizeof(buf));

        value = to_big_endian_order(0x0000000011111111UL);
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
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            const uint64_t value = to_big_endian_order(0x0000000011111111UL);
            wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                             reinterpret_cast<void **>(&store), &dummy);
            AssertStoreContents(s, *store, occupieds_pos, checks);

            wh_iter_destroy(it);
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
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            const uint64_t value = to_big_endian_order(0x0000000011111111UL);
            wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                             reinterpret_cast<void **>(&store), &dummy);
            AssertStoreContents(s, *store, occupieds_pos, checks);

            wh_iter_destroy(it);
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
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            const uint64_t value = to_big_endian_order(0x0000000011111111UL);
            wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                             reinterpret_cast<void **>(&store), &dummy);
            AssertStoreContents(s, *store, occupieds_pos, checks);

            wh_iter_destroy(it);
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value = to_big_endian_order(value);
        s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)});

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
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            const uint64_t value = to_big_endian_order(0x0000000011111111UL);
            wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                             reinterpret_cast<void **>(&store), &dummy);
            REQUIRE_FALSE(store->IsPartialKey());
            REQUIRE_EQ(store->GetInvalidBits(), 0);
            AssertStoreContents(s, *store, occupieds_pos, checks);

            wh_iter_destroy(it);
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value &= ~BITMASK(shamt);
        value = to_big_endian_order(value);
        //it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value) - shamt / 8);
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
            uint8_t res_key[sizeof(uint64_t)];
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore store;

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);

            wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size, 
                             reinterpret_cast<void *>(&store), sizeof(Steroids<false>::InfixStore), &dummy);
            REQUIRE(store.IsPartialKey());
            REQUIRE_EQ(store.GetInvalidBits(), 0);
            AssertStoreContents(s, store, occupieds_pos, checks);

            wh_iter_destroy(it);
        }

        // Split an extension of a partial boundary key
        uint8_t old_boundary [sizeof(uint64_t)];
        uint32_t old_boundary_size;
        {
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);
            uint32_t dummy;
            Steroids<false>::InfixStore store;
            wh_iter_peek(it, reinterpret_cast<void *>(old_boundary), sizeof(old_boundary), &old_boundary_size, 
                             reinterpret_cast<void *>(&store), sizeof(Steroids<false>::InfixStore), &dummy);
            wh_iter_destroy(it);
        }
        uint32_t extended_key_len = old_boundary_size + 1;
        uint8_t extended_key[extended_key_len];
        memcpy(extended_key, old_boundary, extended_key_len);
        extended_key[extended_key_len - 1] = 1;
        s.InsertSplit({extended_key, extended_key_len});

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
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            uint64_t value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
            value &= ~BITMASK(shamt);
            value = to_big_endian_order(value);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
            REQUIRE(store->IsPartialKey());
            REQUIRE_EQ(store->GetInvalidBits(), 0);
            AssertStoreContents(s, *store, occupieds_pos, checks);

            REQUIRE_EQ(old_boundary_size, res_size);
            REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
            wh_iter_skip1(it);
            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
            const uint64_t expected_next_boundary_key = 0x0000000022222222ULL;
            uint64_t current_key = 0;
            memcpy(&current_key, res_key, res_size);
            REQUIRE_EQ(__bswap_64(current_key), expected_next_boundary_key);

            wh_iter_destroy(it);
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
        value = to_big_endian_order(value);
        s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)});

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
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            uint64_t value = to_big_endian_order(0b0000000000000000000000000000000000011101000100110000000000000000UL);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - 2);
            wh_iter_skip1_rev(it);

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
            REQUIRE(store->IsPartialKey());
            REQUIRE_EQ(store->GetInvalidBits(), 0);
            AssertStoreContents(s, *store, occupieds_pos, checks);

            REQUIRE_EQ(old_boundary_size, res_size);
            REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
            wh_iter_skip1(it);
            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
            REQUIRE_EQ(sizeof(value) - 2, res_size);
            REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value) - 2), 0);

            wh_iter_destroy(it);
        }

        uint8_t new_extended_key[12];
        uint32_t new_extended_key_len;
        {
            uint64_t value = to_big_endian_order(0x0000000033333333UL);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value));
            Steroids<false>::InfixStore *store;
            uint32_t dummy;
            wh_iter_peek(it, reinterpret_cast<void *>(new_extended_key), sizeof(new_extended_key), &new_extended_key_len, 
                             reinterpret_cast<void *>(&store), sizeof(Steroids<false>::InfixStore), &dummy);
            wh_iter_destroy(it);
        }
        memset(new_extended_key + new_extended_key_len, 0, 3);
        new_extended_key_len += 3;
        new_extended_key[new_extended_key_len - 1] = 1;
        s.InsertSplit({new_extended_key, new_extended_key_len});

        //it = s.tree_.begin(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        SUBCASE("split infix store using an extension of a full boundary key: left half") {
            const std::vector<uint32_t> occupieds_pos = {};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            const uint64_t value = to_big_endian_order(0x0000000033333333UL);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
            REQUIRE_FALSE(store->IsPartialKey());
            AssertStoreContents(s, *store, occupieds_pos, checks);

            REQUIRE_EQ(sizeof(value), res_size);
            REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);

            wh_iter_destroy(it);
        }

        SUBCASE("split infix store using an extension of a full boundary key: right half") {
            const std::vector<uint32_t> occupieds_pos = {205};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{403,1,0b00001}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            const uint64_t value = to_big_endian_order(0x0000000033333333UL);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
            wh_iter_skip1(it);

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
            REQUIRE_FALSE(store->IsPartialKey());
            AssertStoreContents(s, *store, occupieds_pos, checks);

            REQUIRE_EQ(new_extended_key_len, res_size);
            REQUIRE_EQ(memcmp(new_extended_key, res_key, new_extended_key_len), 0);

            wh_iter_destroy(it);
        }
    }


    static void RandomInsert() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;

        const uint32_t rng_seed = 10;
        std::mt19937 rng(rng_seed);
        const uint32_t init_n = 10000;
        std::vector<uint64_t> init_keys;
        for (int32_t i = 0; i < init_n; i++)
            init_keys.push_back(rng());
        std::sort(init_keys.begin(), init_keys.end());
        for (int32_t i = 0; i < init_n; i++)
            init_keys[i] = to_big_endian_order(init_keys[i]);
        Steroids<false> s(infix_size, init_keys.begin(), init_keys.end(), sizeof(uint64_t),
                   seed, load_factor);

        const uint32_t extra_n = 1400000;
        for (int32_t i = 0; i < extra_n; i++) {
            const uint64_t key = to_big_endian_order(rng());
            s.Insert(reinterpret_cast<const uint8_t *>(&key), sizeof(key));
        }

        /*
        wormhole_iter *it = wh_iter_create(s.better_tree_);
        wh_iter_seek(it, nullptr, 0);
        {
            const std::vector<uint32_t> occupieds_pos = {228};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                {{275,1,0b00001}};

            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);

            const uint32_t expected_size = 8;
            const uint8_t expected_key[expected_size] = {0b00000000,
                0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
                0b00000000, 0b00000000};
            REQUIRE_EQ(res_size, expected_size);
            REQUIRE_EQ(memcmp(res_key, expected_key, expected_size), 0);
            AssertStoreContents(s, *store, occupieds_pos, checks);
        }
        wh_iter_skip1(it);
        {
            const std::vector<uint32_t> occupieds_pos = {0, 2, 5, 6, 9, 10, 14,
                16, 18, 29, 30, 31, 32, 36, 42, 43, 45, 47, 50, 53, 54, 59, 60,
                62, 64, 65, 68, 70, 71, 74, 75, 76, 78, 79, 83, 85, 89, 90, 92,
                93, 97, 98, 99, 100, 103, 104, 105, 106, 108, 110, 111, 114,
                116, 117, 118, 120, 121, 122, 124, 125, 126, 127, 131, 132,
                134, 135, 140, 141, 143, 144, 145, 148, 149, 150, 151, 152,
                153, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 167,
                169, 170, 171, 176, 177, 178, 180, 181, 183, 184, 185, 186,
                188, 190, 191, 192, 195, 196, 200, 202, 204, 207, 209, 213,
                215, 216, 219, 222, 225, 232, 236, 237, 238, 240, 242, 245,
                246, 252, 254, 256, 257, 259};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                {{0,1,0b10100}, {4,1,0b01101}, {10,0,0b01001}, {11,0,0b11100},
                    {12,0,0b11100}, {13,1,0b11001}, {14,0,0b00111},
                    {15,1,0b11100}, {18,1,0b00100}, {20,1,0b11011},
                    {28,0,0b10011}, {29,1,0b11011}, {32,1,0b01011},
                    {36,1,0b11001}, {58,1,0b01100}, {60,0,0b00100},
                    {61,0,0b00111}, {62,1,0b01101}, {63,1,0b10100},
                    {64,1,0b11011}, {72,1,0b11100}, {85,1,0b00011},
                    {87,1,0b01011}, {91,1,0b01100}, {95,0,0b01001},
                    {96,1,0b10101}, {101,0,0b01100}, {102,1,0b01011},
                    {107,1,0b10100}, {109,1,0b10100}, {119,1,0b00101},
                    {121,1,0b00100}, {125,1,0b01111}, {129,1,0b00011},
                    {131,1,0b10101}, {137,1,0b11100}, {141,1,0b10100},
                    {143,1,0b01100}, {149,0,0b00101}, {150,1,0b10100},
                    {151,1,0b11100}, {153,0,0b01101}, {154,1,0b10100},
                    {158,1,0b10001}, {160,1,0b00100}, {168,1,0b10100},
                    {172,0,0b00100}, {173,0,0b01100}, {174,1,0b10100},
                    {180,0,0b00001}, {181,1,0b10100}, {182,1,0b10100},
                    {186,1,0b10100}, {188,1,0b00100}, {196,0,0b00100},
                    {197,1,0b01111}, {198,1,0b11100}, {200,1,0b10111},
                    {202,1,0b10101}, {208,1,0b01100}, {210,0,0b11011},
                    {211,1,0b11101}, {212,0,0b00100}, {213,1,0b10100},
                    {214,0,0b00100}, {215,1,0b00100}, {218,0,0b00100},
                    {219,1,0b01100}, {222,1,0b11100}, {224,1,0b01101},
                    {230,0,0b01111}, {231,1,0b10100}, {235,1,0b00111},
                    {237,0,0b10100}, {238,1,0b10100}, {239,1,0b00100},
                    {243,1,0b10001}, {245,0,0b10001}, {246,1,0b10111},
                    {247,0,0b10011}, {248,1,0b11100}, {251,0,0b01100},
                    {252,1,0b10100}, {253,0,0b11100}, {254,1,0b11111},
                    {255,0,0b01100}, {256,1,0b01111}, {257,1,0b10100},
                    {265,1,0b11100}, {267,0,0b00101}, {268,0,0b01100},
                    {269,0,0b11100}, {270,1,0b11001}, {271,1,0b00111},
                    {273,0,0b01100}, {274,0,0b10100}, {275,1,0b11100},
                    {283,0,0b00111}, {284,1,0b01100}, {285,0,0b01100},
                    {286,0,0b01001}, {287,0,0b10011}, {288,1,0b11100},
                    {289,0,0b10101}, {290,1,0b10101}, {291,1,0b00100},
                    {293,1,0b01100}, {299,1,0b11111}, {301,1,0b11011},
                    {303,1,0b11101}, {305,1,0b01100}, {307,1,0b11100},
                    {309,0,0b00100}, {310,1,0b10100}, {316,1,0b10011},
                    {318,0,0b10100}, {319,1,0b11011}, {320,0,0b10111},
                    {321,1,0b11101}, {322,1,0b11100}, {324,1,0b00001},
                    {326,0,0b01001}, {327,1,0b11101}, {328,1,0b01100},
                    {330,1,0b11111}, {332,1,0b01011}, {334,0,0b10001},
                    {335,1,0b11100}, {338,1,0b00001}, {342,0,0b00100},
                    {343,1,0b01100}, {344,0,0b00100}, {345,1,0b01100},
                    {346,1,0b10100}, {356,1,0b00011}, {358,1,0b01100},
                    {360,1,0b00001}, {364,1,0b11100}, {366,0,0b01100},
                    {367,0,0b01100}, {368,1,0b10100}, {370,1,0b01111},
                    {372,1,0b01101}, {374,1,0b10101}, {376,1,0b11100},
                    {380,1,0b10011}, {384,1,0b01011}, {386,1,0b10100},
                    {389,1,0b11100}, {395,1,0b00011}, {397,1,0b01011},
                    {405,1,0b10100}, {409,1,0b10100}, {413,1,0b11100},
                    {419,1,0b11100}, {423,1,0b11100}, {431,1,0b00100},
                    {435,1,0b10101}, {437,1,0b11100}, {443,0,0b01011},
                    {444,1,0b11100}, {449,1,0b11100}, {455,1,0b01101},
                    {470,0,0b01100}, {471,1,0b01100}, {478,1,0b10100},
                    {480,1,0b00100}, {482,1,0b00100}, {486,1,0b00100},
                    {490,1,0b10100}, {496,0,0b00100}, {497,1,0b10100},
                    {498,0,0b10100}, {499,1,0b10100}, {510,0,0b11100},
                    {511,1,0b11100}, {514,1,0b01100}, {518,1,0b10100},
                    {520,1,0b10100}, {524,0,0b11100}, {525,1,0b11100}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);

            const uint32_t expected_size = 8;
            const uint8_t expected_key[expected_size] = {0b00000000,
                0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000110,
                0b11110111, 0b11000100};
            REQUIRE_EQ(res_size, expected_size);
            REQUIRE_EQ(memcmp(res_key, expected_key, expected_size), 0);
            AssertStoreContents(s, *store, occupieds_pos, checks);
        }
        wh_iter_skip1(it);
        {
            const std::vector<uint32_t> occupieds_pos = {1, 2, 3, 4, 5, 6, 7,
                8, 9, 10, 11, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 23, 25,
                26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
                42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
                74, 75, 76, 77, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
                91, 92, 93, 94, 95, 98, 99, 100, 101, 102, 103, 104, 105, 106,
                107, 108, 109, 110, 111, 112, 114, 115, 116, 117, 119, 120,
                121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
                133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 145,
                146, 147, 148, 149, 151, 152, 153, 154, 155, 156, 157, 159,
                160, 161, 162, 163, 164, 165, 166, 167, 168, 170, 171, 172,
                173, 174, 175, 176, 178, 179, 180, 181, 182, 183, 184, 185,
                186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 197, 198,
                199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
                211, 213, 214, 216, 217, 218, 219, 220, 221, 222, 225, 226,
                227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238,
                239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250,
                251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262,
                263, 264, 265, 266, 267, 268, 270, 271, 272, 273, 274, 275,
                276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 288,
                289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300,
                301, 302, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313,
                314, 315, 316, 317, 318, 319, 320, 321, 322, 324, 325, 326,
                327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 339,
                340, 341, 342, 343};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                {{2,1,0b11111}, {5,0,0b00111}, {6,0,0b01011}, {7,1,0b11101},
                    {8,0,0b01111}, {9,1,0b11101}, {10,0,0b01101},
                    {11,1,0b11001}, {13,0,0b00111}, {14,0,0b01111},
                    {15,0,0b01111}, {16,0,0b10011}, {17,1,0b10101},
                    {18,1,0b00111}, {19,0,0b01011}, {20,0,0b01111},
                    {21,1,0b11001}, {22,0,0b01111}, {23,1,0b10001},
                    {24,1,0b01101}, {27,0,0b00001}, {28,0,0b00001},
                    {29,0,0b00011}, {30,0,0b00111}, {31,0,0b10001},
                    {32,1,0b11011}, {33,0,0b01101}, {34,0,0b10111},
                    {35,0,0b10111}, {36,1,0b11101}, {37,0,0b01101},
                    {38,0,0b10001}, {39,1,0b11011}, {40,0,0b00001},
                    {41,0,0b10011}, {42,1,0b11011}, {43,0,0b11101},
                    {44,1,0b11111}, {45,0,0b00001}, {46,1,0b11111},
                    {47,1,0b01001}, {49,0,0b00001}, {50,0,0b00011},
                    {51,0,0b00101}, {52,0,0b01111}, {53,0,0b10001},
                    {54,0,0b10011}, {55,0,0b11011}, {56,1,0b11101},
                    {57,1,0b01001}, {58,1,0b11001}, {59,1,0b11011},
                    {60,0,0b00001}, {61,0,0b00111}, {62,1,0b11101},
                    {63,0,0b01101}, {64,0,0b11001}, {65,1,0b11011},
                    {68,1,0b10111}, {71,0,0b01011}, {72,0,0b01111},
                    {73,0,0b10011}, {74,0,0b10011}, {75,0,0b10101},
                    {76,0,0b10111}, {77,1,0b11101}, {78,1,0b00011},
                    {79,0,0b00001}, {80,0,0b01011}, {81,1,0b11101},
                    {82,0,0b10011}, {83,1,0b11111}, {84,0,0b00111},
                    {85,1,0b10011}, {86,0,0b01101}, {87,0,0b01101},
                    {88,1,0b11111}, {89,0,0b00011}, {90,0,0b00011},
                    {91,0,0b01101}, {92,1,0b10101}, {93,0,0b00001},
                    {94,0,0b01011}, {95,1,0b11101}, {96,0,0b01001},
                    {97,1,0b10001}, {98,0,0b00111}, {99,0,0b10001},
                    {100,0,0b10011}, {101,1,0b11011}, {102,0,0b00001},
                    {103,1,0b10101}, {104,1,0b00111}, {105,0,0b11011},
                    {106,1,0b11011}, {107,0,0b00101}, {108,1,0b10101},
                    {109,0,0b00001}, {110,0,0b01101}, {111,0,0b01111},
                    {112,1,0b10101}, {113,0,0b00111}, {114,0,0b10011},
                    {115,1,0b10011}, {116,0,0b01101}, {117,0,0b01111},
                    {118,1,0b11111}, {119,0,0b01001}, {120,0,0b01001},
                    {121,0,0b01011}, {122,0,0b01111}, {123,0,0b10101},
                    {124,0,0b11011}, {125,0,0b11011}, {126,1,0b11011},
                    {127,0,0b00001}, {128,0,0b00011}, {129,0,0b01011},
                    {130,0,0b01111}, {131,0,0b01111}, {132,1,0b10001},
                    {133,0,0b00111}, {134,0,0b01101}, {135,1,0b10101},
                    {136,0,0b00001}, {137,0,0b01011}, {138,0,0b11001},
                    {139,1,0b11011}, {140,0,0b00101}, {141,0,0b00111},
                    {142,1,0b10011}, {143,0,0b00001}, {144,0,0b00011},
                    {145,0,0b00101}, {146,0,0b01101}, {147,1,0b10111},
                    {148,1,0b00101}, {149,0,0b10101}, {150,0,0b10111},
                    {151,0,0b11011}, {152,1,0b11101}, {153,0,0b01011},
                    {154,0,0b10111}, {155,0,0b11001}, {156,1,0b11101},
                    {157,0,0b11101}, {158,1,0b11101}, {159,1,0b10011},
                    {160,0,0b10111}, {161,1,0b11101}, {162,1,0b01011},
                    {163,0,0b00101}, {164,0,0b10111}, {165,1,0b11101},
                    {166,0,0b00001}, {167,0,0b00111}, {168,0,0b01101},
                    {169,1,0b11101}, {170,0,0b00001}, {171,1,0b11001},
                    {172,0,0b00011}, {173,0,0b00011}, {174,1,0b01111},
                    {175,0,0b00101}, {176,0,0b10101}, {177,1,0b11101},
                    {178,0,0b00101}, {179,0,0b01011}, {180,0,0b11101},
                    {181,1,0b11111}, {182,0,0b00111}, {183,0,0b01101},
                    {184,1,0b11111}, {185,0,0b00001}, {186,1,0b11101},
                    {187,0,0b00101}, {188,1,0b10111}, {189,1,0b11001},
                    {190,0,0b00011}, {191,1,0b11101}, {192,0,0b00001},
                    {193,0,0b00011}, {194,0,0b01011}, {195,0,0b01101},
                    {196,1,0b10011}, {197,0,0b11001}, {198,1,0b11001},
                    {199,1,0b10011}, {200,0,0b00101}, {201,1,0b11101},
                    {202,0,0b00001}, {203,0,0b01001}, {204,0,0b01001},
                    {205,0,0b01011}, {206,0,0b10001}, {207,1,0b11111},
                    {208,0,0b00001}, {209,0,0b10001}, {210,1,0b10111},
                    {211,0,0b11001}, {212,1,0b11101}, {213,1,0b01111},
                    {214,0,0b00011}, {215,1,0b01101}, {216,0,0b10111},
                    {217,1,0b11101}, {218,1,0b10111}, {219,1,0b01011},
                    {220,0,0b00011}, {221,1,0b10111}, {222,0,0b01001},
                    {223,0,0b01101}, {224,1,0b10111}, {225,0,0b01101},
                    {226,0,0b10101}, {227,1,0b11111}, {228,0,0b01101},
                    {229,0,0b11111}, {230,1,0b11111}, {231,1,0b10111},
                    {233,0,0b10101}, {234,1,0b11111}, {236,1,0b00111},
                    {238,0,0b00101}, {239,0,0b11001}, {240,1,0b11111},
                    {241,1,0b10111}, {244,0,0b00001}, {245,0,0b00101},
                    {246,1,0b00111}, {247,0,0b00111}, {248,0,0b01101},
                    {249,1,0b11001}, {250,0,0b00011}, {251,0,0b01111},
                    {252,0,0b10001}, {253,1,0b11011}, {254,0,0b10011},
                    {255,1,0b11111}, {256,1,0b00001}, {258,0,0b00111},
                    {259,0,0b00111}, {260,0,0b01111}, {261,0,0b10101},
                    {262,1,0b11001}, {263,0,0b00001}, {264,0,0b01101},
                    {265,1,0b10101}, {269,0,0b01001}, {270,0,0b01011},
                    {271,0,0b11011}, {272,1,0b11011}, {273,0,0b00111},
                    {274,0,0b01011}, {275,0,0b10011}, {276,0,0b10101},
                    {277,1,0b10111}, {278,0,0b01011}, {279,0,0b10001},
                    {280,1,0b10001}, {281,0,0b01001}, {282,0,0b10001},
                    {283,1,0b11101}, {284,0,0b00101}, {285,0,0b10101},
                    {286,0,0b10111}, {287,0,0b11011}, {288,1,0b11111},
                    {289,1,0b10011}, {290,0,0b00111}, {291,0,0b10011},
                    {292,0,0b10011}, {293,1,0b11011}, {294,1,0b11001},
                    {295,1,0b01101}, {296,1,0b00011}, {297,1,0b11011},
                    {299,0,0b00101}, {300,0,0b10101}, {301,1,0b11011},
                    {302,0,0b01101}, {303,1,0b01111}, {304,0,0b00111},
                    {305,1,0b11111}, {307,1,0b10011}, {313,0,0b10111},
                    {314,0,0b11111}, {315,1,0b11111}, {316,0,0b00111},
                    {317,0,0b01001}, {318,0,0b01011}, {319,0,0b10011},
                    {320,1,0b11101}, {321,1,0b10001}, {322,1,0b01111},
                    {326,0,0b00001}, {327,0,0b01111}, {328,0,0b10001},
                    {329,0,0b10011}, {330,0,0b11011}, {331,1,0b11101},
                    {332,0,0b00011}, {333,0,0b01111}, {334,1,0b10001},
                    {335,1,0b11001}, {336,0,0b10101}, {337,1,0b10111},
                    {338,0,0b10101}, {339,1,0b11111}, {340,1,0b11111},
                    {343,0,0b00001}, {344,0,0b00011}, {345,0,0b10101},
                    {346,1,0b11011}, {347,0,0b00011}, {348,0,0b00111},
                    {349,1,0b10101}, {350,0,0b00101}, {351,0,0b01011},
                    {352,0,0b01011}, {353,1,0b01111}, {354,0,0b01001},
                    {355,1,0b01011}, {356,0,0b00011}, {357,0,0b10001},
                    {358,0,0b10101}, {359,1,0b11111}, {360,1,0b01011},
                    {361,0,0b01011}, {362,1,0b10001}, {363,1,0b01111},
                    {365,0,0b00001}, {366,0,0b00011}, {367,0,0b00101},
                    {368,0,0b00101}, {369,0,0b01001}, {370,0,0b01011},
                    {371,1,0b10011}, {372,0,0b11001}, {373,1,0b11111},
                    {374,0,0b01001}, {375,0,0b10101}, {376,0,0b11111},
                    {377,1,0b11111}, {378,1,0b11101}, {379,0,0b01001},
                    {380,0,0b01011}, {381,0,0b01011}, {382,1,0b10011},
                    {383,0,0b00111}, {384,0,0b10111}, {385,1,0b11001},
                    {386,0,0b00111}, {387,1,0b11111}, {388,0,0b00101},
                    {389,0,0b00101}, {390,0,0b10011}, {391,1,0b10111},
                    {392,0,0b00001}, {393,0,0b00011}, {394,1,0b11011},
                    {395,0,0b00001}, {396,0,0b00011}, {397,0,0b10101},
                    {398,1,0b11001}, {399,0,0b10001}, {400,1,0b11001},
                    {401,0,0b01011}, {402,1,0b10001}, {403,0,0b10011},
                    {404,0,0b10101}, {405,1,0b11101}, {406,0,0b00101},
                    {407,0,0b00111}, {408,0,0b01011}, {409,1,0b11101},
                    {410,0,0b00001}, {411,0,0b00111}, {412,1,0b00111},
                    {413,0,0b00101}, {414,1,0b01101}, {415,0,0b01111},
                    {416,0,0b11011}, {417,1,0b11111}, {418,0,0b00001},
                    {419,0,0b11011}, {420,1,0b11101}, {421,1,0b10101},
                    {422,0,0b00111}, {423,0,0b01011}, {424,1,0b10011},
                    {425,1,0b01011}, {428,1,0b01111}, {431,0,0b00001},
                    {432,1,0b10101}, {436,0,0b00011}, {437,1,0b11011},
                    {439,1,0b01111}, {442,0,0b00111}, {443,0,0b10101},
                    {444,0,0b10101}, {445,1,0b11101}, {446,0,0b00011},
                    {447,0,0b01111}, {448,0,0b10011}, {449,0,0b11011},
                    {450,1,0b11101}, {451,1,0b10001}, {452,0,0b01111},
                    {453,0,0b11001}, {454,1,0b11011}, {455,0,0b00101},
                    {456,0,0b00111}, {457,0,0b01001}, {458,0,0b01011},
                    {459,1,0b11111}, {460,0,0b00011}, {461,1,0b01011},
                    {462,0,0b01011}, {463,1,0b10111}, {464,1,0b11101},
                    {466,0,0b00101}, {467,0,0b01111}, {468,0,0b10011},
                    {469,0,0b11001}, {470,0,0b11011}, {471,0,0b11101},
                    {472,1,0b11101}, {473,0,0b10011}, {474,0,0b10111},
                    {475,1,0b11011}, {476,0,0b00001}, {477,0,0b00111},
                    {478,1,0b01111}, {479,0,0b01001}, {480,0,0b01001},
                    {481,0,0b10011}, {482,1,0b11111}, {483,1,0b00011},
                    {484,0,0b10111}, {485,1,0b11101}, {486,0,0b01011},
                    {487,0,0b01101}, {488,0,0b10101}, {489,1,0b10111},
                    {490,1,0b00111}, {491,0,0b01001}, {492,1,0b01001},
                    {494,0,0b00011}, {495,0,0b01001}, {496,1,0b10001},
                    {497,0,0b00111}, {498,0,0b00111}, {499,0,0b01011},
                    {500,0,0b01111}, {501,0,0b10001}, {502,0,0b10001},
                    {503,1,0b11101}, {504,0,0b01101}, {505,0,0b10011},
                    {506,0,0b10101}, {507,1,0b10111}, {508,0,0b01001},
                    {509,0,0b01101}, {510,1,0b11001}, {511,0,0b01011},
                    {512,0,0b01101}, {513,0,0b01111}, {514,1,0b10001},
                    {515,0,0b01001}, {516,0,0b10101}, {517,1,0b11101},
                    {518,1,0b11011}, {519,0,0b01111}, {520,0,0b10001},
                    {521,1,0b11101}, {522,0,0b00101}, {523,0,0b00111},
                    {524,1,0b01111}, {525,0,0b00111}, {526,0,0b10011},
                    {527,1,0b10101}, {528,1,0b01101}, {529,1,0b01111},
                    {530,0,0b01001}, {531,0,0b01111}, {532,0,0b10111},
                    {533,1,0b11111}, {534,0,0b00011}, {535,0,0b00011},
                    {536,0,0b10011}, {537,1,0b11011}, {538,0,0b00111},
                    {539,1,0b11011}, {540,0,0b00111}, {541,1,0b01001},
                    {542,0,0b00011}, {543,0,0b01101}, {544,1,0b10111},
                    {545,1,0b11101}, {546,0,0b10101}, {547,1,0b10111},
                    {549,1,0b00101}, {552,0,0b00001}, {553,0,0b10101},
                    {554,0,0b10101}, {555,1,0b10101}, {556,1,0b00101},
                    {557,0,0b00011}, {558,0,0b01011}, {559,0,0b01101},
                    {560,0,0b01111}, {561,1,0b10111}, {562,0,0b00001},
                    {563,0,0b01011}, {564,1,0b11101}, {565,0,0b00011},
                    {566,0,0b01011}, {567,0,0b01101}, {568,0,0b10011},
                    {569,1,0b10111}, {570,1,0b10011}, {571,0,0b11001},
                    {572,1,0b11111}, {573,0,0b10011}, {574,0,0b10011},
                    {575,1,0b11001}, {576,1,0b11011}, {577,0,0b00101},
                    {578,0,0b01011}, {579,0,0b10111}, {580,0,0b10111},
                    {581,1,0b11001}, {582,0,0b00011}, {583,0,0b01101},
                    {584,0,0b01101}, {585,0,0b01111}, {586,0,0b11101},
                    {587,1,0b11111}, {588,0,0b00101}, {589,0,0b01001},
                    {590,1,0b10011}, {591,0,0b01111}, {592,1,0b01111},
                    {593,1,0b11011}, {595,0,0b00001}, {596,1,0b10011},
                    {598,0,0b10001}, {599,0,0b11001}, {600,1,0b11111},
                    {601,0,0b00001}, {602,0,0b00011}, {603,0,0b01001},
                    {604,0,0b01101}, {605,0,0b01111}, {606,1,0b10111},
                    {607,0,0b01111}, {608,1,0b10101}, {609,1,0b01101},
                    {610,0,0b00101}, {611,0,0b01111}, {612,1,0b10001},
                    {617,0,0b00001}, {618,0,0b10111}, {619,1,0b11011},
                    {620,0,0b00011}, {621,1,0b11101}, {623,0,0b01001},
                    {624,1,0b10111}, {626,0,0b00101}, {627,1,0b10011},
                    {628,1,0b10001}, {631,0,0b00111}, {632,0,0b01111},
                    {633,1,0b11001}, {634,0,0b11001}, {635,0,0b11001},
                    {636,1,0b11011}, {637,0,0b01001}, {638,0,0b10011},
                    {639,1,0b10011}, {640,0,0b01111}, {641,0,0b10011},
                    {642,1,0b10101}, {643,0,0b01111}, {644,1,0b11111},
                    {645,0,0b01001}, {646,1,0b10101}, {648,0,0b10001},
                    {649,1,0b11101}, {650,0,0b00111}, {651,1,0b01111},
                    {653,0,0b00001}, {654,0,0b01011}, {655,0,0b01101},
                    {656,0,0b10111}, {657,1,0b11001}, {658,0,0b00101},
                    {659,1,0b10001}, {660,0,0b00101}, {661,0,0b10001},
                    {662,1,0b10101}, {663,1,0b10011}, {664,0,0b00001},
                    {665,0,0b00001}, {666,0,0b00011}, {667,0,0b10011},
                    {668,0,0b10111}, {669,1,0b11101}, {670,0,0b01001},
                    {671,0,0b01101}, {672,0,0b11011}, {673,1,0b11111},
                    {674,0,0b00011}, {675,0,0b00011}, {676,0,0b01101},
                    {677,1,0b11111}, {678,0,0b00011}, {679,0,0b01111},
                    {680,0,0b10101}, {681,0,0b11111}, {682,1,0b11111},
                    {683,0,0b00011}, {684,0,0b10001}, {685,0,0b10101},
                    {686,1,0b10111}, {687,0,0b00001}, {688,0,0b01111},
                    {689,1,0b11111}, {690,0,0b00011}, {691,0,0b00011},
                    {692,1,0b11011}, {693,0,0b01101}, {694,1,0b11011},
                    {695,0,0b10101}, {696,1,0b11011}, {697,0,0b00011},
                    {698,0,0b00101}, {699,0,0b01011}, {700,1,0b01101},
                    {701,0,0b10011}, {702,0,0b10101}, {703,1,0b11011},
                    {704,0,0b00101}, {705,0,0b01101}, {706,1,0b11111},
                    {707,0,0b00001}, {708,0,0b01001}, {709,1,0b11001},
                    {710,1,0b00101}, {711,1,0b11101}, {712,0,0b01001},
                    {713,0,0b10111}, {714,0,0b11001}, {715,1,0b11011},
                    {716,0,0b01001}, {717,1,0b10011}, {718,0,0b01001},
                    {719,1,0b01111}, {720,0,0b00111}, {721,0,0b01011},
                    {722,1,0b01111}, {723,1,0b01011}, {724,1,0b01011},
                    {725,0,0b00111}, {726,0,0b01001}, {727,0,0b10001},
                    {728,1,0b11111}, {729,0,0b00001}, {730,0,0b01001},
                    {731,0,0b01011}, {732,0,0b01011}, {733,1,0b11001},
                    {734,0,0b00001}, {735,0,0b11001}, {736,1,0b11101},
                    {737,0,0b01001}, {738,0,0b10001}, {739,1,0b11111},
                    {740,0,0b00001}, {741,0,0b01011}, {742,1,0b10001},
                    {743,1,0b11111}, {744,0,0b01101}, {745,1,0b11011},
                    {746,0,0b10011}, {747,0,0b10101}, {748,0,0b11001},
                    {749,1,0b11101}, {750,0,0b00001}, {751,0,0b01111},
                    {752,0,0b10001}, {753,0,0b10101}, {754,1,0b11011},
                    {755,0,0b01001}, {756,0,0b01011}, {757,0,0b01101},
                    {758,0,0b10111}, {759,1,0b11001}, {760,0,0b11001},
                    {761,0,0b11101}, {762,1,0b11111}, {763,0,0b01101},
                    {764,1,0b10001}, {765,0,0b00101}, {766,0,0b00111},
                    {767,0,0b01011}, {768,0,0b10011}, {769,1,0b11001},
                    {770,1,0b00101}, {771,0,0b00001}, {772,0,0b10011},
                    {773,1,0b10111}, {774,0,0b01111}, {775,1,0b10101},
                    {776,1,0b00101}, {777,0,0b01111}, {778,1,0b11101},
                    {779,1,0b00111}, {780,0,0b00011}, {781,0,0b01001},
                    {782,0,0b01101}, {783,0,0b10101}, {784,1,0b11111},
                    {785,1,0b00101}, {786,0,0b10111}, {787,1,0b10111},
                    {788,0,0b00001}, {789,1,0b11101}, {790,0,0b01111},
                    {791,0,0b11011}, {792,1,0b11111}, {793,0,0b00101},
                    {794,1,0b11001}, {796,1,0b10111}, {799,0,0b10101},
                    {800,1,0b11001}, {801,0,0b01011}, {802,0,0b10001},
                    {803,1,0b11111}, {804,0,0b00001}, {805,0,0b00001},
                    {806,0,0b01011}, {807,0,0b01101}, {808,0,0b10011},
                    {809,0,0b11111}, {810,1,0b11111}, {811,0,0b10111},
                    {812,0,0b11001}, {813,1,0b11101}, {814,0,0b00011},
                    {815,0,0b01011}, {816,0,0b10001}, {817,1,0b10011},
                    {818,0,0b00111}, {819,1,0b10001}, {820,0,0b01111},
                    {821,1,0b10011}, {822,0,0b00101}, {823,0,0b01001},
                    {824,0,0b11101}, {825,1,0b11111}, {826,1,0b00101},
                    {827,1,0b01111}, {828,1,0b01001}, {829,0,0b00001},
                    {830,0,0b10011}, {831,1,0b10111}, {834,0,0b01101},
                    {835,0,0b11111}, {836,1,0b11111}, {837,0,0b00011},
                    {838,1,0b01101}, {840,0,0b01001}, {841,1,0b10101},
                    {843,1,0b11111}, {845,0,0b00101}, {846,0,0b00111},
                    {847,0,0b00111}, {848,1,0b11111}, {849,0,0b00011},
                    {850,1,0b00111}, {851,0,0b01001}, {852,0,0b01001},
                    {853,1,0b10011}, {854,0,0b00001}, {855,1,0b01001},
                    {856,0,0b01001}, {857,0,0b01101}, {858,0,0b11011},
                    {859,1,0b11111}, {860,1,0b11001}, {862,1,0b10001},
                    {865,0,0b11011}, {866,1,0b11101}, {867,0,0b00001},
                    {868,1,0b10101}, {870,0,0b00101}, {871,0,0b01001},
                    {872,1,0b11011}, {873,0,0b11101}, {874,1,0b11101},
                    {876,0,0b01011}, {877,0,0b01011}, {878,0,0b01011},
                    {879,1,0b01111}, {880,1,0b11001}, {881,0,0b00001},
                    {882,0,0b00111}, {883,0,0b10101}, {884,1,0b10111},
                    {885,0,0b10011}, {886,1,0b10111}, {889,1,0b00101},
                    {892,0,0b01001}, {893,1,0b01111}, {895,0,0b01001},
                    {896,0,0b10001}, {897,1,0b11111}, {898,0,0b01111},
                    {899,1,0b11101}, {900,0,0b00001}, {901,1,0b10111},
                    {903,0,0b01001}, {904,0,0b01011}, {905,0,0b01111},
                    {906,0,0b11001}, {907,1,0b11011}, {908,1,0b01011},
                    {909,0,0b01011}, {910,0,0b01011}, {911,0,0b11001},
                    {912,0,0b11101}, {913,1,0b11101}, {914,0,0b00111},
                    {915,0,0b01011}, {916,1,0b10001}, {917,0,0b00111},
                    {918,1,0b11001}, {919,0,0b01101}, {920,0,0b01111},
                    {921,0,0b10011}, {922,0,0b10101}, {923,0,0b10101},
                    {924,0,0b11001}, {925,0,0b11001}, {926,1,0b11011},
                    {927,0,0b01001}, {928,0,0b01001}, {929,0,0b10011},
                    {930,1,0b10111}, {931,0,0b00001}, {932,1,0b10011},
                    {933,0,0b10111}, {934,0,0b10111}, {935,1,0b11011},
                    {936,0,0b01111}, {937,0,0b10011}, {938,1,0b11011},
                    {939,0,0b01001}, {940,0,0b10001}, {941,1,0b10011},
                    {942,1,0b11011}, {943,1,0b10011}, {944,0,0b01101},
                    {945,0,0b10001}, {946,1,0b10111}};

            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);

            const uint32_t expected_size = 8;
            const uint8_t expected_key[expected_size] = {0b00000000,
                0b00000000, 0b00000000, 0b00000000, 0b00000010, 0b00011001,
                0b01011100, 0b00100011};
            REQUIRE_EQ(res_size, expected_size);
            REQUIRE_EQ(memcmp(res_key, expected_key, expected_size), 0);
            AssertStoreContents(s, *store, occupieds_pos, checks);
        }
        {
            const std::vector<uint32_t> occupieds_pos = {228};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                {{275,1,0b00001}};

            const uint8_t *res_key;
            uint32_t res_size, dummy;
            Steroids<false>::InfixStore *store;

            wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
            print_key(res_key, res_size);
            PrintStore(s, *store);
        }
        wh_iter_destroy(it);
         */
    }


    static void PointQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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
        Steroids<false> s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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


    static void Delete() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);

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

                const uint8_t *key_ptr;
                uint32_t key_len;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                Steroids<false>::InfixStore *store;
                uint32_t dummy;

                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                uint64_t total_implicit = 0b1'11111111 - 0b0'00000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'01010101'01011,
                                                          0b0'11011011'00100,
                                                          0b0'11011011'00001,
                                                          0b1'01010101'01000,
                                                          0b1'01010101'01011,
                                                          0b1'11011011'00001};
                s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                wh_iter_skip1(it);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                std::vector<uint64_t> right_store_infixes {0b1'01010101'10000,
                                                           0b1'01010101'01011,
                                                           0b1'01111111'11001,
                                                           0b1'11111100'01000,
                                                           0b1'11111100'00111};
                s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                s.DeleteMerge(it);

                it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                {
                    const std::vector<uint32_t> occupieds_pos = {0, 1, 171,
                        192, 255};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                         {{0,0,0b11101}, {1,0,0b11111}, {2,1,0b11111},
                             {3,0,0b00001}, {4,0,0b00001}, {5,1,0b00011},
                             {358,0,0b11000}, {359,1,0b10101}, {402,1,0b11101},
                             {534,0,0b00100}, {535,1,0b00011}};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    Steroids<false>::InfixStore *store;

                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                            reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);

                    uint64_t value = to_big_endian_order(boundary_keys[0]);
                    REQUIRE_EQ(sizeof(value), res_size);
                    REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    wh_iter_skip1(it);
                    value = to_big_endian_order(boundary_keys[2]);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                            reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_EQ(sizeof(value), res_size);
                    REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                }
                wh_iter_destroy(it);
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

                const uint8_t *key_ptr;
                uint32_t key_len;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                Steroids<false>::InfixStore *store;
                uint32_t dummy;

                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                uint64_t total_implicit = 0b1'11111111 - 0b0'00000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'00000000'10100,
                                                          0b0'00000000'10101,
                                                          0b0'00000001'10111,
                                                          0b0'00000010'10000,
                                                          0b0'00000010'00001,
                                                          0b0'00000011'10111,
                                                          0b1'00000000'00001};
                s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                wh_iter_skip1(it);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                std::vector<uint64_t> right_store_infixes {0b0'10100000'11111,
                                                           0b0'11110101'01000,
                                                           0b0'11110101'00001,
                                                           0b1'01001011'01011};
                s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                s.DeleteMerge(it);

                it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                {
                    const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3,
                        256, 257, 258};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                         {{0,0,0b10100}, {1,1,0b10101}, {2,1,0b10111},
                             {4,0,0b10000}, {5,1,0b00001}, {6,1,0b10111},
                             {530,1,0b00001}, {532,0,0b01011}, {533,0,0b11111},
                             {534,1,0b11111}, {535,1,0b10101}};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    Steroids<false>::InfixStore *store;

                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                            reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);

                    uint64_t value = to_big_endian_order(boundary_keys[0]);
                    REQUIRE_EQ(sizeof(value), res_size);
                    REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    wh_iter_skip1(it);
                    value = to_big_endian_order(boundary_keys[2]);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                            reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_EQ(sizeof(value), res_size);
                    REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                }
                wh_iter_destroy(it);
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

                const uint8_t *key_ptr;
                uint32_t key_len;
                uint64_t value = to_big_endian_order(boundary_keys[0]);
                Steroids<false>::InfixStore *store;
                uint32_t dummy;

                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                uint64_t total_implicit = 0b1'11111111 - 0b0'00000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'00000000'10100,
                                                          0b0'00000000'10101,
                                                          0b0'00000001'10111,
                                                          0b0'00000010'10000,
                                                          0b0'00000010'00001,
                                                          0b0'00000011'10111,
                                                          0b1'00000000'00001};
                s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                wh_iter_skip1(it);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                std::vector<uint64_t> right_store_infixes {0b0'10100000'11111,
                                                           0b0'11110101'01000,
                                                           0b0'11110101'00001,
                                                           0b1'01001011'01011};
                s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                s.DeleteMerge(it);

                it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                        reinterpret_cast<void **>(&store), &dummy);
                {
                    const std::vector<uint32_t> occupieds_pos = {0, 160, 245,
                        331};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks =
                         {{0,0,0b00001}, {1,0,0b00001}, {2,0,0b00001},
                             {3,0,0b00001}, {4,0,0b00001}, {5,0,0b00001},
                             {6,1,0b00001}, {168,1,0b11111}, {257,0,0b01000},
                             {258,1,0b00001}, {348,1,0b01011}};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    Steroids<false>::InfixStore *store;

                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                            reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);

                    uint64_t value = to_big_endian_order(boundary_keys[0]);
                    REQUIRE_EQ(sizeof(value), res_size);
                    REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    wh_iter_skip1(it);
                    value = to_big_endian_order(boundary_keys[2]);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                            reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_EQ(sizeof(value), res_size);
                    REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                }
                wh_iter_destroy(it);
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
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * i + r * (100 - i)) / 100;
                const uint64_t value = to_big_endian_order(interp);
                s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
                keys.emplace_back(interp, bits_to_zero_out);
            }

            for (int32_t i = 90; i >= 70; i -= 2) {
                const uint32_t shared = 34;
                const uint32_t ignore = 1;
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
                const uint64_t value = to_big_endian_order(interp);
                s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
                keys.emplace_back(interp, bits_to_zero_out);
            }

            {
                uint64_t value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
                auto it = boundary_keys.upper_bound(value);
                const uint64_t next_key = to_big_endian_order(*it);
                --it;
                const uint64_t prev_key = to_big_endian_order(*it);
                auto [shared, ignore, implicit_size] = s.GetSharedIgnoreImplicitLengths(
                        {reinterpret_cast<const uint8_t *>(&prev_key), sizeof(prev_key)},
                        {reinterpret_cast<const uint8_t *>(&next_key), sizeof(next_key)});
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - implicit_size - s.infix_size_;

                value = to_big_endian_order(value);
                s.InsertSplit({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
                const uint64_t rev_value = __bswap_64(value);
                keys.emplace_back(rev_value, bits_to_zero_out);
                boundary_keys.insert(0b0000000000000000000000000000000000011101000010110000000000000000UL);
            }

            for (int32_t i = 0; i < keys.size(); i++) {
                const uint64_t query = to_big_endian_order(keys[i].first);
                REQUIRE(s.PointQuery(reinterpret_cast<const uint8_t *>(&query), sizeof(query)));
            }

            const uint32_t shuffle_seed = 10;
            std::mt19937 shuffle_gen(shuffle_seed);
            std::shuffle(keys.begin(), keys.end(), shuffle_gen);

            for (int32_t i = 0; i < keys.size(); i++) {
                if (boundary_keys.find(keys[i].first) != boundary_keys.end())
                    boundary_keys.erase(keys[i].first);
                else if (0b00011101000010110000000000000000UL == (keys[i].first & (~BITMASK(keys[i].second + 1)))) {
                    const uint64_t l = 0b00011101000010110000000000000000UL;
                    const uint64_t r = 0b00011101000010111111111111111111UL;
                    const bool found = std::find_if(keys.begin() + i + 1, keys.end(),
                                                    [&](std::pair<uint64_t, uint64_t> key) { return l <= key.first && key.first <= r; })
                                            != keys.end();
                    if (!found)
                        boundary_keys.erase(keys[i].first & (~BITMASK(keys[i].second + 1)));
                }
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
    }


    static void ShrinkInfixSize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Steroids<false> s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Steroids<false>::base_implicit_size - s.infix_size_;

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

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, nullptr, 0);
            {
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                const uint64_t value = 0;
                REQUIRE_EQ(res_size, sizeof(value));
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }
            wh_iter_skip1(it);
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

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                const uint64_t value = to_big_endian_order(0b00010001000100010001000100010001UL);
                REQUIRE_EQ(res_size, sizeof(value));
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }
            wh_iter_destroy(it);
        }

        SUBCASE("shrink by two") {
            s.ShrinkInfixSize(infix_size - 2);
            REQUIRE_EQ(s.infix_size_, infix_size - 2);

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, nullptr, 0);
            {
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                const uint64_t value = 0;
                REQUIRE_EQ(res_size, sizeof(value));
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }
            wh_iter_skip1(it);
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

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                const uint64_t value = to_big_endian_order(0b00010001000100010001000100010001UL);
                REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }
            wh_iter_destroy(it);
        }
    }


    static void BulkLoad() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t n_keys = 1300;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);
        SUBCASE("fixed length") {
            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++)
                keys.push_back(rng());
            std::sort(keys.begin(), keys.end());
            for (int32_t i = 0; i < n_keys; i++)
                keys[i] = to_big_endian_order(keys[i]);

            Steroids<false> s(infix_size, keys.begin(), keys.end(), sizeof(uint64_t), seed, load_factor);

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, nullptr, 0);
            {
                const uint8_t expected_boundary[] = {0b00000000, 0b00000000,
                    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
                    0b00000000};
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }

            wh_iter_skip1(it);
            {
                const uint8_t expected_boundary[] = {0b00000000, 0b00010000,
                    0b00001100, 0b00111101, 0b11110111, 0b11101011, 0b00011100,
                    0b10100010};
                const std::vector<uint32_t> occupieds_pos = {2, 3, 4, 5, 7, 10,
                    11, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                    27, 28, 30, 31, 32, 35, 36, 37, 39, 40, 41, 42, 43, 44, 47,
                    49, 50, 51, 53, 54, 55, 56, 58, 59, 60, 61, 62, 63, 65, 66,
                    67, 68, 69, 70, 72, 73, 74, 75, 78, 80, 82, 84, 85, 87, 88,
                    89, 90, 91, 92, 93, 94, 96, 98, 101, 102, 103, 105, 106,
                    107, 108, 110, 111, 112, 113, 114, 115, 117, 118, 119, 120,
                    121, 122, 123, 125, 126, 128, 129, 130, 131, 132, 135, 136,
                    137, 139, 140, 143, 144, 146, 148, 149, 150, 151, 152, 154,
                    155, 157, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
                    170, 171, 172, 173, 174, 177, 178, 179, 180, 181, 182, 183,
                    184, 185, 186, 187, 188, 189, 190, 192, 194, 195, 197, 198,
                    199, 201, 202, 204, 205, 207, 208, 209, 212, 214, 216, 217,
                    218, 219, 221, 222, 223, 224, 225, 226, 228, 229, 230, 232,
                    233, 234, 235, 236, 237, 238, 240, 242, 243, 244, 246, 247,
                    248, 249, 251, 254, 255, 256, 257, 258, 259, 260, 261, 262,
                    263, 264, 265, 266, 267, 268, 270, 271, 273, 274, 275, 276,
                    277, 278, 279, 281, 283, 285, 286, 287, 292, 293, 295, 297,
                    298, 299, 302, 303, 306, 307, 308, 309, 311, 312, 314, 318,
                    320, 321, 322, 323, 324, 326, 327, 328, 329, 330, 331, 332,
                    333, 334, 335, 336, 338, 339, 340, 341, 343, 344, 345, 346,
                    347, 348, 349, 351, 352, 353, 354, 355, 357, 359, 360, 361,
                    363, 365, 366, 368, 369, 372, 373, 375, 377, 379, 380, 381,
                    382, 384, 385, 386, 387, 388, 389, 390, 392, 393, 394, 395,
                    396, 399, 400, 401, 403, 405, 406};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = 
                    {{2,0,0b01011}, {3,1,0b10111}, {4,0,0b01011},
                        {5,1,0b01111}, {6,0,0b00001}, {7,1,0b00001},
                        {8,0,0b01011}, {9,0,0b01101}, {10,1,0b10001},
                        {11,0,0b00011}, {12,0,0b00101}, {13,0,0b01111},
                        {14,1,0b11111}, {15,1,0b01001}, {16,0,0b11101},
                        {17,1,0b11111}, {18,1,0b10011}, {19,1,0b11111},
                        {20,1,0b01111}, {21,0,0b00111}, {22,1,0b01111},
                        {23,0,0b10001}, {24,0,0b11101}, {25,1,0b11101},
                        {26,1,0b00101}, {27,0,0b00011}, {28,1,0b11001},
                        {29,0,0b00001}, {30,0,0b01101}, {31,1,0b10101},
                        {32,1,0b01011}, {33,0,0b00001}, {34,1,0b10011},
                        {35,1,0b01001}, {36,1,0b10001}, {37,0,0b10111},
                        {38,1,0b11111}, {39,1,0b00011}, {40,0,0b00101},
                        {41,0,0b01111}, {42,1,0b11111}, {43,0,0b01001},
                        {44,1,0b10011}, {45,0,0b00101}, {46,0,0b10101},
                        {47,1,0b11101}, {48,1,0b11011}, {49,0,0b00101},
                        {50,1,0b01011}, {51,1,0b01101}, {52,1,0b11101},
                        {53,0,0b00111}, {54,1,0b11011}, {55,1,0b01101},
                        {56,1,0b10101}, {57,0,0b10111}, {58,0,0b11001},
                        {59,1,0b11111}, {60,1,0b01101}, {61,0,0b00011},
                        {62,0,0b10101}, {63,0,0b10101}, {64,1,0b10111},
                        {65,1,0b11001}, {66,1,0b10111}, {67,0,0b00101},
                        {68,1,0b10011}, {69,1,0b10111}, {70,1,0b10101},
                        {71,1,0b10101}, {72,0,0b00001}, {73,0,0b01001},
                        {74,1,0b01101}, {75,1,0b10101}, {76,0,0b11001},
                        {77,1,0b11011}, {78,0,0b01001}, {79,1,0b10101},
                        {80,1,0b00101}, {81,0,0b01111}, {82,0,0b10011},
                        {83,1,0b10101}, {84,0,0b00011}, {85,1,0b11111},
                        {86,1,0b11101}, {87,0,0b01111}, {88,1,0b10011},
                        {89,1,0b11101}, {90,0,0b00001}, {91,0,0b01111},
                        {92,1,0b11111}, {93,1,0b01011}, {94,0,0b11011},
                        {95,1,0b11111}, {96,0,0b00111}, {97,1,0b11011},
                        {98,1,0b10101}, {99,1,0b01011}, {100,1,0b10011},
                        {101,1,0b10101}, {102,0,0b10001}, {103,0,0b10101},
                        {104,1,0b10101}, {105,1,0b10101}, {106,1,0b01111},
                        {108,0,0b00101}, {109,1,0b11111}, {110,1,0b00101},
                        {112,0,0b00101}, {113,1,0b01011}, {114,1,0b11111},
                        {116,0,0b00101}, {117,1,0b10111}, {118,1,0b10001},
                        {119,1,0b10011}, {120,0,0b00001}, {121,1,0b01101},
                        {122,0,0b00001}, {123,0,0b00101}, {124,0,0b01111},
                        {125,1,0b10101}, {126,0,0b00111}, {127,1,0b11101},
                        {128,1,0b10001}, {129,1,0b00011}, {130,1,0b11101},
                        {133,0,0b01101}, {134,1,0b01101}, {135,1,0b00001},
                        {136,1,0b01111}, {138,0,0b00011}, {139,1,0b00111},
                        {140,1,0b10111}, {141,1,0b00011}, {142,1,0b00101},
                        {145,1,0b10001}, {146,1,0b10101}, {147,0,0b10101},
                        {148,1,0b11001}, {149,1,0b00101}, {150,0,0b00101},
                        {151,0,0b00101}, {152,0,0b10101}, {153,1,0b10101},
                        {154,1,0b11001}, {155,0,0b01101}, {156,1,0b10001},
                        {157,0,0b10111}, {158,0,0b10111}, {159,1,0b11101},
                        {160,0,0b00111}, {161,1,0b10001}, {162,0,0b10001},
                        {163,1,0b11011}, {164,0,0b01111}, {165,1,0b11101},
                        {166,1,0b01001}, {167,1,0b00111}, {168,0,0b00101},
                        {169,0,0b01101}, {170,1,0b10001}, {171,1,0b11001},
                        {172,0,0b00101}, {173,1,0b01101}, {174,1,0b10011},
                        {175,1,0b11111}, {176,0,0b10111}, {177,1,0b11001},
                        {178,1,0b00111}, {179,0,0b01101}, {180,1,0b11111},
                        {181,1,0b00001}, {182,1,0b00011}, {183,0,0b00101},
                        {184,0,0b01001}, {185,0,0b01101}, {186,1,0b01101},
                        {187,0,0b10001}, {188,0,0b10011}, {189,1,0b11101},
                        {190,0,0b10101}, {191,1,0b11011}, {192,0,0b10011},
                        {193,1,0b10101}, {194,1,0b10101}, {195,0,0b00111},
                        {196,1,0b11101}, {197,0,0b00011}, {198,1,0b01001},
                        {199,0,0b10011}, {200,1,0b11101}, {201,0,0b00101},
                        {202,1,0b11101}, {203,1,0b10111}, {204,1,0b10001},
                        {205,1,0b11001}, {207,0,0b00101}, {208,1,0b10111},
                        {211,1,0b10001}, {212,1,0b01001}, {213,1,0b11111},
                        {215,0,0b10001}, {216,0,0b10001}, {217,0,0b10111},
                        {218,0,0b10111}, {219,1,0b11111}, {220,1,0b01111},
                        {221,1,0b11111}, {222,0,0b00011}, {223,1,0b00101},
                        {224,0,0b00011}, {225,1,0b10111}, {226,1,0b00001},
                        {227,1,0b00001}, {228,1,0b00011}, {229,0,0b01101},
                        {230,1,0b01111}, {231,0,0b01011}, {232,1,0b11111},
                        {233,1,0b00101}, {234,0,0b00001}, {235,0,0b01111},
                        {236,1,0b11001}, {237,0,0b00001}, {238,1,0b10111},
                        {239,0,0b10001}, {240,0,0b10011}, {241,1,0b11001},
                        {242,0,0b00001}, {243,0,0b01011}, {244,1,0b11011},
                        {245,1,0b00011}, {246,1,0b11001}, {247,1,0b01111},
                        {248,1,0b11001}, {249,0,0b10011}, {250,1,0b11001},
                        {251,1,0b10111}, {252,1,0b11111}, {253,0,0b00011},
                        {254,1,0b10101}, {255,1,0b01011}, {256,1,0b10011},
                        {257,0,0b11011}, {258,1,0b11101}, {259,0,0b00101},
                        {260,0,0b01111}, {261,0,0b10111}, {262,1,0b10111},
                        {263,0,0b00011}, {264,0,0b00011}, {265,1,0b10101},
                        {266,0,0b00011}, {267,0,0b01011}, {268,1,0b11001},
                        {269,0,0b10101}, {270,0,0b10101}, {271,0,0b10101},
                        {272,1,0b11101}, {273,0,0b00111}, {274,1,0b10001},
                        {275,1,0b11111}, {276,0,0b01001}, {277,0,0b10011},
                        {278,0,0b11011}, {279,1,0b11111}, {280,1,0b00111},
                        {281,0,0b00011}, {282,0,0b00101}, {283,1,0b10101},
                        {284,0,0b00111}, {285,1,0b10011}, {286,1,0b00011},
                        {287,0,0b01011}, {288,1,0b10111}, {289,1,0b11101},
                        {290,1,0b00101}, {291,0,0b00101}, {292,0,0b01001},
                        {293,0,0b01101}, {294,1,0b11101}, {295,0,0b01011},
                        {296,0,0b11001}, {297,1,0b11011}, {298,1,0b11011},
                        {299,1,0b11001}, {300,0,0b01101}, {301,1,0b11111},
                        {302,1,0b10101}, {303,0,0b00011}, {304,1,0b10001},
                        {305,0,0b10001}, {306,1,0b11101}, {307,1,0b11101},
                        {308,0,0b00111}, {309,1,0b01001}, {310,0,0b00111},
                        {311,1,0b01111}, {312,0,0b10011}, {313,1,0b11011},
                        {314,1,0b11101}, {315,1,0b10001}, {316,1,0b00001},
                        {317,0,0b00101}, {318,0,0b00101}, {319,1,0b10101},
                        {320,0,0b01011}, {321,1,0b10001}, {322,0,0b01011},
                        {323,1,0b01101}, {324,0,0b00011}, {325,1,0b00111},
                        {326,1,0b01101}, {327,1,0b00111}, {328,1,0b10101},
                        {329,0,0b01001}, {330,1,0b11101}, {331,1,0b00011},
                        {332,1,0b01101}, {333,1,0b01011}, {334,1,0b01001},
                        {335,1,0b01101}, {336,0,0b01001}, {337,1,0b11001},
                        {338,1,0b10001}, {339,1,0b11101}, {340,1,0b11111},
                        {341,1,0b10101}, {342,1,0b01011}, {343,0,0b11011},
                        {344,1,0b11111}, {345,1,0b11011}, {346,1,0b10101},
                        {347,1,0b10011}, {348,0,0b00011}, {349,0,0b01001},
                        {350,1,0b10001}, {351,0,0b01001}, {352,1,0b10101},
                        {353,1,0b00001}, {354,0,0b11001}, {355,1,0b11001},
                        {356,1,0b01001}, {357,1,0b11001}, {358,0,0b10101},
                        {359,1,0b11101}, {360,0,0b00001}, {361,1,0b00001},
                        {362,1,0b00101}, {363,1,0b00011}, {364,1,0b00001},
                        {365,0,0b00011}, {366,1,0b11001}, {367,1,0b00011},
                        {368,1,0b10111}, {369,1,0b11001}, {370,1,0b00011},
                        {371,1,0b01001}, {373,1,0b01101}, {376,1,0b00001},
                        {377,0,0b01001}, {378,0,0b01111}, {379,1,0b11111},
                        {380,1,0b00001}, {385,1,0b11101}, {387,0,0b00011},
                        {388,0,0b00101}, {389,0,0b01011}, {390,1,0b11111},
                        {391,0,0b01111}, {392,1,0b11011}, {393,1,0b00111},
                        {394,0,0b01011}, {395,1,0b10001}, {396,0,0b00011},
                        {397,0,0b01001}, {398,0,0b10111}, {399,1,0b11111},
                        {400,1,0b00001}, {401,1,0b10111}, {404,1,0b11101},
                        {405,1,0b01001}, {406,0,0b01001}, {407,1,0b11001},
                        {408,0,0b00101}, {409,0,0b10111}, {410,1,0b11001},
                        {411,1,0b10111}, {412,1,0b01011}, {414,0,0b00011},
                        {415,1,0b10011}, {418,0,0b01001}, {419,1,0b11101},
                        {420,0,0b00011}, {421,0,0b00011}, {422,0,0b11011},
                        {423,1,0b11101}, {424,0,0b10101}, {425,0,0b11011},
                        {426,1,0b11111}, {427,0,0b01101}, {428,1,0b01111},
                        {429,1,0b01011}, {430,0,0b00001}, {431,1,0b10111},
                        {432,1,0b11011}, {433,1,0b10101}, {434,1,0b11101},
                        {435,0,0b00001}, {436,0,0b01001}, {437,1,0b10101},
                        {438,1,0b00101}, {439,1,0b01101}, {440,0,0b01111},
                        {441,1,0b10011}, {442,1,0b00111}, {443,1,0b11011},
                        {444,1,0b01001}, {445,0,0b01011}, {446,0,0b10101},
                        {447,1,0b11001}, {448,0,0b01101}, {449,0,0b10011},
                        {450,0,0b10111}, {451,0,0b11101}, {452,1,0b11111},
                        {453,1,0b11101}, {454,1,0b01101}, {455,0,0b00101},
                        {456,0,0b10011}, {457,1,0b10011}, {458,1,0b11101},
                        {459,1,0b10101}, {460,1,0b00001}, {461,0,0b01001},
                        {462,1,0b10111}, {463,1,0b00111}, {464,0,0b10011},
                        {465,1,0b11001}, {466,1,0b00011}, {467,0,0b00101},
                        {468,1,0b11011}, {469,1,0b10001}, {470,1,0b01011},
                        {471,0,0b00111}, {472,0,0b11001}, {473,1,0b11111},
                        {474,0,0b01011}, {475,1,0b10111}, {476,1,0b11011},
                        {477,1,0b01001}, {478,0,0b00011}, {479,1,0b11011},
                        {480,1,0b01011}, {481,1,0b11101}, {482,1,0b00001},
                        {483,1,0b10011}, {484,0,0b00011}, {485,0,0b11101},
                        {486,1,0b11111}, {487,1,0b01011}, {488,0,0b01011},
                        {489,1,0b11111}, {490,1,0b11011}, {491,1,0b01101},
                        {492,1,0b01011}, {493,0,0b00101}, {494,0,0b00101},
                        {495,1,0b11001}, {496,1,0b10001}, {497,0,0b11011},
                        {498,1,0b11111}, {499,1,0b11101}, {500,0,0b00011},
                        {501,0,0b00101}, {502,0,0b01111}, {503,0,0b01111},
                        {504,0,0b10111}, {505,1,0b11001}, {506,0,0b00011},
                        {507,0,0b00101}, {508,1,0b11111}, {509,1,0b01001},
                        {510,0,0b00011}, {511,0,0b01011}, {512,1,0b10011},
                        {513,1,0b10101}, {514,1,0b10101}, {515,0,0b00001},
                        {516,1,0b11001}, {517,1,0b11111}, {518,0,0b11011},
                        {519,1,0b11101}, {520,1,0b11001}, {521,1,0b11001},
                        {522,0,0b01101}, {523,0,0b10001}, {524,1,0b11111},
                        {525,0,0b00111}, {526,0,0b10111}, {527,1,0b11011},
                        {528,1,0b00111}, {529,0,0b00101}, {530,1,0b11001},
                        {531,1,0b00101}, {532,0,0b10101}, {533,0,0b10111},
                        {534,1,0b11011}, {535,0,0b00011}, {536,0,0b01001},
                        {537,1,0b11101}};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }

            wh_iter_skip1(it);
            {
                const uint8_t expected_boundary[] = {0b01100101, 0b11000011,
                    0b01001000, 0b01111100, 0b10000001, 0b11001001, 0b11011000,
                    0b11010010};
                const std::vector<uint32_t> occupieds_pos = {0, 1, 3, 4, 5, 6,
                    7, 9, 10, 11, 12, 13, 14, 15, 17, 21, 22, 23, 26, 27, 28,
                    29, 30, 31, 32, 33, 36, 39, 40, 41, 43, 45, 46, 47, 49, 52,
                    53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 67, 68,
                    70, 71, 72, 75, 78, 80, 82, 83, 84, 85, 88, 89, 90, 91, 92,
                    93, 95, 96, 97, 98, 99, 100, 101, 102, 103, 105, 106, 107,
                    108, 111, 113, 114, 116, 117, 118, 121, 122, 123, 125, 126,
                    127, 128, 134, 135, 136, 137, 138, 140, 141, 142, 143, 144,
                    147, 148, 149, 150, 151, 154, 155, 156, 157, 158, 159, 160,
                    161, 162, 163, 164, 167, 169, 170, 171, 172, 174, 175, 176,
                    177, 179, 182, 183, 185, 186, 187, 188, 190, 191, 192, 193,
                    194, 195, 196, 197, 200, 201, 202, 203, 204, 206, 208, 209,
                    210, 211, 212, 213, 214, 215, 217, 221, 222, 223, 224, 225,
                    226, 227, 228, 230, 231, 232, 235, 236, 238, 239, 240, 241,
                    243, 244, 247, 248, 249, 251, 254, 256, 257, 258, 261, 262,
                    263, 264, 265, 269, 272, 273, 274, 275, 276, 277, 278, 280,
                    281, 282, 284, 286, 287, 289, 290, 291, 292, 294, 295, 296,
                    297, 298, 301, 302, 305, 306, 307, 308, 310, 312, 316, 319,
                    320, 321, 322, 323, 324, 326, 327, 328, 329, 330, 331, 332,
                    334, 335, 336, 337, 338, 340, 344, 345, 346, 347, 348, 350,
                    351, 352, 353, 356, 357, 358, 359, 360, 361, 362, 363, 364,
                    366, 367, 368, 369, 371, 372, 373, 374, 376, 377, 378, 379,
                    380, 382, 384, 385, 386, 387, 388, 391, 393, 394, 395, 396,
                    398, 399};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = 
                    {{0,1,0b10111}, {1,1,0b01101}, {4,1,0b10001},
                        {5,1,0b01001}, {6,0,0b00111}, {7,0,0b00111},
                        {8,1,0b01011}, {9,0,0b01011}, {10,1,0b01111},
                        {11,0,0b01001}, {12,1,0b10111}, {13,1,0b00111},
                        {14,0,0b00101}, {15,0,0b00111}, {16,1,0b10011},
                        {17,1,0b10011}, {18,0,0b00011}, {19,1,0b11111},
                        {20,0,0b01011}, {21,1,0b11111}, {22,0,0b01111},
                        {23,0,0b10101}, {24,1,0b11011}, {25,1,0b10011},
                        {26,0,0b00101}, {27,0,0b01011}, {28,0,0b10111},
                        {29,1,0b11011}, {30,0,0b00011}, {31,1,0b11001},
                        {32,0,0b01001}, {33,0,0b01001}, {34,1,0b10111},
                        {35,0,0b10011}, {36,1,0b10011}, {37,0,0b11011},
                        {38,1,0b11101}, {39,1,0b10011}, {40,0,0b01001},
                        {41,0,0b01011}, {42,1,0b11011}, {43,0,0b01011},
                        {44,0,0b10111}, {45,0,0b10111}, {46,1,0b11011},
                        {47,0,0b00011}, {48,0,0b00101}, {49,0,0b10101},
                        {50,1,0b11111}, {51,0,0b00101}, {52,1,0b11011},
                        {53,1,0b01101}, {54,0,0b00011}, {55,0,0b01111},
                        {56,1,0b10001}, {57,0,0b00001}, {58,0,0b00111},
                        {59,0,0b01001}, {60,0,0b01101}, {61,1,0b11011},
                        {62,0,0b01011}, {63,1,0b10101}, {64,1,0b10001},
                        {65,0,0b01111}, {66,1,0b11001}, {67,0,0b00111},
                        {68,0,0b10111}, {69,1,0b11101}, {70,1,0b00101},
                        {71,1,0b00111}, {72,0,0b00011}, {73,1,0b01101},
                        {74,0,0b00111}, {75,0,0b01111}, {76,1,0b11001},
                        {77,0,0b10001}, {78,0,0b10011}, {79,1,0b11011},
                        {80,1,0b00001}, {81,1,0b11101}, {82,0,0b01011},
                        {83,0,0b01111}, {84,1,0b10011}, {85,0,0b00111},
                        {86,0,0b01111}, {87,1,0b10111}, {88,1,0b00011},
                        {89,1,0b11001}, {90,1,0b11011}, {91,1,0b10111},
                        {92,1,0b11011}, {93,0,0b10101}, {94,1,0b11001},
                        {95,0,0b01101}, {96,1,0b11111}, {97,1,0b10101},
                        {98,1,0b11101}, {99,0,0b01111}, {100,1,0b10001},
                        {101,0,0b00101}, {102,0,0b10011}, {103,0,0b10101},
                        {104,1,0b11101}, {105,1,0b10111}, {106,0,0b00101},
                        {107,0,0b00111}, {108,1,0b11101}, {109,1,0b01011},
                        {110,1,0b11101}, {111,1,0b11011}, {112,1,0b01011},
                        {113,1,0b01001}, {114,0,0b00001}, {115,0,0b10101},
                        {116,1,0b11001}, {117,1,0b10001}, {118,0,0b01101},
                        {119,0,0b10011}, {120,1,0b11011}, {121,0,0b10111},
                        {122,1,0b11011}, {123,0,0b01111}, {124,0,0b10101},
                        {125,1,0b11011}, {126,0,0b00111}, {127,0,0b01001},
                        {128,0,0b10011}, {129,0,0b11001}, {130,1,0b11111},
                        {131,0,0b10101}, {132,1,0b11101}, {133,0,0b00101},
                        {134,0,0b10001}, {135,0,0b10011}, {136,1,0b11101},
                        {137,0,0b00001}, {138,1,0b11101}, {139,1,0b01101},
                        {140,1,0b11111}, {141,0,0b00101}, {142,1,0b00111},
                        {143,0,0b01011}, {144,0,0b10011}, {145,1,0b11011},
                        {146,0,0b00011}, {147,1,0b01101}, {148,1,0b00001},
                        {149,1,0b01001}, {150,1,0b10001}, {151,1,0b00111},
                        {152,0,0b01001}, {153,0,0b10111}, {154,1,0b11111},
                        {155,1,0b01101}, {156,0,0b00111}, {157,1,0b01011},
                        {158,0,0b00001}, {159,1,0b10111}, {160,0,0b00101},
                        {161,0,0b01101}, {162,1,0b10101}, {163,0,0b01101},
                        {164,1,0b10101}, {165,1,0b10011}, {166,1,0b01101},
                        {167,0,0b10101}, {168,1,0b11001}, {169,1,0b10111},
                        {170,1,0b01101}, {171,0,0b01111}, {172,1,0b10101},
                        {173,0,0b01011}, {174,0,0b01111}, {175,0,0b10011},
                        {176,1,0b11101}, {177,0,0b00011}, {178,1,0b11111},
                        {179,0,0b01011}, {180,1,0b10111}, {181,0,0b10001},
                        {182,1,0b11001}, {183,0,0b10101}, {184,1,0b10111},
                        {185,1,0b11011}, {186,1,0b10111}, {187,0,0b01001},
                        {188,0,0b01001}, {189,0,0b11011}, {190,1,0b11111},
                        {191,0,0b00111}, {192,1,0b11001}, {193,1,0b01001},
                        {194,1,0b01101}, {195,1,0b10111}, {196,0,0b00001},
                        {197,1,0b10001}, {198,1,0b11111}, {199,1,0b01101},
                        {200,0,0b01101}, {201,1,0b10011}, {202,1,0b11101},
                        {203,1,0b01111}, {204,1,0b00001}, {205,0,0b00111},
                        {206,0,0b01011}, {207,0,0b01111}, {208,1,0b11011},
                        {209,0,0b10111}, {210,1,0b11011}, {211,1,0b11111},
                        {212,1,0b00011}, {213,1,0b01111}, {214,0,0b00111},
                        {215,1,0b11001}, {216,0,0b01001}, {217,1,0b10001},
                        {218,1,0b10111}, {219,1,0b01001}, {220,0,0b00001},
                        {221,1,0b01101}, {222,1,0b00111}, {223,1,0b10011},
                        {224,1,0b10011}, {227,0,0b00011}, {228,1,0b10011},
                        {229,0,0b01111}, {230,0,0b11001}, {231,1,0b11111},
                        {232,0,0b00101}, {233,1,0b11001}, {234,1,0b10001},
                        {235,0,0b00001}, {236,0,0b00111}, {237,1,0b11001},
                        {238,0,0b10111}, {239,1,0b11011}, {240,1,0b10001},
                        {241,1,0b01001}, {242,0,0b01001}, {243,1,0b10011},
                        {244,1,0b00001}, {245,0,0b10011}, {246,1,0b10111},
                        {248,0,0b00001}, {249,1,0b11111}, {250,1,0b00101},
                        {251,0,0b01011}, {252,1,0b01111}, {253,1,0b10111},
                        {255,1,0b10001}, {256,1,0b00011}, {258,1,0b00011},
                        {259,1,0b00111}, {260,0,0b00101}, {261,0,0b01101},
                        {262,1,0b10111}, {263,0,0b10111}, {264,0,0b11001},
                        {265,1,0b11001}, {266,1,0b01111}, {267,1,0b11101},
                        {268,1,0b11001}, {270,0,0b01001}, {271,1,0b10101},
                        {272,0,0b10001}, {273,0,0b10111}, {274,1,0b11111},
                        {275,0,0b00011}, {276,1,0b00101}, {277,1,0b00101},
                        {278,1,0b11011}, {279,0,0b00001}, {280,1,0b10101},
                        {281,0,0b00001}, {282,0,0b01001}, {283,0,0b11111},
                        {284,1,0b11111}, {285,1,0b11111}, {286,1,0b01111},
                        {287,0,0b11111}, {288,1,0b11111}, {289,0,0b00101},
                        {290,1,0b11101}, {291,1,0b11101}, {292,1,0b00111},
                        {293,0,0b01101}, {294,1,0b10111}, {297,0,0b00001},
                        {298,1,0b10001}, {299,1,0b11111}, {300,1,0b00011},
                        {301,1,0b00101}, {302,1,0b01011}, {303,0,0b10111},
                        {304,1,0b10111}, {305,0,0b10001}, {306,0,0b10111},
                        {307,0,0b11001}, {308,1,0b11101}, {309,1,0b10111},
                        {310,0,0b00001}, {311,1,0b11001}, {312,1,0b10001},
                        {313,1,0b10101}, {315,1,0b01101}, {317,0,0b11001},
                        {318,1,0b11001}, {319,1,0b01111}, {321,0,0b01001},
                        {322,1,0b01101}, {323,0,0b00011}, {324,1,0b00111},
                        {325,1,0b11001}, {326,1,0b10001}, {327,1,0b01001},
                        {331,1,0b01101}, {333,1,0b01001}, {334,1,0b01111},
                        {337,0,0b10111}, {338,1,0b11011}, {341,1,0b00111},
                        {344,1,0b01101}, {345,0,0b00001}, {346,0,0b01001},
                        {347,0,0b01101}, {348,1,0b10101}, {349,1,0b00001},
                        {350,0,0b00011}, {351,1,0b11111}, {352,0,0b11001},
                        {353,1,0b11011}, {354,0,0b10011}, {355,0,0b10111},
                        {356,1,0b11001}, {357,0,0b10011}, {358,1,0b10111},
                        {359,0,0b01011}, {360,0,0b10011}, {361,1,0b11001},
                        {362,0,0b00111}, {363,1,0b11101}, {365,0,0b00101},
                        {366,1,0b01111}, {367,0,0b00101}, {368,0,0b10111},
                        {369,1,0b11101}, {370,1,0b10111}, {371,0,0b00011},
                        {372,0,0b01101}, {373,1,0b10101}, {374,1,0b01101},
                        {375,0,0b01001}, {376,1,0b11111}, {377,0,0b00011},
                        {378,1,0b00101}, {379,0,0b01111}, {380,1,0b11011},
                        {381,0,0b00001}, {382,0,0b01101}, {383,1,0b11101},
                        {384,0,0b00011}, {385,1,0b10101}, {386,0,0b00101},
                        {387,1,0b11101}, {388,0,0b01101}, {389,1,0b11111},
                        {390,0,0b01111}, {391,0,0b10011}, {392,0,0b10101},
                        {393,1,0b11001}, {394,0,0b01011}, {395,1,0b10011},
                        {396,0,0b00101}, {397,0,0b10001}, {398,1,0b11001},
                        {399,1,0b10101}, {400,1,0b01011}, {401,0,0b01001},
                        {402,1,0b10011}, {403,0,0b01001}, {404,1,0b11011},
                        {405,0,0b00001}, {406,0,0b00101}, {407,0,0b01101},
                        {408,0,0b10011}, {409,1,0b11001}, {410,0,0b00111},
                        {411,1,0b01101}, {412,0,0b01001}, {413,1,0b11101},
                        {414,1,0b00111}, {415,1,0b10011}, {416,0,0b01101},
                        {417,1,0b01101}, {418,1,0b01001}, {419,0,0b00011},
                        {420,0,0b01101}, {421,1,0b11011}, {422,1,0b01001},
                        {423,0,0b01011}, {424,1,0b11001}, {425,1,0b10101},
                        {426,0,0b01111}, {427,1,0b10011}, {428,1,0b10011},
                        {430,0,0b00001}, {431,1,0b10101}, {432,1,0b00001},
                        {433,1,0b00101}, {434,0,0b00001}, {435,1,0b10101},
                        {436,0,0b00001}, {437,0,0b00001}, {438,1,0b10101},
                        {439,0,0b00001}, {440,0,0b00001}, {441,0,0b00011},
                        {442,0,0b01001}, {443,0,0b01101}, {444,1,0b11001},
                        {445,0,0b10001}, {446,1,0b11101}, {447,1,0b11111},
                        {448,0,0b00001}, {449,0,0b01011}, {450,0,0b10101},
                        {451,1,0b11011}, {452,0,0b00011}, {453,1,0b01111},
                        {454,1,0b00011}, {455,0,0b00001}, {456,0,0b10001},
                        {457,1,0b10101}, {458,0,0b10001}, {459,1,0b11111},
                        {460,1,0b11111}, {461,1,0b10011}, {462,0,0b00011},
                        {463,1,0b11111}, {464,1,0b01111}, {465,1,0b01101},
                        {466,1,0b10001}, {467,0,0b00111}, {468,1,0b10011},
                        {469,0,0b11101}, {470,1,0b11111}, {471,1,0b10111},
                        {472,0,0b00011}, {473,1,0b01011}, {474,1,0b11101},
                        {475,0,0b00001}, {476,0,0b00101}, {477,1,0b10101},
                        {478,0,0b10111}, {479,1,0b11111}, {480,1,0b10111},
                        {481,1,0b10011}, {482,0,0b01011}, {483,1,0b01101},
                        {484,0,0b10101}, {485,1,0b10101}, {486,0,0b10101},
                        {487,1,0b10111}, {488,0,0b00001}, {489,0,0b00101},
                        {490,0,0b00101}, {491,0,0b00101}, {492,1,0b01101},
                        {493,1,0b01111}, {494,1,0b00001}, {495,1,0b10001},
                        {496,0,0b01011}, {497,1,0b10001}, {498,1,0b11101},
                        {499,1,0b01001}, {500,1,0b01111}, {501,0,0b00101},
                        {502,0,0b01011}, {503,0,0b11101}, {504,1,0b11111},
                        {505,1,0b00111}, {506,1,0b10111}, {507,0,0b10111},
                        {508,1,0b11101}, {509,0,0b00001}, {510,1,0b00001},
                        {511,1,0b00001}, {512,1,0b01111}, {513,1,0b10101},
                        {514,1,0b00011}, {515,1,0b10101}, {516,1,0b01011},
                        {517,1,0b00111}, {518,1,0b11111}, {519,1,0b00011},
                        {520,1,0b00001}, {521,1,0b00111}, {524,0,0b00001},
                        {525,0,0b11101}, {526,1,0b11111}, {527,1,0b10011},
                        {528,0,0b00101}, {529,1,0b10001}, {530,1,0b10101},
                        {531,0,0b10001}, {532,1,0b10111}, {533,0,0b01101},
                        {534,0,0b01111}, {535,1,0b11111}, {536,0,0b00111},
                        {537,1,0b11011}};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }

            wh_iter_skip1(it);
            {
                const uint8_t expected_boundary[] = {0b11001001, 0b11101110,
                    0b11000100, 0b11001000, 0b11110011, 0b10110110, 0b11110100,
                    0b01100101};
                const std::vector<uint32_t> occupieds_pos = {4, 5, 6, 10, 14,
                    16, 17, 21, 24, 30, 33, 36, 37, 43, 44, 45, 46, 47, 48, 50,
                    51, 52, 54, 55, 57, 60, 61, 62, 63, 64, 67, 68, 72, 73, 74,
                    75, 77, 78, 79, 85, 88, 89, 90, 91, 92, 94, 95, 97, 100,
                    103, 104, 105, 106, 107, 108, 110, 111, 115, 118, 119, 120,
                    121, 122, 123, 124, 125, 126, 133, 134, 138, 139, 140, 144,
                    149, 150, 151, 153, 154, 157, 162, 164, 166, 168, 169, 172,
                    177, 178, 181, 182, 186, 187, 189, 194, 197, 204, 205, 207,
                    208, 209, 212, 214, 217, 218, 219, 221, 222, 224, 225, 228,
                    229, 230, 232, 233, 234, 235, 241, 244, 247, 252, 254, 255,
                    257, 261, 263, 265, 269, 270, 272, 274, 275, 276, 278, 280,
                    282, 285, 287, 288, 291, 292, 293, 294, 296, 300, 306, 307,
                    308, 310, 311, 313, 314, 315, 317, 320, 323, 327, 328, 329,
                    337, 338, 340, 341, 343, 344, 347, 350, 351, 353, 356, 361,
                    363, 365, 366, 368, 369, 371, 372, 373, 374, 375, 378, 381,
                    383, 384, 388, 390, 391, 393, 394, 397, 402, 407, 409, 410,
                    413, 415, 418, 419, 425, 427, 428, 429, 431};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = 
                    {{4,1,0b01101}, {6,1,0b01111}, {7,1,0b10101},
                        {12,1,0b11001}, {17,0,0b01101}, {18,1,0b11001},
                        {19,1,0b00011}, {21,1,0b10111}, {26,1,0b01001},
                        {29,1,0b11011}, {37,1,0b00111}, {41,1,0b10101},
                        {44,1,0b11111}, {46,1,0b01001}, {53,1,0b01111},
                        {54,1,0b10001}, {56,1,0b11101}, {57,1,0b01101},
                        {58,1,0b01101}, {59,1,0b01101}, {62,1,0b11001},
                        {63,0,0b00001}, {64,1,0b10101}, {65,1,0b00011},
                        {67,0,0b10101}, {68,1,0b11001}, {69,0,0b01011},
                        {70,1,0b11001}, {71,1,0b00111}, {74,1,0b11001},
                        {75,1,0b11101}, {77,1,0b01001}, {78,1,0b01111},
                        {79,1,0b01011}, {83,1,0b11011}, {84,0,0b01001},
                        {85,1,0b11001}, {89,0,0b01001}, {90,1,0b11111},
                        {91,0,0b01001}, {92,0,0b10101}, {93,1,0b11011},
                        {94,1,0b10001}, {95,1,0b00111}, {96,1,0b11101},
                        {97,1,0b11101}, {98,0,0b01011}, {99,1,0b10001},
                        {105,0,0b01001}, {106,1,0b01111}, {109,1,0b00101},
                        {110,1,0b11001}, {112,0,0b00001}, {113,1,0b00001},
                        {114,1,0b10101}, {115,1,0b11011}, {116,0,0b00001},
                        {117,0,0b10101}, {118,1,0b11011}, {119,1,0b01011},
                        {120,1,0b10011}, {124,1,0b10101}, {128,1,0b00011},
                        {129,1,0b10011}, {130,1,0b10111}, {131,1,0b11101},
                        {133,1,0b00111}, {134,0,0b00001}, {135,1,0b01011},
                        {136,0,0b00111}, {137,1,0b11011}, {138,1,0b11111},
                        {143,1,0b11011}, {146,1,0b11011}, {148,1,0b11111},
                        {149,1,0b01101}, {150,1,0b01111}, {151,1,0b00001},
                        {153,0,0b00001}, {154,1,0b01011}, {155,0,0b01001},
                        {156,1,0b01011}, {157,1,0b10101}, {158,0,0b01001},
                        {159,1,0b10101}, {165,0,0b00001}, {166,1,0b00101},
                        {167,0,0b01101}, {168,1,0b10101}, {171,1,0b00011},
                        {173,1,0b10011}, {174,0,0b10001}, {175,1,0b11101},
                        {179,1,0b01111}, {185,0,0b01111}, {186,1,0b10101},
                        {187,0,0b01111}, {188,1,0b11111}, {189,1,0b00011},
                        {190,1,0b00011}, {191,0,0b01111}, {192,1,0b10101},
                        {195,0,0b01011}, {196,1,0b11101}, {201,0,0b00011},
                        {202,1,0b00111}, {204,0,0b01001}, {205,1,0b01011},
                        {206,0,0b00101}, {207,0,0b10101}, {208,1,0b11011},
                        {209,1,0b10111}, {210,0,0b01101}, {211,1,0b01111},
                        {214,1,0b11001}, {220,0,0b00011}, {221,1,0b01101},
                        {222,0,0b01001}, {223,1,0b10111}, {225,1,0b00111},
                        {226,0,0b00101}, {227,1,0b10111}, {231,1,0b10111},
                        {232,1,0b01011}, {235,1,0b01011}, {241,1,0b10111},
                        {245,1,0b00001}, {253,1,0b01001}, {255,1,0b11011},
                        {257,1,0b01011}, {258,1,0b11111}, {260,1,0b11111},
                        {263,0,0b11011}, {264,1,0b11111}, {266,0,0b10101},
                        {267,1,0b11111}, {270,1,0b10001}, {271,0,0b00011},
                        {272,1,0b11011}, {273,0,0b00101}, {274,1,0b11111},
                        {275,0,0b00011}, {276,1,0b11111}, {277,1,0b11111},
                        {278,1,0b10111}, {280,1,0b00011}, {283,0,0b00101},
                        {284,1,0b11101}, {285,1,0b00111}, {286,1,0b01111},
                        {288,1,0b00101}, {290,1,0b11101}, {291,1,0b00001},
                        {292,1,0b10011}, {299,1,0b00101}, {303,0,0b01001},
                        {304,0,0b11001}, {305,1,0b11111}, {307,1,0b10011},
                        {313,1,0b11111}, {316,1,0b01001}, {317,1,0b01101},
                        {319,0,0b01001}, {320,0,0b01101}, {321,1,0b11001},
                        {324,1,0b01111}, {327,1,0b11101}, {329,1,0b01101},
                        {334,1,0b00111}, {336,1,0b11001}, {338,1,0b01011},
                        {341,1,0b11101}, {342,1,0b01001}, {343,0,0b01011},
                        {344,1,0b01111}, {346,1,0b01111}, {348,1,0b00001},
                        {350,1,0b10001}, {354,0,0b01011}, {355,1,0b11111},
                        {357,0,0b01011}, {358,1,0b11001}, {359,1,0b11111},
                        {362,0,0b01101}, {363,1,0b11001}, {364,1,0b00011},
                        {365,1,0b10001}, {366,1,0b10101}, {368,1,0b00001},
                        {373,1,0b11011}, {380,1,0b01011}, {382,1,0b00011},
                        {383,0,0b01001}, {384,1,0b11111}, {385,1,0b01111},
                        {387,1,0b10101}, {389,1,0b10101}, {390,1,0b10101},
                        {392,1,0b11001}, {394,0,0b00111}, {395,1,0b01111},
                        {398,1,0b11011}, {402,0,0b10101}, {403,1,0b11011},
                        {406,1,0b01111}, {408,1,0b00001}, {409,1,0b01101},
                        {419,1,0b10001}, {420,0,0b01001}, {421,1,0b11001},
                        {423,0,0b00011}, {424,1,0b11011}, {425,1,0b00001},
                        {426,0,0b00001}, {427,1,0b10001}, {428,0,0b01001},
                        {429,0,0b10001}, {430,1,0b11101}, {431,0,0b00011},
                        {432,0,0b11001}, {433,1,0b11001}, {435,1,0b11101},
                        {436,1,0b10111}, {439,1,0b10011}, {443,0,0b00011},
                        {444,1,0b01011}, {449,1,0b10001}, {451,1,0b01011},
                        {454,1,0b01011}, {455,0,0b01111}, {456,1,0b11111},
                        {458,1,0b01001}, {459,1,0b01011}, {461,1,0b11101},
                        {463,1,0b00111}, {464,0,0b01101}, {465,1,0b11111},
                        {466,1,0b11001}, {467,1,0b10111}, {470,1,0b10011},
                        {474,0,0b00111}, {475,1,0b11001}, {476,1,0b01001},
                        {477,0,0b01001}, {478,0,0b10011}, {479,1,0b11001},
                        {482,0,0b00101}, {483,1,0b11001}, {485,0,0b00011},
                        {486,0,0b00111}, {487,1,0b10111}, {488,1,0b10101},
                        {489,1,0b01111}, {490,0,0b01001}, {491,1,0b10001},
                        {494,1,0b01111}, {500,1,0b00101}, {506,1,0b00111},
                        {509,1,0b00101}, {510,1,0b10011}, {514,1,0b11101},
                        {516,0,0b00101}, {517,1,0b11001}, {520,0,0b10011},
                        {521,0,0b11001}, {522,1,0b11001}, {523,0,0b00011},
                        {524,1,0b11101}, {528,1,0b01111}, {531,1,0b00111},
                        {532,1,0b10111}, {533,0,0b01111}, {534,0,0b10011},
                        {535,1,0b10101}, {536,1,0b00111}};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }

            wh_iter_skip1(it);
            {
                const uint8_t expected_boundary[] = {0b11111111, 0b11111111,
                    0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111,
                    0b11111111};
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }
            wh_iter_destroy(it);
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

            Steroids<false> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor);

            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, nullptr, 0);
            {
                const size_t expected_boundary_length = 8;
                const uint8_t expected_boundary[expected_boundary_length] =
                    {0b00000000, 0b00000000, 0b00000000, 0b00000000,
                        0b00000000, 0b00000000, 0b00000000, 0b00000000};
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }

            wh_iter_skip1(it);
            {
                const size_t expected_boundary_length = 7;
                const uint8_t expected_boundary[expected_boundary_length] =
                    {0b00000000, 0b00010000, 0b00001100, 0b00111101, 0b11110111,
                        0b11101011, 0b00011100};
                const std::vector<uint32_t> occupieds_pos = {2, 3, 4, 5, 7, 10,
                    11, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                    27, 28, 30, 31, 32, 35, 36, 37, 39, 40, 41, 42, 43, 44, 47,
                    49, 50, 51, 53, 54, 55, 56, 58, 59, 60, 61, 62, 63, 65, 66,
                    67, 68, 69, 70, 72, 73, 74, 75, 78, 80, 82, 84, 85, 87, 88,
                    89, 90, 91, 92, 93, 94, 96, 98, 101, 102, 103, 105, 106,
                    107, 108, 110, 111, 112, 113, 114, 115, 117, 118, 119, 120,
                    121, 122, 123, 125, 126, 128, 129, 130, 131, 132, 135, 136,
                    137, 139, 140, 143, 144, 146, 148, 149, 150, 151, 152, 154,
                    155, 157, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
                    170, 171, 172, 173, 174, 177, 178, 179, 180, 181, 182, 183,
                    184, 185, 186, 187, 188, 189, 190, 192, 194, 195, 197, 198,
                    199, 201, 202, 204, 205, 207, 208, 209, 212, 214, 216, 217,
                    218, 219, 221, 222, 223, 224, 225, 226, 228, 229, 230, 232,
                    233, 234, 235, 236, 237, 238, 240, 242, 243, 244, 246, 247,
                    248, 249, 251, 254, 255, 256, 257, 258, 259, 260, 261, 262,
                    263, 264, 265, 266, 267, 268, 270, 271, 273, 274, 275, 276,
                    277, 278, 279, 281, 283, 285, 286, 287, 292, 293, 295, 297,
                    298, 299, 302, 303, 306, 307, 308, 309, 311, 312, 314, 318,
                    320, 321, 322, 323, 324, 326, 327, 328, 329, 330, 331, 332,
                    333, 334, 335, 336, 338, 339, 340, 341, 343, 344, 345, 346,
                    347, 348, 349, 351, 352, 353, 354, 355, 357, 359, 360, 361,
                    363, 365, 366, 368, 369, 372, 373, 375, 377, 379, 380, 381,
                    382, 384, 385, 386, 387, 388, 389, 390, 392, 393, 394, 395,
                    396, 399, 400, 401, 403, 405, 406};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = 
                    {{2,0,0b01011}, {3,1,0b10111}, {4,0,0b01011},
                        {5,1,0b01111}, {6,0,0b00001}, {7,1,0b00001},
                        {8,0,0b01011}, {9,0,0b01101}, {10,1,0b10001},
                        {11,0,0b00011}, {12,0,0b00101}, {13,0,0b01111},
                        {14,1,0b11111}, {15,1,0b01001}, {16,0,0b11101},
                        {17,1,0b11111}, {18,1,0b10011}, {19,1,0b11111},
                        {20,1,0b01111}, {21,0,0b00111}, {22,1,0b01111},
                        {23,0,0b10001}, {24,0,0b11101}, {25,1,0b11101},
                        {26,1,0b00101}, {27,0,0b00011}, {28,1,0b11001},
                        {29,0,0b00001}, {30,0,0b01101}, {31,1,0b10101},
                        {32,1,0b01011}, {33,0,0b00001}, {34,1,0b10011},
                        {35,1,0b01001}, {36,1,0b10001}, {37,0,0b10111},
                        {38,1,0b11111}, {39,1,0b00011}, {40,0,0b00101},
                        {41,0,0b01111}, {42,1,0b11111}, {43,0,0b01001},
                        {44,1,0b10011}, {45,0,0b00101}, {46,0,0b10101},
                        {47,1,0b11101}, {48,1,0b11011}, {49,0,0b00101},
                        {50,1,0b01011}, {51,1,0b01101}, {52,1,0b11101},
                        {53,0,0b00111}, {54,1,0b11011}, {55,1,0b01101},
                        {56,1,0b10101}, {57,0,0b10111}, {58,0,0b11001},
                        {59,1,0b11111}, {60,1,0b01101}, {61,0,0b00011},
                        {62,0,0b10101}, {63,0,0b10101}, {64,1,0b10111},
                        {65,1,0b11001}, {66,1,0b10111}, {67,0,0b00101},
                        {68,1,0b10011}, {69,1,0b10111}, {70,1,0b10101},
                        {71,1,0b10101}, {72,0,0b00001}, {73,0,0b01001},
                        {74,1,0b01101}, {75,1,0b10101}, {76,0,0b11001},
                        {77,1,0b11011}, {78,0,0b01001}, {79,1,0b10101},
                        {80,1,0b00101}, {81,0,0b01111}, {82,0,0b10011},
                        {83,1,0b10101}, {84,0,0b00011}, {85,1,0b11111},
                        {86,1,0b11101}, {87,0,0b01111}, {88,1,0b10011},
                        {89,1,0b11101}, {90,0,0b00001}, {91,0,0b01111},
                        {92,1,0b11111}, {93,1,0b01011}, {94,0,0b11011},
                        {95,1,0b11111}, {96,0,0b00111}, {97,1,0b11011},
                        {98,1,0b10101}, {99,1,0b01011}, {100,1,0b10011},
                        {101,1,0b10101}, {102,0,0b10001}, {103,0,0b10101},
                        {104,1,0b10101}, {105,1,0b10101}, {106,1,0b01111},
                        {108,0,0b00101}, {109,1,0b11111}, {110,1,0b00101},
                        {112,0,0b00101}, {113,1,0b01011}, {114,1,0b11111},
                        {116,0,0b00101}, {117,1,0b10111}, {118,1,0b10001},
                        {119,1,0b10011}, {120,0,0b00001}, {121,1,0b01101},
                        {122,0,0b00001}, {123,0,0b00101}, {124,0,0b01111},
                        {125,1,0b10101}, {126,0,0b00111}, {127,1,0b11101},
                        {128,1,0b10001}, {129,1,0b00011}, {130,1,0b11101},
                        {133,0,0b01101}, {134,1,0b01101}, {135,1,0b00001},
                        {136,1,0b01111}, {138,0,0b00011}, {139,1,0b00111},
                        {140,1,0b10111}, {141,1,0b00011}, {142,1,0b00101},
                        {145,1,0b10001}, {146,1,0b10101}, {147,0,0b10101},
                        {148,1,0b11001}, {149,1,0b00101}, {150,0,0b00101},
                        {151,0,0b00101}, {152,0,0b10101}, {153,1,0b10101},
                        {154,1,0b11001}, {155,0,0b01101}, {156,1,0b10001},
                        {157,0,0b10111}, {158,0,0b10111}, {159,1,0b11101},
                        {160,0,0b00111}, {161,1,0b10001}, {162,0,0b10001},
                        {163,1,0b11011}, {164,0,0b01111}, {165,1,0b11101},
                        {166,1,0b01001}, {167,1,0b00111}, {168,0,0b00101},
                        {169,0,0b01101}, {170,1,0b10001}, {171,1,0b11001},
                        {172,0,0b00101}, {173,1,0b01101}, {174,1,0b10011},
                        {175,1,0b11111}, {176,0,0b10111}, {177,1,0b11001},
                        {178,1,0b00111}, {179,0,0b01101}, {180,1,0b11111},
                        {181,1,0b00001}, {182,1,0b00011}, {183,0,0b00101},
                        {184,0,0b01001}, {185,0,0b01101}, {186,1,0b01101},
                        {187,0,0b10001}, {188,0,0b10011}, {189,1,0b11101},
                        {190,0,0b10101}, {191,1,0b11011}, {192,0,0b10011},
                        {193,1,0b10101}, {194,1,0b10101}, {195,0,0b00111},
                        {196,1,0b11101}, {197,0,0b00011}, {198,1,0b01001},
                        {199,0,0b10011}, {200,1,0b11101}, {201,0,0b00101},
                        {202,1,0b11101}, {203,1,0b10111}, {204,1,0b10001},
                        {205,1,0b11001}, {207,0,0b00101}, {208,1,0b10111},
                        {211,1,0b10001}, {212,1,0b01001}, {213,1,0b11111},
                        {215,0,0b10001}, {216,0,0b10001}, {217,0,0b10111},
                        {218,0,0b10111}, {219,1,0b11111}, {220,1,0b01111},
                        {221,1,0b11111}, {222,0,0b00011}, {223,1,0b00101},
                        {224,0,0b00011}, {225,1,0b10111}, {226,1,0b00001},
                        {227,1,0b00001}, {228,1,0b00011}, {229,0,0b01101},
                        {230,1,0b01111}, {231,0,0b01011}, {232,1,0b11111},
                        {233,1,0b00101}, {234,0,0b00001}, {235,0,0b01111},
                        {236,1,0b11001}, {237,0,0b00001}, {238,1,0b10111},
                        {239,0,0b10001}, {240,0,0b10011}, {241,1,0b11001},
                        {242,0,0b00001}, {243,0,0b01011}, {244,1,0b11011},
                        {245,1,0b00011}, {246,1,0b11001}, {247,1,0b01111},
                        {248,1,0b11001}, {249,0,0b10011}, {250,1,0b11001},
                        {251,1,0b10111}, {252,1,0b11111}, {253,0,0b00011},
                        {254,1,0b10101}, {255,1,0b01011}, {256,1,0b10011},
                        {257,0,0b11011}, {258,1,0b11101}, {259,0,0b00101},
                        {260,0,0b01111}, {261,0,0b10111}, {262,1,0b10111},
                        {263,0,0b00011}, {264,0,0b00011}, {265,1,0b10101},
                        {266,0,0b00011}, {267,0,0b01011}, {268,1,0b11001},
                        {269,0,0b10101}, {270,0,0b10101}, {271,0,0b10101},
                        {272,1,0b11101}, {273,0,0b00111}, {274,1,0b10001},
                        {275,1,0b11111}, {276,0,0b01001}, {277,0,0b10011},
                        {278,0,0b11011}, {279,1,0b11111}, {280,1,0b00111},
                        {281,0,0b00011}, {282,0,0b00101}, {283,1,0b10101},
                        {284,0,0b00111}, {285,1,0b10011}, {286,1,0b00011},
                        {287,0,0b01011}, {288,1,0b10111}, {289,1,0b11101},
                        {290,1,0b00101}, {291,0,0b00101}, {292,0,0b01001},
                        {293,0,0b01101}, {294,1,0b11101}, {295,0,0b01011},
                        {296,0,0b11001}, {297,1,0b11011}, {298,1,0b11011},
                        {299,1,0b11001}, {300,0,0b01101}, {301,1,0b11111},
                        {302,1,0b10101}, {303,0,0b00011}, {304,1,0b10001},
                        {305,0,0b10001}, {306,1,0b11101}, {307,1,0b11101},
                        {308,0,0b00111}, {309,1,0b01001}, {310,0,0b00111},
                        {311,1,0b01111}, {312,0,0b10011}, {313,1,0b11011},
                        {314,1,0b11101}, {315,1,0b10001}, {316,1,0b00001},
                        {317,0,0b00101}, {318,0,0b00101}, {319,1,0b10101},
                        {320,0,0b01011}, {321,1,0b10001}, {322,0,0b01011},
                        {323,1,0b01101}, {324,0,0b00011}, {325,1,0b00111},
                        {326,1,0b01101}, {327,1,0b00111}, {328,1,0b10101},
                        {329,0,0b01001}, {330,1,0b11101}, {331,1,0b00011},
                        {332,1,0b01101}, {333,1,0b01011}, {334,1,0b01001},
                        {335,1,0b01101}, {336,0,0b01001}, {337,1,0b11001},
                        {338,1,0b10001}, {339,1,0b11101}, {340,1,0b11111},
                        {341,1,0b10101}, {342,1,0b01011}, {343,0,0b11011},
                        {344,1,0b11111}, {345,1,0b11011}, {346,1,0b10101},
                        {347,1,0b10011}, {348,0,0b00011}, {349,0,0b01001},
                        {350,1,0b10001}, {351,0,0b01001}, {352,1,0b10101},
                        {353,1,0b00001}, {354,0,0b11001}, {355,1,0b11001},
                        {356,1,0b01001}, {357,1,0b11001}, {358,0,0b10101},
                        {359,1,0b11101}, {360,0,0b00001}, {361,1,0b00001},
                        {362,1,0b00101}, {363,1,0b00011}, {364,1,0b00001},
                        {365,0,0b00011}, {366,1,0b11001}, {367,1,0b00011},
                        {368,1,0b10111}, {369,1,0b11001}, {370,1,0b00011},
                        {371,1,0b01001}, {373,1,0b01101}, {376,1,0b00001},
                        {377,0,0b01001}, {378,0,0b01111}, {379,1,0b11111},
                        {380,1,0b00001}, {385,1,0b11101}, {387,0,0b00011},
                        {388,0,0b00101}, {389,0,0b01011}, {390,1,0b11111},
                        {391,0,0b01111}, {392,1,0b11011}, {393,1,0b00111},
                        {394,0,0b01011}, {395,1,0b10001}, {396,0,0b00011},
                        {397,0,0b01001}, {398,0,0b10111}, {399,1,0b11111},
                        {400,1,0b00001}, {401,1,0b10111}, {404,1,0b11101},
                        {405,1,0b01001}, {406,0,0b01001}, {407,1,0b11001},
                        {408,0,0b00101}, {409,0,0b10111}, {410,1,0b11001},
                        {411,1,0b10111}, {412,1,0b01011}, {414,0,0b00011},
                        {415,1,0b10011}, {418,0,0b01001}, {419,1,0b11101},
                        {420,0,0b00011}, {421,0,0b00011}, {422,0,0b11011},
                        {423,1,0b11101}, {424,0,0b10101}, {425,0,0b11011},
                        {426,1,0b11111}, {427,0,0b01101}, {428,1,0b01111},
                        {429,1,0b01011}, {430,0,0b00001}, {431,1,0b10111},
                        {432,1,0b11011}, {433,1,0b10101}, {434,1,0b11101},
                        {435,0,0b00001}, {436,0,0b01001}, {437,1,0b10101},
                        {438,1,0b00101}, {439,1,0b01101}, {440,0,0b01111},
                        {441,1,0b10011}, {442,1,0b00111}, {443,1,0b11011},
                        {444,1,0b01001}, {445,0,0b01011}, {446,0,0b10101},
                        {447,1,0b11001}, {448,0,0b01101}, {449,0,0b10011},
                        {450,0,0b10111}, {451,0,0b11101}, {452,1,0b11111},
                        {453,1,0b11101}, {454,1,0b01101}, {455,0,0b00101},
                        {456,0,0b10011}, {457,1,0b10011}, {458,1,0b11101},
                        {459,1,0b10101}, {460,1,0b00001}, {461,0,0b01001},
                        {462,1,0b10111}, {463,1,0b00111}, {464,0,0b10011},
                        {465,1,0b11001}, {466,1,0b00011}, {467,0,0b00101},
                        {468,1,0b11011}, {469,1,0b10001}, {470,1,0b01011},
                        {471,0,0b00111}, {472,0,0b11001}, {473,1,0b11111},
                        {474,0,0b01011}, {475,1,0b10111}, {476,1,0b11011},
                        {477,1,0b01001}, {478,0,0b00011}, {479,1,0b11011},
                        {480,1,0b01011}, {481,1,0b11101}, {482,1,0b00001},
                        {483,1,0b10011}, {484,0,0b00011}, {485,0,0b11101},
                        {486,1,0b11111}, {487,1,0b01011}, {488,0,0b01011},
                        {489,1,0b11111}, {490,1,0b11011}, {491,1,0b01101},
                        {492,1,0b01011}, {493,0,0b00101}, {494,0,0b00101},
                        {495,1,0b11001}, {496,1,0b10001}, {497,0,0b11011},
                        {498,1,0b11111}, {499,1,0b11101}, {500,0,0b00011},
                        {501,0,0b00101}, {502,0,0b01111}, {503,0,0b01111},
                        {504,0,0b10111}, {505,1,0b11001}, {506,0,0b00011},
                        {507,0,0b00101}, {508,1,0b11111}, {509,1,0b01001},
                        {510,0,0b00011}, {511,0,0b01011}, {512,1,0b10011},
                        {513,1,0b10101}, {514,1,0b10101}, {515,0,0b00001},
                        {516,1,0b11001}, {517,1,0b11111}, {518,0,0b11011},
                        {519,1,0b11101}, {520,1,0b11001}, {521,1,0b11001},
                        {522,0,0b01101}, {523,0,0b10001}, {524,1,0b11111},
                        {525,0,0b00111}, {526,0,0b10111}, {527,1,0b11011},
                        {528,1,0b00111}, {529,0,0b00101}, {530,1,0b11001},
                        {531,1,0b00101}, {532,0,0b10101}, {533,0,0b10111},
                        {534,1,0b11011}, {535,0,0b00011}, {536,0,0b01001},
                        {537,1,0b11101}};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }

            wh_iter_skip1(it);
            {
                const size_t expected_boundary_length = 7;
                const uint8_t expected_boundary[expected_boundary_length] =
                    {0b01100101, 0b11000011, 0b01001000, 0b01111100,
                        0b10000001, 0b11001001, 0b11011000};
                const std::vector<uint32_t> occupieds_pos = {0, 1, 3, 4, 5, 6,
                    7, 9, 10, 11, 12, 13, 14, 15, 17, 21, 22, 23, 26, 27, 28,
                    29, 30, 31, 32, 33, 36, 39, 40, 41, 43, 45, 46, 47, 49, 52,
                    53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 67, 68,
                    70, 71, 72, 75, 78, 80, 82, 83, 84, 85, 88, 89, 90, 91, 92,
                    93, 95, 96, 97, 98, 99, 100, 101, 102, 103, 105, 106, 107,
                    108, 111, 113, 114, 116, 117, 118, 121, 122, 123, 125, 126,
                    127, 128, 134, 135, 136, 137, 138, 140, 141, 142, 143, 144,
                    147, 148, 149, 150, 151, 154, 155, 156, 157, 158, 159, 160,
                    161, 162, 163, 164, 167, 169, 170, 171, 172, 174, 175, 176,
                    177, 179, 182, 183, 185, 186, 187, 188, 190, 191, 192, 193,
                    194, 195, 196, 197, 200, 201, 202, 203, 204, 206, 208, 209,
                    210, 211, 212, 213, 214, 215, 217, 221, 222, 223, 224, 225,
                    226, 227, 228, 230, 231, 232, 235, 236, 238, 239, 240, 241,
                    243, 244, 247, 248, 249, 251, 254, 256, 257, 258, 261, 262,
                    263, 264, 265, 269, 272, 273, 274, 275, 276, 277, 278, 280,
                    281, 282, 284, 286, 287, 289, 290, 291, 292, 294, 295, 296,
                    297, 298, 301, 302, 305, 306, 307, 308, 310, 312, 316, 319,
                    320, 321, 322, 323, 324, 326, 327, 328, 329, 330, 331, 332,
                    334, 335, 336, 337, 338, 340, 344, 345, 346, 347, 348, 350,
                    351, 352, 353, 356, 357, 358, 359, 360, 361, 362, 363, 364,
                    366, 367, 368, 369, 371, 372, 373, 374, 376, 377, 378, 379,
                    380, 382, 384, 385, 386, 387, 388, 391, 393, 394, 395, 396,
                    398, 399};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = 
                    {{0,1,0b10111}, {1,1,0b01101}, {4,1,0b10001},
                        {5,1,0b01001}, {6,0,0b00111}, {7,0,0b00111},
                        {8,1,0b01011}, {9,0,0b01011}, {10,1,0b01111},
                        {11,0,0b01001}, {12,1,0b10111}, {13,1,0b00111},
                        {14,0,0b00101}, {15,0,0b00111}, {16,1,0b10011},
                        {17,1,0b10011}, {18,0,0b00011}, {19,1,0b11111},
                        {20,0,0b01011}, {21,1,0b11111}, {22,0,0b01111},
                        {23,0,0b10101}, {24,1,0b11011}, {25,1,0b10011},
                        {26,0,0b00101}, {27,0,0b01011}, {28,0,0b10111},
                        {29,1,0b11011}, {30,0,0b00011}, {31,1,0b11001},
                        {32,0,0b01001}, {33,0,0b01001}, {34,1,0b10111},
                        {35,0,0b10011}, {36,1,0b10011}, {37,0,0b11011},
                        {38,1,0b11101}, {39,1,0b10011}, {40,0,0b01001},
                        {41,0,0b01011}, {42,1,0b11011}, {43,0,0b01011},
                        {44,0,0b10111}, {45,0,0b10111}, {46,1,0b11011},
                        {47,0,0b00011}, {48,0,0b00101}, {49,0,0b10101},
                        {50,1,0b11111}, {51,0,0b00101}, {52,1,0b11011},
                        {53,1,0b01101}, {54,0,0b00011}, {55,0,0b01111},
                        {56,1,0b10001}, {57,0,0b00001}, {58,0,0b00111},
                        {59,0,0b01001}, {60,0,0b01101}, {61,1,0b11011},
                        {62,0,0b01011}, {63,1,0b10101}, {64,1,0b10001},
                        {65,0,0b01111}, {66,1,0b11001}, {67,0,0b00111},
                        {68,0,0b10111}, {69,1,0b11101}, {70,1,0b00101},
                        {71,1,0b00111}, {72,0,0b00011}, {73,1,0b01101},
                        {74,0,0b00111}, {75,0,0b01111}, {76,1,0b11001},
                        {77,0,0b10001}, {78,0,0b10011}, {79,1,0b11011},
                        {80,1,0b00001}, {81,1,0b11101}, {82,0,0b01011},
                        {83,0,0b01111}, {84,1,0b10011}, {85,0,0b00111},
                        {86,0,0b01111}, {87,1,0b10111}, {88,1,0b00011},
                        {89,1,0b11001}, {90,1,0b11011}, {91,1,0b10111},
                        {92,1,0b11011}, {93,0,0b10101}, {94,1,0b11001},
                        {95,0,0b01101}, {96,1,0b11111}, {97,1,0b10101},
                        {98,1,0b11101}, {99,0,0b01111}, {100,1,0b10001},
                        {101,0,0b00101}, {102,0,0b10011}, {103,0,0b10101},
                        {104,1,0b11101}, {105,1,0b10111}, {106,0,0b00101},
                        {107,0,0b00111}, {108,1,0b11101}, {109,1,0b01011},
                        {110,1,0b11101}, {111,1,0b11011}, {112,1,0b01011},
                        {113,1,0b01001}, {114,0,0b00001}, {115,0,0b10101},
                        {116,1,0b11001}, {117,1,0b10001}, {118,0,0b01101},
                        {119,0,0b10011}, {120,1,0b11011}, {121,0,0b10111},
                        {122,1,0b11011}, {123,0,0b01111}, {124,0,0b10101},
                        {125,1,0b11011}, {126,0,0b00111}, {127,0,0b01001},
                        {128,0,0b10011}, {129,0,0b11001}, {130,1,0b11111},
                        {131,0,0b10101}, {132,1,0b11101}, {133,0,0b00101},
                        {134,0,0b10001}, {135,0,0b10011}, {136,1,0b11101},
                        {137,0,0b00001}, {138,1,0b11101}, {139,1,0b01101},
                        {140,1,0b11111}, {141,0,0b00101}, {142,1,0b00111},
                        {143,0,0b01011}, {144,0,0b10011}, {145,1,0b11011},
                        {146,0,0b00011}, {147,1,0b01101}, {148,1,0b00001},
                        {149,1,0b01001}, {150,1,0b10001}, {151,1,0b00111},
                        {152,0,0b01001}, {153,0,0b10111}, {154,1,0b11111},
                        {155,1,0b01101}, {156,0,0b00111}, {157,1,0b01011},
                        {158,0,0b00001}, {159,1,0b10111}, {160,0,0b00101},
                        {161,0,0b01101}, {162,1,0b10101}, {163,0,0b01101},
                        {164,1,0b10101}, {165,1,0b10011}, {166,1,0b01101},
                        {167,0,0b10101}, {168,1,0b11001}, {169,1,0b10111},
                        {170,1,0b01101}, {171,0,0b01111}, {172,1,0b10101},
                        {173,0,0b01011}, {174,0,0b01111}, {175,0,0b10011},
                        {176,1,0b11101}, {177,0,0b00011}, {178,1,0b11111},
                        {179,0,0b01011}, {180,1,0b10111}, {181,0,0b10001},
                        {182,1,0b11001}, {183,0,0b10101}, {184,1,0b10111},
                        {185,1,0b11011}, {186,1,0b10111}, {187,0,0b01001},
                        {188,0,0b01001}, {189,0,0b11011}, {190,1,0b11111},
                        {191,0,0b00111}, {192,1,0b11001}, {193,1,0b01001},
                        {194,1,0b01101}, {195,1,0b10111}, {196,0,0b00001},
                        {197,1,0b10001}, {198,1,0b11111}, {199,1,0b01101},
                        {200,0,0b01101}, {201,1,0b10011}, {202,1,0b11101},
                        {203,1,0b01111}, {204,1,0b00001}, {205,0,0b00111},
                        {206,0,0b01011}, {207,0,0b01111}, {208,1,0b11011},
                        {209,0,0b10111}, {210,1,0b11011}, {211,1,0b11111},
                        {212,1,0b00011}, {213,1,0b01111}, {214,0,0b00111},
                        {215,1,0b11001}, {216,0,0b01001}, {217,1,0b10001},
                        {218,1,0b10111}, {219,1,0b01001}, {220,0,0b00001},
                        {221,1,0b01101}, {222,1,0b00111}, {223,1,0b10011},
                        {224,1,0b10011}, {227,0,0b00011}, {228,1,0b10011},
                        {229,0,0b01111}, {230,0,0b11001}, {231,1,0b11111},
                        {232,0,0b00101}, {233,1,0b11001}, {234,1,0b10001},
                        {235,0,0b00001}, {236,0,0b00111}, {237,1,0b11001},
                        {238,0,0b10111}, {239,1,0b11011}, {240,1,0b10001},
                        {241,1,0b01001}, {242,0,0b01001}, {243,1,0b10011},
                        {244,1,0b00001}, {245,0,0b10011}, {246,1,0b10111},
                        {248,0,0b00001}, {249,1,0b11111}, {250,1,0b00101},
                        {251,0,0b01011}, {252,1,0b01111}, {253,1,0b10111},
                        {255,1,0b10001}, {256,1,0b00011}, {258,1,0b00011},
                        {259,1,0b00111}, {260,0,0b00101}, {261,0,0b01101},
                        {262,1,0b10111}, {263,0,0b10111}, {264,0,0b11001},
                        {265,1,0b11001}, {266,1,0b01111}, {267,1,0b11101},
                        {268,1,0b11001}, {270,0,0b01001}, {271,1,0b10101},
                        {272,0,0b10001}, {273,0,0b10111}, {274,1,0b11111},
                        {275,0,0b00011}, {276,1,0b00101}, {277,1,0b00101},
                        {278,1,0b11011}, {279,0,0b00001}, {280,1,0b10101},
                        {281,0,0b00001}, {282,0,0b01001}, {283,0,0b11111},
                        {284,1,0b11111}, {285,1,0b11111}, {286,1,0b01111},
                        {287,0,0b11111}, {288,1,0b11111}, {289,0,0b00101},
                        {290,1,0b11101}, {291,1,0b11101}, {292,1,0b00111},
                        {293,0,0b01101}, {294,1,0b10111}, {297,0,0b00001},
                        {298,1,0b10001}, {299,1,0b11111}, {300,1,0b00011},
                        {301,1,0b00101}, {302,1,0b01011}, {303,0,0b10111},
                        {304,1,0b10111}, {305,0,0b10001}, {306,0,0b10111},
                        {307,0,0b11001}, {308,1,0b11101}, {309,1,0b10111},
                        {310,0,0b00001}, {311,1,0b11001}, {312,1,0b10001},
                        {313,1,0b10101}, {315,1,0b01101}, {317,0,0b11001},
                        {318,1,0b11001}, {319,1,0b01111}, {321,0,0b01001},
                        {322,1,0b01101}, {323,0,0b00011}, {324,1,0b00111},
                        {325,1,0b11001}, {326,1,0b10001}, {327,1,0b01001},
                        {331,1,0b01101}, {333,1,0b01001}, {334,1,0b01111},
                        {337,0,0b10111}, {338,1,0b11011}, {341,1,0b00111},
                        {344,1,0b01101}, {345,0,0b00001}, {346,0,0b01001},
                        {347,0,0b01101}, {348,1,0b10101}, {349,1,0b00001},
                        {350,0,0b00011}, {351,1,0b11111}, {352,0,0b11001},
                        {353,1,0b11011}, {354,0,0b10011}, {355,0,0b10111},
                        {356,1,0b11001}, {357,0,0b10011}, {358,1,0b10111},
                        {359,0,0b01011}, {360,0,0b10011}, {361,1,0b11001},
                        {362,0,0b00111}, {363,1,0b11101}, {365,0,0b00101},
                        {366,1,0b01111}, {367,0,0b00101}, {368,0,0b10111},
                        {369,1,0b11101}, {370,1,0b10111}, {371,0,0b00011},
                        {372,0,0b01101}, {373,1,0b10101}, {374,1,0b01101},
                        {375,0,0b01001}, {376,1,0b11111}, {377,0,0b00011},
                        {378,1,0b00101}, {379,0,0b01111}, {380,1,0b11011},
                        {381,0,0b00001}, {382,0,0b01101}, {383,1,0b11101},
                        {384,0,0b00011}, {385,1,0b10101}, {386,0,0b00101},
                        {387,1,0b11101}, {388,0,0b01101}, {389,1,0b11111},
                        {390,0,0b01111}, {391,0,0b10011}, {392,0,0b10101},
                        {393,1,0b11001}, {394,0,0b01011}, {395,1,0b10011},
                        {396,0,0b00101}, {397,0,0b10001}, {398,1,0b11001},
                        {399,1,0b10101}, {400,1,0b01011}, {401,0,0b01001},
                        {402,1,0b10011}, {403,0,0b01001}, {404,1,0b11011},
                        {405,0,0b00001}, {406,0,0b00101}, {407,0,0b01101},
                        {408,0,0b10011}, {409,1,0b11001}, {410,0,0b00111},
                        {411,1,0b01101}, {412,0,0b01001}, {413,1,0b11101},
                        {414,1,0b00111}, {415,1,0b10011}, {416,0,0b01101},
                        {417,1,0b01101}, {418,1,0b01001}, {419,0,0b00011},
                        {420,0,0b01101}, {421,1,0b11011}, {422,1,0b01001},
                        {423,0,0b01011}, {424,1,0b11001}, {425,1,0b10101},
                        {426,0,0b01111}, {427,1,0b10011}, {428,1,0b10011},
                        {430,0,0b00001}, {431,1,0b10101}, {432,1,0b00001},
                        {433,1,0b00101}, {434,0,0b00001}, {435,1,0b10101},
                        {436,0,0b00001}, {437,0,0b00001}, {438,1,0b10101},
                        {439,0,0b00001}, {440,0,0b00001}, {441,0,0b00011},
                        {442,0,0b01001}, {443,0,0b01101}, {444,1,0b11001},
                        {445,0,0b10001}, {446,1,0b11101}, {447,1,0b11111},
                        {448,0,0b00001}, {449,0,0b01011}, {450,0,0b10101},
                        {451,1,0b11011}, {452,0,0b00011}, {453,1,0b01111},
                        {454,1,0b00011}, {455,0,0b00001}, {456,0,0b10001},
                        {457,1,0b10101}, {458,0,0b10001}, {459,1,0b11111},
                        {460,1,0b11111}, {461,1,0b10011}, {462,0,0b00011},
                        {463,1,0b11111}, {464,1,0b01111}, {465,1,0b01101},
                        {466,1,0b10001}, {467,0,0b00111}, {468,1,0b10011},
                        {469,0,0b11101}, {470,1,0b11111}, {471,1,0b10111},
                        {472,0,0b00011}, {473,1,0b01011}, {474,1,0b11101},
                        {475,0,0b00001}, {476,0,0b00101}, {477,1,0b10101},
                        {478,0,0b10111}, {479,1,0b11111}, {480,1,0b10111},
                        {481,1,0b10011}, {482,0,0b01011}, {483,1,0b01101},
                        {484,0,0b10101}, {485,1,0b10101}, {486,0,0b10101},
                        {487,1,0b10111}, {488,0,0b00001}, {489,0,0b00101},
                        {490,0,0b00101}, {491,0,0b00101}, {492,1,0b01101},
                        {493,1,0b01111}, {494,1,0b00001}, {495,1,0b10001},
                        {496,0,0b01011}, {497,1,0b10001}, {498,1,0b11101},
                        {499,1,0b01001}, {500,1,0b01111}, {501,0,0b00101},
                        {502,0,0b01011}, {503,0,0b11101}, {504,1,0b11111},
                        {505,1,0b00111}, {506,1,0b10111}, {507,0,0b10111},
                        {508,1,0b11101}, {509,0,0b00001}, {510,1,0b00001},
                        {511,1,0b00001}, {512,1,0b01111}, {513,1,0b10101},
                        {514,1,0b00011}, {515,1,0b10101}, {516,1,0b01011},
                        {517,1,0b00111}, {518,1,0b11111}, {519,1,0b00011},
                        {520,1,0b00001}, {521,1,0b00111}, {524,0,0b00001},
                        {525,0,0b11101}, {526,1,0b11111}, {527,1,0b10011},
                        {528,0,0b00101}, {529,1,0b10001}, {530,1,0b10101},
                        {531,0,0b10001}, {532,1,0b10111}, {533,0,0b01101},
                        {534,0,0b01111}, {535,1,0b11111}, {536,0,0b00111},
                        {537,1,0b11011}};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }

            wh_iter_skip1(it);
            {
                const size_t expected_boundary_length = 8;
                const uint8_t expected_boundary[expected_boundary_length] =
                    {0b11001001, 0b11101110, 0b11000100, 0b11001000,
                        0b11110011, 0b10110110, 0b11110100, 0b01100101};
                const std::vector<uint32_t> occupieds_pos = {4, 5, 6, 10, 14,
                    16, 17, 21, 24, 30, 33, 36, 37, 43, 44, 45, 46, 47, 48, 50,
                    51, 52, 54, 55, 57, 60, 61, 62, 63, 64, 67, 68, 72, 73, 74,
                    75, 77, 78, 79, 85, 88, 89, 90, 91, 92, 94, 95, 97, 100,
                    103, 104, 105, 106, 107, 108, 110, 111, 115, 118, 119, 120,
                    121, 122, 123, 124, 125, 126, 133, 134, 138, 139, 140, 144,
                    149, 150, 151, 153, 154, 157, 162, 164, 166, 168, 169, 172,
                    177, 178, 181, 182, 186, 187, 189, 194, 197, 204, 205, 207,
                    208, 209, 212, 214, 217, 218, 219, 221, 222, 224, 225, 228,
                    229, 230, 232, 233, 234, 235, 241, 244, 247, 252, 254, 255,
                    257, 261, 263, 265, 269, 270, 272, 274, 275, 276, 278, 280,
                    282, 285, 287, 288, 291, 292, 293, 294, 296, 300, 306, 307,
                    308, 310, 311, 313, 314, 315, 317, 320, 323, 327, 328, 329,
                    337, 338, 340, 341, 343, 344, 347, 350, 351, 353, 356, 361,
                    363, 365, 366, 368, 369, 371, 372, 373, 374, 375, 378, 381,
                    383, 384, 388, 390, 391, 393, 394, 397, 402, 407, 409, 410,
                    413, 415, 418, 419, 425, 427, 428, 429, 431};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = 
                    {{4,1,0b01101}, {6,1,0b01111}, {7,1,0b10101},
                        {12,1,0b11001}, {17,0,0b01101}, {18,1,0b11001},
                        {19,1,0b00011}, {21,1,0b10111}, {26,1,0b01001},
                        {29,1,0b11011}, {37,1,0b00111}, {41,1,0b10101},
                        {44,1,0b11111}, {46,1,0b01001}, {53,1,0b01111},
                        {54,1,0b10001}, {56,1,0b11101}, {57,1,0b01101},
                        {58,1,0b01101}, {59,1,0b01101}, {62,1,0b11001},
                        {63,0,0b00001}, {64,1,0b10101}, {65,1,0b00011},
                        {67,0,0b10101}, {68,1,0b11001}, {69,0,0b01011},
                        {70,1,0b11001}, {71,1,0b00111}, {74,1,0b11001},
                        {75,1,0b11101}, {77,1,0b01001}, {78,1,0b01111},
                        {79,1,0b01011}, {83,1,0b11011}, {84,0,0b01001},
                        {85,1,0b11001}, {89,0,0b01001}, {90,1,0b11111},
                        {91,0,0b01001}, {92,0,0b10101}, {93,1,0b11011},
                        {94,1,0b10001}, {95,1,0b00111}, {96,1,0b11101},
                        {97,1,0b11101}, {98,0,0b01011}, {99,1,0b10001},
                        {105,0,0b01001}, {106,1,0b01111}, {109,1,0b00101},
                        {110,1,0b11001}, {112,0,0b00001}, {113,1,0b00001},
                        {114,1,0b10101}, {115,1,0b11011}, {116,0,0b00001},
                        {117,0,0b10101}, {118,1,0b11011}, {119,1,0b01011},
                        {120,1,0b10011}, {124,1,0b10101}, {128,1,0b00011},
                        {129,1,0b10011}, {130,1,0b10111}, {131,1,0b11101},
                        {133,1,0b00111}, {134,0,0b00001}, {135,1,0b01011},
                        {136,0,0b00111}, {137,1,0b11011}, {138,1,0b11111},
                        {143,1,0b11011}, {146,1,0b11011}, {148,1,0b11111},
                        {149,1,0b01101}, {150,1,0b01111}, {151,1,0b00001},
                        {153,0,0b00001}, {154,1,0b01011}, {155,0,0b01001},
                        {156,1,0b01011}, {157,1,0b10101}, {158,0,0b01001},
                        {159,1,0b10101}, {165,0,0b00001}, {166,1,0b00101},
                        {167,0,0b01101}, {168,1,0b10101}, {171,1,0b00011},
                        {173,1,0b10011}, {174,0,0b10001}, {175,1,0b11101},
                        {179,1,0b01111}, {185,0,0b01111}, {186,1,0b10101},
                        {187,0,0b01111}, {188,1,0b11111}, {189,1,0b00011},
                        {190,1,0b00011}, {191,0,0b01111}, {192,1,0b10101},
                        {195,0,0b01011}, {196,1,0b11101}, {201,0,0b00011},
                        {202,1,0b00111}, {204,0,0b01001}, {205,1,0b01011},
                        {206,0,0b00101}, {207,0,0b10101}, {208,1,0b11011},
                        {209,1,0b10111}, {210,0,0b01101}, {211,1,0b01111},
                        {214,1,0b11001}, {220,0,0b00011}, {221,1,0b01101},
                        {222,0,0b01001}, {223,1,0b10111}, {225,1,0b00111},
                        {226,0,0b00101}, {227,1,0b10111}, {231,1,0b10111},
                        {232,1,0b01011}, {235,1,0b01011}, {241,1,0b10111},
                        {245,1,0b00001}, {253,1,0b01001}, {255,1,0b11011},
                        {257,1,0b01011}, {258,1,0b11111}, {260,1,0b11111},
                        {263,0,0b11011}, {264,1,0b11111}, {266,0,0b10101},
                        {267,1,0b11111}, {270,1,0b10001}, {271,0,0b00011},
                        {272,1,0b11011}, {273,0,0b00101}, {274,1,0b11111},
                        {275,0,0b00011}, {276,1,0b11111}, {277,1,0b11111},
                        {278,1,0b10111}, {280,1,0b00011}, {283,0,0b00101},
                        {284,1,0b11101}, {285,1,0b00111}, {286,1,0b01111},
                        {288,1,0b00101}, {290,1,0b11101}, {291,1,0b00001},
                        {292,1,0b10011}, {299,1,0b00101}, {303,0,0b01001},
                        {304,0,0b11001}, {305,1,0b11111}, {307,1,0b10011},
                        {313,1,0b11111}, {316,1,0b01001}, {317,1,0b01101},
                        {319,0,0b01001}, {320,0,0b01101}, {321,1,0b11001},
                        {324,1,0b01111}, {327,1,0b11101}, {329,1,0b01101},
                        {334,1,0b00111}, {336,1,0b11001}, {338,1,0b01011},
                        {341,1,0b11101}, {342,1,0b01001}, {343,0,0b01011},
                        {344,1,0b01111}, {346,1,0b01111}, {348,1,0b00001},
                        {350,1,0b10001}, {354,0,0b01011}, {355,1,0b11111},
                        {357,0,0b01011}, {358,1,0b11001}, {359,1,0b11111},
                        {362,0,0b01101}, {363,1,0b11001}, {364,1,0b00011},
                        {365,1,0b10001}, {366,1,0b10101}, {368,1,0b00001},
                        {373,1,0b11011}, {380,1,0b01011}, {382,1,0b00011},
                        {383,0,0b01001}, {384,1,0b11111}, {385,1,0b01111},
                        {387,1,0b10101}, {389,1,0b10101}, {390,1,0b10101},
                        {392,1,0b11001}, {394,0,0b00111}, {395,1,0b01111},
                        {398,1,0b11011}, {402,0,0b10101}, {403,1,0b11011},
                        {406,1,0b01111}, {408,1,0b00001}, {409,1,0b01101},
                        {419,1,0b10001}, {420,0,0b01001}, {421,1,0b11001},
                        {423,0,0b00011}, {424,1,0b11011}, {425,1,0b00001},
                        {426,0,0b00001}, {427,1,0b10001}, {428,0,0b01001},
                        {429,0,0b10001}, {430,1,0b11101}, {431,0,0b00011},
                        {432,0,0b11001}, {433,1,0b11001}, {435,1,0b11101},
                        {436,1,0b10111}, {439,1,0b10011}, {443,0,0b00011},
                        {444,1,0b01011}, {449,1,0b10001}, {451,1,0b01011},
                        {454,1,0b01011}, {455,0,0b01111}, {456,1,0b11111},
                        {458,1,0b01001}, {459,1,0b01011}, {461,1,0b11101},
                        {463,1,0b00111}, {464,0,0b01101}, {465,1,0b11111},
                        {466,1,0b11001}, {467,1,0b10111}, {470,1,0b10011},
                        {474,0,0b00111}, {475,1,0b11001}, {476,1,0b01001},
                        {477,0,0b01001}, {478,0,0b10011}, {479,1,0b11001},
                        {482,0,0b00101}, {483,1,0b11001}, {485,0,0b00011},
                        {486,0,0b00111}, {487,1,0b10111}, {488,1,0b10101},
                        {489,1,0b01111}, {490,0,0b01001}, {491,1,0b10001},
                        {494,1,0b01111}, {500,1,0b00101}, {506,1,0b00111},
                        {509,1,0b00101}, {510,1,0b10011}, {514,1,0b11101},
                        {516,0,0b00101}, {517,1,0b11001}, {520,0,0b10011},
                        {521,0,0b11001}, {522,1,0b11001}, {523,0,0b00011},
                        {524,1,0b11101}, {528,1,0b01111}, {531,1,0b00111},
                        {532,1,0b10111}, {533,0,0b01111}, {534,0,0b10011},
                        {535,1,0b10101}, {536,1,0b00111}};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }

            wh_iter_skip1(it);
            {
                const size_t expected_boundary_length = 8;
                const uint8_t expected_boundary[expected_boundary_length] =
                    {0b11111111, 0b11111111, 0b11111111, 0b11111111,
                        0b11111111, 0b11111111, 0b11111111, 0b11111111};
                const std::vector<uint32_t> occupieds_pos = {};
                const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                const uint8_t *res_key;
                uint32_t res_size, dummy;
                Steroids<false>::InfixStore *store;
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);
            }
            wh_iter_destroy(it);
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
        std::cerr << " size_grade=" << size_grade << " elem_count=" << store.GetElemCount();
        std::cerr << " --- ptr=" << store.ptr << std::endl;
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

TEST_SUITE("steroids") {
    TEST_CASE("insert") {
        SteroidsTests::Insert();
    }

    TEST_CASE("random inserts") {
        SteroidsTests::RandomInsert();
    }

    TEST_CASE("point query") {
        SteroidsTests::PointQuery();
    }

    TEST_CASE("range query") {
        SteroidsTests::RangeQuery();
    }

    TEST_CASE("delete") {
        SteroidsTests::Delete();
    }

    TEST_CASE("shrink infix size") {
        SteroidsTests::ShrinkInfixSize();
    }

    TEST_CASE("bulk load") {
        SteroidsTests::BulkLoad();
    }
}

