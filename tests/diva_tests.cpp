/**
 * @file diva tests
 * @author ---
 */

#include "wormhole/wh.h"
#include "wormhole/wh_int.h"
#include <endian.h>
#include <limits>
#include <random>
#include <x86intrin.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "diva.hpp"
#include "util.hpp"

const std::string ansi_green = "\033[0;32m";
const std::string ansi_white = "\033[0;97m";

class DivaTests {
public:
    template <bool O>
    static void Insert() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<O> s(infix_size, seed, load_factor);

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
            const std::vector<uint32_t> occupieds_pos = {5, 11, 16, 21, 27, 32, 38, 43, 49, 54, 60, 65, 71, 76, 82, 87, 92, 98, 103, 109, 114, 120, 125, 131, 136, 142, 147, 153, 158, 163, 169, 174, 180, 185, 191, 196, 202, 207, 213, 218, 224, 229, 234, 240, 245, 251, 256, 262, 267, 273, 278, 284, 289, 295, 300, 305, 311, 316, 322, 327, 333, 338, 344, 349, 355, 360, 366, 371, 376, 382, 387, 393, 398, 404, 409, 415, 420, 426, 431, 437, 442, 447, 453, 458, 464, 469, 475, 478, 480, 486, 491, 497, 502, 508, 513, 518, 524, 529, 535, 540};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  9,1,0b10011},   { 21,1,0b00001},   { 31,1,0b10001},   { 41,1,0b11111},   { 53,1,0b01111},   { 63,1,0b11101},   { 74,1,0b01011},   { 84,1,0b11011},   { 96,1,0b01001},   {106,1,0b10111},   {118,1,0b00111},   {128,1,0b10101},   {139,1,0b00101},   {149,1,0b10011},   {161,1,0b00001},   {171,1,0b10001},   {181,1,0b11111},   {193,1,0b01101},   {202,1,0b11101},   {214,1,0b01011},   {224,1,0b11011},   {236,1,0b01001},   {246,1,0b10111},   {258,1,0b00111},   {267,1,0b10101},   {279,1,0b00101},   {289,1,0b10011},   {301,1,0b00001},   {311,1,0b10001},   {321,1,0b11111},   {333,1,0b01101},   {342,1,0b11101},   {354,1,0b01011},   {364,1,0b11011},   {376,1,0b01001},   {386,1,0b10111},   {398,1,0b00111},   {407,1,0b10101},   {419,1,0b00101},   {429,1,0b10011},   {441,1,0b00001},   {451,1,0b10001},   {461,1,0b11111},   {472,1,0b01101},   {482,1,0b11101},   {494,1,0b01011},   {504,1,0b11011},   {516,1,0b01001},   {526,1,0b10111},   {537,1,0b00111},   {547,1,0b10101},   {559,1,0b00011},   {569,1,0b10011},   {581,1,0b00001},   {591,1,0b10001},   {601,1,0b11111},   {612,1,0b01101},   {622,1,0b11101},   {634,1,0b01011},   {644,1,0b11011},   {656,1,0b01001},   {666,1,0b10111},   {677,1,0b00111},   {687,1,0b10101},   {699,1,0b00011},   {709,1,0b10011},   {721,1,0b00001},   {731,1,0b10001},   {740,1,0b11111},   {752,1,0b01101},   {762,1,0b11101},   {774,1,0b01011},   {784,1,0b11001},   {796,1,0b01001},   {805,1,0b10111},   {817,1,0b00111},   {827,1,0b10101},   {839,1,0b00011},   {849,1,0b10011},   {861,1,0b00001},   {870,1,0b10001},   {880,1,0b11111},   {892,1,0b01101},   {902,1,0b11101},   {914,1,0b01011},   {924,1,0b11001},   {935,1,0b01001},   {941,1,0b00001},   {945,1,0b10111},   {957,1,0b00111},   {967,1,0b10101},   {979,1,0b00011},   {989,1,0b10011},   {1001,1,0b00001},   {1010,1,0b01111},   {1020,1,0b11111},   {1032,1,0b01101},   {1042,1,0b11101},   {1054,1,0b01011},   {1064,1,0b11001}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore *store;

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                wh_iter_destroy(it);
            }
        }

        for (int32_t i = 90; i >= 70; i -= 2) {
            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * i + r * (100 - i)) / 100;
            value = to_big_endian_order(interp);
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        }
        SUBCASE("overlapping interpolated reversed inserts with a stride of 2") {
            const std::vector<uint32_t> occupieds_pos = {5, 11, 16, 21, 27, 32, 38, 43, 49, 54, 60, 65, 71, 76, 82, 87, 92, 98, 103, 109, 114, 120, 125, 131, 136, 142, 147, 153, 158, 163, 169, 174, 180, 185, 191, 196, 202, 207, 213, 218, 224, 229, 234, 240, 245, 251, 256, 262, 267, 273, 278, 284, 289, 295, 300, 305, 311, 316, 322, 327, 333, 338, 344, 349, 355, 360, 366, 371, 376, 382, 387, 393, 398, 404, 409, 415, 420, 426, 431, 437, 442, 447, 453, 458, 464, 469, 475, 478, 480, 486, 491, 497, 502, 508, 513, 518, 524, 529, 535, 540};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  9,1,0b10011},   { 21,1,0b00001},   { 31,1,0b10001},   { 41,1,0b11111},   { 53,1,0b01111},   { 63,1,0b11101},   { 74,1,0b01011},   { 84,1,0b11011},   { 96,1,0b01001},   {106,0,0b10111},   {107,1,0b10111},   {118,1,0b00111},   {128,0,0b10101},   {129,1,0b10101},   {139,1,0b00101},   {149,0,0b10011},   {150,1,0b10011},   {161,1,0b00001},   {171,0,0b10001},   {172,1,0b10001},   {181,1,0b11111},   {193,0,0b01101},   {194,1,0b01101},   {202,1,0b11101},   {214,0,0b01011},   {215,1,0b01011},   {224,1,0b11011},   {236,0,0b01001},   {237,1,0b01001},   {246,1,0b10111},   {258,0,0b00111},   {259,1,0b00111},   {267,1,0b10101},   {279,0,0b00101},   {280,1,0b00101},   {289,1,0b10011},   {301,0,0b00001},   {302,1,0b00001},   {311,1,0b10001},   {321,0,0b11111},   {322,1,0b11111},   {333,1,0b01101},   {342,1,0b11101},   {354,1,0b01011},   {364,1,0b11011},   {376,1,0b01001},   {386,1,0b10111},   {398,1,0b00111},   {407,1,0b10101},   {419,1,0b00101},   {429,1,0b10011},   {441,1,0b00001},   {451,1,0b10001},   {461,1,0b11111},   {472,1,0b01101},   {482,1,0b11101},   {494,1,0b01011},   {504,1,0b11011},   {516,1,0b01001},   {526,1,0b10111},   {537,1,0b00111},   {547,1,0b10101},   {559,1,0b00011},   {569,1,0b10011},   {581,1,0b00001},   {591,1,0b10001},   {601,1,0b11111},   {612,1,0b01101},   {622,1,0b11101},   {634,1,0b01011},   {644,1,0b11011},   {656,1,0b01001},   {666,1,0b10111},   {677,1,0b00111},   {687,1,0b10101},   {699,1,0b00011},   {709,1,0b10011},   {721,1,0b00001},   {731,1,0b10001},   {740,1,0b11111},   {752,1,0b01101},   {762,1,0b11101},   {774,1,0b01011},   {784,1,0b11001},   {796,1,0b01001},   {805,1,0b10111},   {817,1,0b00111},   {827,1,0b10101},   {839,1,0b00011},   {849,1,0b10011},   {861,1,0b00001},   {870,1,0b10001},   {880,1,0b11111},   {892,1,0b01101},   {902,1,0b11101},   {914,1,0b01011},   {924,1,0b11001},   {935,1,0b01001},   {941,1,0b00001},   {945,1,0b10111},   {957,1,0b00111},   {967,1,0b10101},   {979,1,0b00011},   {989,1,0b10011},   {1001,1,0b00001},   {1010,1,0b01111},   {1020,1,0b11111},   {1032,1,0b01101},   {1042,1,0b11101},   {1054,1,0b01011},   {1064,1,0b11001}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore *store;

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                wh_iter_destroy(it);
            }
        }

        const uint32_t shamt = 16;
        for (int32_t i = 1; i < 50; i++) {
            const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
            const uint64_t interp = (l * 30 + r * 70) / 100 + (i << shamt);
            value = to_big_endian_order(interp);
            s.Insert(reinterpret_cast<uint8_t *>(&value), sizeof(value));
        }
        SUBCASE("overlapping interpolated consecutive inserts") {
            const std::vector<uint32_t> occupieds_pos = {5, 11, 16, 21, 27, 32, 38, 43, 49, 54, 60, 65, 71, 76, 82, 87, 92, 98, 103, 109, 114, 120, 125, 131, 136, 142, 147, 153, 158, 163, 169, 174, 180, 185, 191, 196, 202, 207, 213, 218, 224, 229, 234, 240, 245, 251, 256, 262, 267, 273, 278, 284, 289, 295, 300, 305, 311, 316, 322, 327, 333, 338, 344, 349, 355, 360, 366, 371, 376, 382, 383, 384, 385, 386, 387, 388, 393, 398, 404, 409, 415, 420, 426, 431, 437, 442, 447, 453, 458, 464, 469, 475, 478, 480, 486, 491, 497, 502, 508, 513, 518, 524, 529, 535, 540};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  9,1,0b10011},   { 21,1,0b00001},   { 31,1,0b10001},   { 41,1,0b11111},   { 53,1,0b01111},   { 63,1,0b11101},   { 74,1,0b01011},   { 84,1,0b11011},   { 96,1,0b01001},   {106,0,0b10111},   {107,1,0b10111},   {118,1,0b00111},   {128,0,0b10101},   {129,1,0b10101},   {139,1,0b00101},   {149,0,0b10011},   {150,1,0b10011},   {161,1,0b00001},   {171,0,0b10001},   {172,1,0b10001},   {181,1,0b11111},   {193,0,0b01101},   {194,1,0b01101},   {202,1,0b11101},   {214,0,0b01011},   {215,1,0b01011},   {224,1,0b11011},   {236,0,0b01001},   {237,1,0b01001},   {246,1,0b10111},   {258,0,0b00111},   {259,1,0b00111},   {267,1,0b10101},   {279,0,0b00101},   {280,1,0b00101},   {289,1,0b10011},   {301,0,0b00001},   {302,1,0b00001},   {311,1,0b10001},   {321,0,0b11111},   {322,1,0b11111},   {333,1,0b01101},   {342,1,0b11101},   {354,1,0b01011},   {364,1,0b11011},   {376,1,0b01001},   {386,1,0b10111},   {398,1,0b00111},   {407,1,0b10101},   {419,1,0b00101},   {429,1,0b10011},   {441,1,0b00001},   {451,1,0b10001},   {461,1,0b11111},   {472,1,0b01101},   {482,1,0b11101},   {494,1,0b01011},   {504,1,0b11011},   {516,1,0b01001},   {526,1,0b10111},   {537,1,0b00111},   {547,1,0b10101},   {559,1,0b00011},   {569,1,0b10011},   {581,1,0b00001},   {591,1,0b10001},   {601,1,0b11111},   {612,1,0b01101},   {622,1,0b11101},   {634,1,0b01011},   {644,1,0b11011},   {656,1,0b01001},   {666,1,0b10111},   {677,1,0b00111},   {687,1,0b10101},   {699,1,0b00011},   {709,1,0b10011},   {721,1,0b00001},   {731,1,0b10001},   {740,1,0b11111},   {752,0,0b01101},   {753,0,0b10001},   {754,0,0b10101},   {755,0,0b11001},   {756,1,0b11101},   {757,0,0b00001},   {758,0,0b00101},   {759,0,0b01001},   {760,0,0b01101},   {761,0,0b10001},   {762,0,0b10101},   {763,0,0b11001},   {764,1,0b11101},   {765,0,0b00001},   {766,0,0b00101},   {767,0,0b01001},   {768,0,0b01101},   {769,0,0b10001},   {770,0,0b10101},   {771,0,0b11001},   {772,1,0b11101},   {773,0,0b00001},   {774,0,0b00101},   {775,0,0b01001},   {776,0,0b01101},   {777,0,0b10001},   {778,0,0b10101},   {779,0,0b11001},   {780,1,0b11101},   {781,0,0b00001},   {782,0,0b00101},   {783,0,0b01001},   {784,0,0b01101},   {785,0,0b10001},   {786,0,0b10101},   {787,0,0b11001},   {788,1,0b11101},   {789,0,0b00001},   {790,0,0b00101},   {791,0,0b01001},   {792,0,0b01101},   {793,0,0b10001},   {794,0,0b10101},   {795,0,0b11001},   {796,0,0b11101},   {797,1,0b11101},   {798,0,0b00001},   {799,0,0b00101},   {800,0,0b01001},   {801,0,0b01101},   {802,1,0b10001},   {803,1,0b01011},   {804,1,0b11001},   {805,1,0b01001},   {806,1,0b10111},   {817,1,0b00111},   {827,1,0b10101},   {839,1,0b00011},   {849,1,0b10011},   {861,1,0b00001},   {870,1,0b10001},   {880,1,0b11111},   {892,1,0b01101},   {902,1,0b11101},   {914,1,0b01011},   {924,1,0b11001},   {935,1,0b01001},   {941,1,0b00001},   {945,1,0b10111},   {957,1,0b00111},   {967,1,0b10101},   {979,1,0b00011},   {989,1,0b10011},   {1001,1,0b00001},   {1010,1,0b01111},   {1020,1,0b11111},   {1032,1,0b01101},   {1042,1,0b11101},   {1054,1,0b01011},   {1064,1,0b11001}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore *store;

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                wh_iter_destroy(it);
            }
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value = to_big_endian_order(value);
        s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)});

        SUBCASE("split infix store: left half") {
            const std::vector<uint32_t> occupieds_pos = {11, 22, 33, 43, 54, 65, 76, 87, 98, 109, 120, 131, 142, 153, 164, 175, 185, 196, 207, 218, 229, 240, 251, 262, 273, 284, 295, 306, 317, 327, 338, 349, 360, 371, 382, 393, 404, 415, 426, 437, 448, 459, 469, 480, 491, 502, 513, 524, 535, 546, 557, 568, 579, 590, 601, 611, 622, 633, 644, 655, 666, 677, 688, 699, 710, 721, 732, 743, 753, 764, 765, 766};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,1,0b00110},   {  2,1,0b00010},   {  4,1,0b00010},   {  5,1,0b11110},   {  6,1,0b11110},   {  8,1,0b11010},   {  9,1,0b10110},   { 10,1,0b10110},   { 12,1,0b10010},   { 13,0,0b01110},   { 14,1,0b01110},   { 15,1,0b01110},   { 16,0,0b01010},   { 17,1,0b01010},   { 18,1,0b01010},   { 19,0,0b00110},   { 20,1,0b00110},   { 21,1,0b00010},   { 22,0,0b00010},   { 23,1,0b00010},   { 24,1,0b11110},   { 25,0,0b11010},   { 26,1,0b11010},   { 27,1,0b11010},   { 28,0,0b10110},   { 29,1,0b10110},   { 30,1,0b10110},   { 31,0,0b10010},   { 32,1,0b10010},   { 33,1,0b01110},   { 34,0,0b01110},   { 35,1,0b01110},   { 36,1,0b01010},   { 37,0,0b01010},   { 38,1,0b01010},   { 39,1,0b00110},   { 40,0,0b00010},   { 41,1,0b00010},   { 42,1,0b00010},   { 43,0,0b11110},   { 44,1,0b11110},   { 45,1,0b11010},   { 46,1,0b11010},   { 47,1,0b10110},   { 48,1,0b10110},   { 49,1,0b10010},   { 50,1,0b01110},   { 51,1,0b01110},   { 52,1,0b01010},   { 53,1,0b01010},   { 55,1,0b00110},   { 56,1,0b00010},   { 57,1,0b00010},   { 59,1,0b11110},   { 60,1,0b11010},   { 61,1,0b11010},   { 63,1,0b10110},   { 64,1,0b10110},   { 66,1,0b10010},   { 67,1,0b01110},   { 68,1,0b01110},   { 69,1,0b01010},   { 70,1,0b00110},   { 71,1,0b00110},   { 72,1,0b00010},   { 73,1,0b00010},   { 74,1,0b11110},   { 75,1,0b11010},   { 76,1,0b11010},   { 77,1,0b10110},   { 78,1,0b10110},   { 79,1,0b10010},   { 80,1,0b01110},   { 81,1,0b01110},   { 82,1,0b01010},   { 83,1,0b00110},   { 84,1,0b00110},   { 85,1,0b00010},   { 86,1,0b00010},   { 87,1,0b11110},   { 88,1,0b11010},   { 89,0,0b00010},   { 90,0,0b01010},   { 91,0,0b10010},   { 92,1,0b11010},   { 93,0,0b00010},   { 94,0,0b01010},   { 95,1,0b10010}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore *store;

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                const uint64_t value = to_big_endian_order(0x0000000011111111UL);
                wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));

                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                REQUIRE_FALSE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 0);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                wh_int_iter_destroy(it);
            }
            else {
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
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
        value &= ~BITMASK(shamt);
        value = to_big_endian_order(value);
        SUBCASE("split infix store: right half") {
            const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 40, 62, 84, 105, 127, 149, 171, 193, 215, 237, 258, 280, 302, 324, 346, 368, 379, 389, 411, 433, 455, 477, 499, 520, 542, 564, 586, 608, 630};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,1,0b10111},   {  1,0,0b00100},   {  2,1,0b10100},   {  3,0,0b00100},   {  4,1,0b10100},   {  5,0,0b00100},   {  6,1,0b10100},   {  7,0,0b00100},   {  8,1,0b10100},   {  9,0,0b00100},   { 10,1,0b10100},   { 11,0,0b00100},   { 12,1,0b10100},   { 13,0,0b00100},   { 14,1,0b10100},   { 15,0,0b00100},   { 16,1,0b10100},   { 17,0,0b00100},   { 18,1,0b10100},   { 19,0,0b00100},   { 20,1,0b10100},   { 21,0,0b00100},   { 22,1,0b10100},   { 23,0,0b00100},   { 24,1,0b10100},   { 25,0,0b00100},   { 26,1,0b10100},   { 27,0,0b00100},   { 28,1,0b10100},   { 29,0,0b00100},   { 30,1,0b10100},   { 31,0,0b00100},   { 32,1,0b10100},   { 33,0,0b00100},   { 34,1,0b10100},   { 35,0,0b00100},   { 36,0,0b10100},   { 37,1,0b10100},   { 38,0,0b00100},   { 39,1,0b10100},   { 40,0,0b00100},   { 41,1,0b10100},   { 42,1,0b00100},   { 43,1,0b01100},   { 44,1,0b00100},   { 45,1,0b00100},   { 46,1,0b11100},   { 47,1,0b11100},   { 48,1,0b10100},   { 49,1,0b01100},   { 50,1,0b01100},   { 51,1,0b00100},   { 52,1,0b00100},   { 53,1,0b11100},   { 54,1,0b10100},   { 55,1,0b10100},   { 56,1,0b01100},   { 57,1,0b00100},   { 58,1,0b00100},   { 59,1,0b00100},   { 60,1,0b11100},   { 61,1,0b11100},   { 62,1,0b10100},   { 63,1,0b01100},   { 64,1,0b01100},   { 65,1,0b00100},   { 66,1,0b11100},   { 67,1,0b11100},   { 68,1,0b10100},   { 70,1,0b10100},   { 73,1,0b01100},   { 76,1,0b00100}};
            uint8_t res_key[sizeof(uint64_t)];
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore store;

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);

                wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);

                wh_int_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size, 
                                 reinterpret_cast<void *>(&store), sizeof(typename Diva<O>::InfixStore), &dummy);
                REQUIRE(store.IsPartialKey());
                REQUIRE_EQ(store.GetInvalidBits(), 7);
                AssertStoreContents(s, store, occupieds_pos, checks);

                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);

                wh_iter_peek(it, reinterpret_cast<void *>(res_key), sizeof(res_key), &res_size, 
                             reinterpret_cast<void *>(&store), sizeof(typename Diva<O>::InfixStore), &dummy);
                REQUIRE(store.IsPartialKey());
                REQUIRE_EQ(store.GetInvalidBits(), 7);
                AssertStoreContents(s, store, occupieds_pos, checks);

                wh_iter_destroy(it);
            }
        }

        // Split an extension of a partial boundary key
        uint8_t old_boundary [sizeof(uint64_t)];
        uint32_t old_boundary_size;
        if constexpr (O) {
            wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
            wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);
            uint32_t dummy;
            typename Diva<O>::InfixStore store;
            wh_int_iter_peek(it, reinterpret_cast<void *>(old_boundary), sizeof(old_boundary), &old_boundary_size, 
                             reinterpret_cast<void *>(&store), sizeof(typename Diva<O>::InfixStore), &dummy);
            wh_int_iter_destroy(it);
        }
        else {
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);
            uint32_t dummy;
            typename Diva<O>::InfixStore store;
            wh_iter_peek(it, reinterpret_cast<void *>(old_boundary), sizeof(old_boundary), &old_boundary_size, 
                         reinterpret_cast<void *>(&store), sizeof(typename Diva<O>::InfixStore), &dummy);
            wh_iter_destroy(it);
        }
        uint32_t extended_key_len = old_boundary_size + 1;
        uint8_t extended_key[extended_key_len];
        memcpy(extended_key, old_boundary, extended_key_len);
        extended_key[extended_key_len - 1] = 1;
        s.InsertSplit({extended_key, extended_key_len});

        SUBCASE("split infix store using an extension of a partial boundary key") {
            const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 40, 62, 84, 105, 127, 149, 171, 193, 215, 237, 258, 280, 302, 324, 346, 368, 379, 389, 411, 433, 455, 477, 499, 520, 542, 564, 586, 608, 630};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b10001},   {  1,1,0b10111},   {  2,0,0b00100},   {  3,1,0b10100},   {  4,0,0b00100},   {  5,1,0b10100},   {  6,0,0b00100},   {  7,1,0b10100},   {  8,0,0b00100},   {  9,1,0b10100},   { 10,0,0b00100},   { 11,1,0b10100},   { 12,0,0b00100},   { 13,1,0b10100},   { 14,0,0b00100},   { 15,1,0b10100},   { 16,0,0b00100},   { 17,1,0b10100},   { 18,0,0b00100},   { 19,1,0b10100},   { 20,0,0b00100},   { 21,1,0b10100},   { 22,0,0b00100},   { 23,1,0b10100},   { 24,0,0b00100},   { 25,1,0b10100},   { 26,0,0b00100},   { 27,1,0b10100},   { 28,0,0b00100},   { 29,1,0b10100},   { 30,0,0b00100},   { 31,1,0b10100},   { 32,0,0b00100},   { 33,1,0b10100},   { 34,0,0b00100},   { 35,1,0b10100},   { 36,0,0b00100},   { 37,0,0b10100},   { 38,1,0b10100},   { 39,0,0b00100},   { 40,1,0b10100},   { 41,0,0b00100},   { 42,1,0b10100},   { 43,1,0b00100},   { 44,1,0b01100},   { 45,1,0b00100},   { 46,1,0b00100},   { 47,1,0b11100},   { 48,1,0b11100},   { 49,1,0b10100},   { 50,1,0b01100},   { 51,1,0b01100},   { 52,1,0b00100},   { 53,1,0b00100},   { 54,1,0b11100},   { 55,1,0b10100},   { 56,1,0b10100},   { 57,1,0b01100},   { 58,1,0b00100},   { 59,1,0b00100},   { 60,1,0b00100},   { 61,1,0b11100},   { 62,1,0b11100},   { 63,1,0b10100},   { 64,1,0b01100},   { 65,1,0b01100},   { 66,1,0b00100},   { 67,1,0b11100},   { 68,1,0b11100},   { 69,1,0b10100},   { 70,1,0b10100},   { 73,1,0b01100},   { 76,1,0b00100}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore *store;

            uint64_t value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (8ULL << shamt);
            value &= ~BITMASK(shamt);
            value = to_big_endian_order(value);
            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);
                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                REQUIRE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 7);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                REQUIRE_EQ(old_boundary_size, res_size);
                REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
                wh_int_iter_skip1(it);
                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                const uint64_t expected_next_boundary_key = 0x0000000022222222ULL;
                uint64_t current_key = 0;
                memcpy(&current_key, res_key, res_size);
                REQUIRE_EQ(__bswap_64(current_key), expected_next_boundary_key);

                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - shamt / 8);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                REQUIRE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 7);
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
        }

        value = (0x0000000011111111ULL * 30 + 0x0000000022222222ULL * 70) / 100 + (16ULL << shamt);
        value = to_big_endian_order(value);
        s.InsertSplit({reinterpret_cast<uint8_t *>(&value), sizeof(value)});

        SUBCASE("split infix store, create void infixes") {
            const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3, 4, 5, 6, 7, 24, 25, 26, 27, 28, 29, 30, 31, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 478, 479};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,1,0b10000},   {  1,1,0b10000},   {  2,1,0b10000},   {  3,1,0b10000},   {  4,1,0b10000},   {  5,1,0b10000},   {  6,1,0b10000},   {  7,1,0b10000},   { 11,1,0b10000},   { 12,1,0b10000},   { 13,1,0b10000},   { 14,1,0b10000},   { 15,1,0b10000},   { 16,1,0b10000},   { 17,1,0b10000},   { 18,1,0b10000},   { 31,1,0b10000},   { 32,1,0b10000},   { 33,1,0b10000},   { 34,1,0b10000},   { 35,1,0b10000},   { 36,1,0b10000},   { 37,1,0b10000},   { 38,1,0b10000},   { 39,1,0b10000},   { 40,1,0b10000},   { 41,1,0b10000},   { 42,1,0b10000},   { 43,1,0b10000},   { 44,1,0b10000},   { 45,1,0b10000},   { 46,1,0b10000},   { 47,1,0b10000},   { 48,1,0b10000},   { 49,1,0b10000},   { 50,1,0b10000},   { 51,1,0b10000},   { 52,1,0b10000},   { 53,1,0b10000},   { 54,1,0b10000},   { 55,1,0b10000},   { 56,1,0b10000},   { 57,1,0b10000},   { 58,1,0b10000},   { 59,1,0b10000},   { 60,1,0b10000},   { 61,1,0b10000},   { 62,1,0b10000},   { 63,1,0b10000},   { 64,1,0b10000},   { 65,1,0b10000},   { 66,1,0b10000},   { 67,1,0b10000},   { 68,1,0b10000},   { 69,1,0b10000},   { 70,1,0b10000},   { 71,1,0b10000},   { 72,1,0b10000},   { 73,1,0b10000},   { 74,1,0b10000},   { 75,1,0b10000},   { 76,1,0b10000},   { 77,1,0b10000},   { 78,1,0b10000},   { 79,1,0b10000},   { 80,1,0b10000},   { 81,1,0b10000},   { 82,1,0b10000},   { 83,1,0b10000},   { 84,1,0b10000},   { 85,1,0b10000},   { 86,1,0b10000},   { 87,1,0b10000},   { 88,1,0b10000},   { 89,1,0b10000},   { 90,1,0b10000},   { 91,1,0b10000},   { 92,1,0b10000},   { 93,1,0b10000},   { 94,1,0b10000},   { 95,1,0b10000},   { 96,1,0b10000},   { 97,1,0b10000},   { 98,1,0b10000},   { 99,1,0b10000},   {100,1,0b10000},   {101,1,0b10000},   {102,1,0b10000},   {103,1,0b10000},   {104,1,0b10000},   {105,1,0b10000},   {106,1,0b10000},   {107,1,0b10000},   {108,1,0b10000},   {109,1,0b10000},   {110,1,0b10000},   {111,1,0b10000},   {112,1,0b10000},   {113,1,0b10000},   {114,1,0b10000},   {115,1,0b10000},   {116,1,0b10000},   {117,1,0b10000},   {118,1,0b10000},   {119,1,0b10000},   {120,1,0b10000},   {121,1,0b10000},   {122,1,0b10000},   {123,1,0b10000},   {124,1,0b10000},   {125,1,0b10000},   {126,1,0b10000},   {127,1,0b10000},   {128,1,0b10000},   {129,1,0b10000},   {130,1,0b10000},   {131,1,0b10000},   {132,1,0b10000},   {133,1,0b10000},   {134,1,0b10000},   {135,1,0b10000},   {136,1,0b10000},   {137,1,0b10000},   {138,1,0b10000},   {139,1,0b10000},   {140,1,0b10000},   {141,1,0b10000},   {142,1,0b10000},   {143,1,0b10000},   {144,1,0b10000},   {145,1,0b10000},   {146,1,0b10000},   {147,1,0b10000},   {148,1,0b10000},   {149,1,0b10000},   {150,1,0b10000},   {151,1,0b10000},   {152,1,0b10000},   {153,1,0b10000},   {154,1,0b10000},   {155,1,0b10000},   {156,1,0b10000},   {157,1,0b10000},   {158,1,0b10000},   {159,1,0b10000},   {160,1,0b10000},   {161,1,0b10000},   {162,1,0b10000},   {163,1,0b10000},   {164,1,0b10000},   {165,1,0b10000},   {166,1,0b10000},   {167,1,0b10000},   {168,1,0b10000},   {169,1,0b10000},   {170,1,0b10000},   {171,1,0b10000},   {172,1,0b10000},   {173,1,0b10000},   {174,1,0b10000},   {175,1,0b10000},   {176,1,0b10000},   {177,1,0b10000},   {178,1,0b10000},   {179,1,0b10000},   {180,1,0b10000},   {181,1,0b10000},   {182,1,0b10000},   {183,1,0b10000},   {184,1,0b10000},   {185,1,0b10000},   {186,1,0b10000},   {187,1,0b10000},   {188,1,0b10000},   {189,1,0b10000},   {190,1,0b10000},   {191,1,0b10000},   {192,1,0b10000},   {193,1,0b10000},   {194,1,0b10000},   {195,1,0b10000},   {196,1,0b10000},   {197,1,0b10000},   {198,1,0b10000},   {199,1,0b10000},   {200,1,0b10000},   {201,1,0b10000},   {202,1,0b10000},   {203,1,0b10000},   {204,1,0b10000},   {205,1,0b10000},   {206,1,0b10000},   {207,1,0b10000},   {208,1,0b10000},   {209,1,0b10000},   {210,1,0b10000},   {211,1,0b10000},   {212,1,0b10000},   {213,1,0b10000},   {214,1,0b10000},   {215,1,0b10000},   {216,1,0b10000},   {217,1,0b10000},   {218,1,0b10000},   {219,1,0b10000},   {220,1,0b10000},   {221,1,0b10000},   {222,1,0b10000},   {223,1,0b10000},   {224,1,0b10000},   {225,1,0b10000},   {226,1,0b10000},   {227,1,0b10000},   {228,1,0b10000},   {229,1,0b10000},   {230,1,0b10000},   {231,1,0b10000},   {232,1,0b10000},   {233,1,0b10000},   {234,1,0b10000},   {235,1,0b10000},   {236,1,0b10000},   {237,1,0b10000},   {238,1,0b10000},   {239,1,0b10000},   {240,1,0b10000},   {241,1,0b10000},   {242,1,0b10000},   {243,1,0b10000},   {244,1,0b10000},   {245,1,0b10000},   {246,1,0b10000},   {247,1,0b10000},   {248,1,0b10000},   {249,1,0b10000},   {250,1,0b10000},   {251,1,0b10000},   {252,1,0b10000},   {253,1,0b10000},   {254,1,0b10000}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore *store;

            uint64_t value = to_big_endian_order(0b0000000000000000000000000000000000011101000100110000000000000000UL);
            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - 2);
                wh_int_iter_skip1_rev(it);

                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                REQUIRE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 7);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                REQUIRE_EQ(old_boundary_size, res_size);
                REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
                wh_int_iter_skip1(it);
                wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);
                REQUIRE_EQ(sizeof(value) - 1, res_size);
                REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value) - 2), 0);

                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value) - 2);
                wh_iter_skip1_rev(it);

                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                REQUIRE(store->IsPartialKey());
                REQUIRE_EQ(store->GetInvalidBits(), 7);
                AssertStoreContents(s, *store, occupieds_pos, checks);

                REQUIRE_EQ(old_boundary_size, res_size);
                REQUIRE_EQ(memcmp(old_boundary, res_key, old_boundary_size), 0);
                wh_iter_skip1(it);
                wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                 reinterpret_cast<void **>(&store), &dummy);
                REQUIRE_EQ(sizeof(value) - 1, res_size);
                REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value) - 2), 0);

                wh_iter_destroy(it);
            }
        }

        if constexpr (O)
            return;

        uint8_t new_extended_key[12];
        uint32_t new_extended_key_len;
        {
            uint64_t value = to_big_endian_order(0x0000000033333333UL);
            wormhole_iter *it = wh_iter_create(s.better_tree_);
            wh_iter_seek(it, reinterpret_cast<void *>(&value), sizeof(value));
            typename Diva<O>::InfixStore *store;
            uint32_t dummy;
            wh_iter_peek(it, reinterpret_cast<void *>(new_extended_key), sizeof(new_extended_key), &new_extended_key_len, 
                         reinterpret_cast<void *>(&store), sizeof(typename Diva<O>::InfixStore), &dummy);
            wh_iter_destroy(it);
        }
        memset(new_extended_key + new_extended_key_len, 0, 3);
        new_extended_key_len += 3;
        new_extended_key[new_extended_key_len - 1] = 1;
        s.InsertSplit({new_extended_key, new_extended_key_len});

        SUBCASE("split infix store using an extension of a full boundary key: left half") {
            const std::vector<uint32_t> occupieds_pos = {};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore *store;

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
            const std::vector<uint32_t> occupieds_pos = {410};
            const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{ 48,1,0b00001}};
            const uint8_t *res_key;
            uint32_t res_size, dummy;
            typename Diva<O>::InfixStore *store;

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


    template <bool O>
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
        if constexpr (!O) {
            for (int32_t i = 0; i < init_n; i++)
                init_keys[i] = to_big_endian_order(init_keys[i]);
        }
        Diva<O> s(infix_size, init_keys.begin(), init_keys.end(), sizeof(uint64_t),
                      seed, load_factor);

        const uint32_t extra_n = 1400000;
        for (int32_t i = 0; i < extra_n; i++) {
            const uint64_t key = to_big_endian_order(rng());
            s.Insert(reinterpret_cast<const uint8_t *>(&key), sizeof(key));
        }
    }


    template <bool O>
    static void PointQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<O> s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
            partial_keys.insert(0b0000000000000000000000000000000000011101000010110100000000000000UL);

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


    template <bool O>
    static void RangeQuery() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<O> s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
            partial_keys.insert(0b0000000000000000000000000000000000011101000010110100000000000000UL);

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


    template <bool O>
    static void Delete() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<O> s(infix_size, seed, load_factor);

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
                typename Diva<O>::InfixStore *store;
                uint32_t dummy;

                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'001010101'01011,
                    0b0'011011011'00100, 0b0'011011011'00001,
                    0b1'001010101'01000, 0b1'001010101'01011,
                    0b1'011011011'00001};
                std::vector<uint64_t> right_store_infixes {0b1'001010101'10000,
                    0b1'001010101'01011, 0b1'001111111'11001,
                    0b1'011111100'01000, 0b1'011111100'00111};
                if constexpr (O) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_int_iter_skip1(it);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                    s.DeleteMerge(it);

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        const std::vector<uint32_t> occupieds_pos = {0, 1, 299, 320, 383};
                        const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b11001},   {  1,0,0b11011},   {  2,1,0b11011},   {  3,0,0b00001},   {  4,0,0b00001},   {  5,1,0b00011},   { 37,0,0b11000},   { 38,1,0b10101},   { 40,1,0b11101},   { 47,0,0b00100},   { 48,1,0b00011}};
                        const uint8_t *res_key;
                        uint32_t res_size, dummy;
                        typename Diva<O>::InfixStore *store;

                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_FALSE(store->IsPartialKey());
                        REQUIRE_EQ(store->GetInvalidBits(), 0);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_int_iter_skip1(it);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_int_iter_destroy(it);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_iter_skip1(it);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                    s.DeleteMerge(it);

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        const std::vector<uint32_t> occupieds_pos = {0, 1, 299, 320, 383};
                        const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b11001},   {  1,0,0b11011},   {  2,1,0b11011},   {  3,0,0b00001},   {  4,0,0b00001},   {  5,1,0b00011},   { 37,0,0b11000},   { 38,1,0b10101},   { 40,1,0b11101},   { 47,0,0b00100},   { 48,1,0b00011}};
                        const uint8_t *res_key;
                        uint32_t res_size, dummy;
                        typename Diva<O>::InfixStore *store;

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
                typename Diva<O>::InfixStore *store;
                uint32_t dummy;

                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'000000000'10100,
                    0b0'000000000'10101, 0b0'000000001'10111,
                    0b0'000000010'10000, 0b0'000000010'00001,
                    0b0'000000011'10111, 0b1'000000000'00001};
                std::vector<uint64_t> right_store_infixes {0b0'010100000'11111,
                    0b0'011110101'01000, 0b0'011110101'00001,
                    0b1'001001011'01011};
                if constexpr (O) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_int_iter_skip1(it);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                    s.DeleteMerge(it);

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3, 512, 513, 514, 516};
                        const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b10100},   {  1,1,0b10101},   {  2,1,0b10111},   {  3,0,0b10000},   {  4,1,0b00001},   {  5,1,0b10111},   { 59,1,0b00001},   { 60,1,0b01101},   { 61,0,0b00011},   { 62,1,0b00011},   { 63,1,0b10111}};
                        const uint8_t *res_key;
                        uint32_t res_size, dummy;
                        typename Diva<O>::InfixStore *store;

                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_FALSE(store->IsPartialKey());
                        REQUIRE_EQ(store->GetInvalidBits(), 0);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_int_iter_skip1(it);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_int_iter_destroy(it);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_iter_skip1(it);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                    s.DeleteMerge(it);

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        const std::vector<uint32_t> occupieds_pos = {0, 1, 2, 3, 512, 513, 514, 516};
                        const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b10100},   {  1,1,0b10101},   {  2,1,0b10111},   {  3,0,0b10000},   {  4,1,0b00001},   {  5,1,0b10111},   { 59,1,0b00001},   { 60,1,0b01101},   { 61,0,0b00011},   { 62,1,0b00011},   { 63,1,0b10111}};
                        const uint8_t *res_key;
                        uint32_t res_size, dummy;
                        typename Diva<O>::InfixStore *store;

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
                typename Diva<O>::InfixStore *store;
                uint32_t dummy;

                const uint64_t total_implicit = 0b1'111111111 - 0b0'000000000 + 1;
                std::vector<uint64_t> left_store_infixes {0b0'000000000'10100,
                    0b0'000000000'10101, 0b0'000000001'10111,
                    0b0'000000010'10000, 0b0'000000010'00001,
                    0b0'000000011'10111, 0b1'000000000'00001};
                std::vector<uint64_t> right_store_infixes {0b0'010100000'11111,
                    0b0'011110101'01000, 0b0'011110101'00001,
                    0b1'001001011'01011};
                if constexpr (O) {
                    wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_int_iter_skip1(it);
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                    s.DeleteMerge(it);

                    it = wh_int_iter_create(s.better_tree_int_);
                    wh_int_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                         reinterpret_cast<void **>(&store), &dummy);
                    {
                        const std::vector<uint32_t> occupieds_pos = {0, 160, 245, 587};
                        const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b00001},   {  1,0,0b00001},   {  2,0,0b00001},   {  3,0,0b00001},   {  4,0,0b00001},   {  5,0,0b00001},   {  6,1,0b00001},   { 10,1,0b11111},   { 15,0,0b01000},   { 16,1,0b00001},   { 36,1,0b01011}};
                        const uint8_t *res_key;
                        uint32_t res_size, dummy;
                        typename Diva<O>::InfixStore *store;

                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_FALSE(store->IsPartialKey());
                        REQUIRE_EQ(store->GetInvalidBits(), 0);
                        AssertStoreContents(s, *store, occupieds_pos, checks);

                        uint64_t value = to_big_endian_order(boundary_keys[0]);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                        wh_int_iter_skip1(it);
                        value = to_big_endian_order(boundary_keys[2]);
                        wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                             reinterpret_cast<void **>(&store), &dummy);
                        REQUIRE_EQ(sizeof(value), res_size);
                        REQUIRE_EQ(memcmp(reinterpret_cast<uint8_t *>(&value), res_key, sizeof(value)), 0);
                    }
                    wh_int_iter_destroy(it);
                }
                else {
                    wormhole_iter *it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, left_store_infixes.data(), left_store_infixes.size(), total_implicit);

                    wh_iter_skip1(it);
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    s.LoadListToInfixStore(*store, right_store_infixes.data(), right_store_infixes.size(), total_implicit);

                    s.DeleteMerge(it);

                    it = wh_iter_create(s.better_tree_);
                    wh_iter_seek(it, reinterpret_cast<const void *>(&value), sizeof(value));
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&key_ptr), &key_len,
                                     reinterpret_cast<void **>(&store), &dummy);
                    {
                        const std::vector<uint32_t> occupieds_pos = {0, 160, 245, 587};
                        const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b00001},   {  1,0,0b00001},   {  2,0,0b00001},   {  3,0,0b00001},   {  4,0,0b00001},   {  5,0,0b00001},   {  6,1,0b00001},   { 10,1,0b11111},   { 15,0,0b01000},   { 16,1,0b00001},   { 36,1,0b01011}};
                        const uint8_t *res_key;
                        uint32_t res_size, dummy;
                        typename Diva<O>::InfixStore *store;

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
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

                const uint64_t l = 0x0000000011111111ULL, r = 0x0000000022222222ULL;
                const uint64_t interp = (l * i + r * (100 - i)) / 100;
                const uint64_t value = to_big_endian_order(interp);
                s.InsertSimple({reinterpret_cast<const uint8_t *>(&value), sizeof(value)});
                keys.emplace_back(interp, bits_to_zero_out);
            }

            for (int32_t i = 90; i >= 70; i -= 2) {
                const uint32_t shared = 34;
                const uint32_t ignore = 1;
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
                const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
                    const uint64_t r = 0b00011101000010110111111111111111UL;
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


    template <bool O>
    static void ShrinkInfixSize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        Diva<O> s(infix_size, seed, load_factor);

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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
            const uint32_t bits_to_zero_out = sizeof(uint64_t) * 8 - shared - ignore - Diva<O>::base_implicit_size - s.infix_size_;

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

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, nullptr, 0);
                {
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    const uint64_t value = 0;
                    REQUIRE_EQ(res_size, sizeof(value));
                    REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }
                wh_int_iter_skip1(it);
                {
                    const std::vector<uint32_t> occupieds_pos = {5, 11, 16, 21, 27, 32, 38, 43, 49, 54, 60, 65, 71, 76, 82, 87, 92, 98, 103, 109, 114, 120, 125, 131, 136, 142, 147, 153, 158, 163, 169, 174, 180, 185, 191, 196, 202, 207, 213, 218, 224, 229, 234, 240, 245, 251, 256, 262, 267, 273, 278, 284, 289, 295, 300, 305, 311, 316, 322, 327, 333, 338, 344, 349, 355, 360, 366, 371, 376, 382, 383, 384, 385, 386, 387, 388, 393, 398, 404, 409, 415, 420, 426, 431, 437, 442, 447, 453, 458, 464, 469, 475, 480, 486, 491, 497, 502, 508, 513, 518, 524, 529, 535, 540};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  9,1,0b1001},   { 21,1,0b0001},   { 31,1,0b1001},   { 41,1,0b1111},   { 53,1,0b0111},   { 63,1,0b1111},   { 74,1,0b0101},   { 84,1,0b1101},   { 96,1,0b0101},   {106,0,0b1011},   {107,1,0b1011},   {118,1,0b0011},   {128,0,0b1011},   {129,1,0b1011},   {139,1,0b0011},   {149,0,0b1001},   {150,1,0b1001},   {161,1,0b0001},   {171,0,0b1001},   {172,1,0b1001},   {181,1,0b1111},   {193,0,0b0111},   {194,1,0b0111},   {202,1,0b1111},   {214,0,0b0101},   {215,1,0b0101},   {224,1,0b1101},   {236,0,0b0101},   {237,1,0b0101},   {246,1,0b1011},   {258,0,0b0011},   {259,1,0b0011},   {267,1,0b1011},   {279,0,0b0011},   {280,1,0b0011},   {289,1,0b1001},   {301,0,0b0001},   {302,1,0b0001},   {311,1,0b1001},   {321,0,0b1111},   {322,1,0b1111},   {333,1,0b0111},   {342,1,0b1111},   {354,1,0b0101},   {364,1,0b1101},   {376,1,0b0101},   {386,1,0b1011},   {398,1,0b0011},   {407,1,0b1011},   {419,1,0b0011},   {429,1,0b1001},   {441,1,0b0001},   {451,1,0b1001},   {461,1,0b1111},   {472,1,0b0111},   {482,1,0b1111},   {494,1,0b0101},   {504,1,0b1101},   {516,1,0b0101},   {526,1,0b1011},   {537,1,0b0011},   {547,1,0b1011},   {559,1,0b0001},   {569,1,0b1001},   {581,1,0b0001},   {591,1,0b1001},   {601,1,0b1111},   {612,1,0b0111},   {622,1,0b1111},   {634,1,0b0101},   {644,1,0b1101},   {656,1,0b0101},   {666,1,0b1011},   {677,1,0b0011},   {687,1,0b1011},   {699,1,0b0001},   {709,1,0b1001},   {721,1,0b0001},   {731,1,0b1001},   {740,1,0b1111},   {752,0,0b0111},   {753,0,0b1001},   {754,0,0b1011},   {755,0,0b1101},   {756,1,0b1111},   {757,0,0b0001},   {758,0,0b0011},   {759,0,0b0101},   {760,0,0b0111},   {761,0,0b1001},   {762,0,0b1011},   {763,0,0b1101},   {764,1,0b1111},   {765,0,0b0001},   {766,0,0b0011},   {767,0,0b0101},   {768,0,0b0111},   {769,0,0b1001},   {770,0,0b1011},   {771,0,0b1101},   {772,1,0b1111},   {773,0,0b0001},   {774,0,0b0011},   {775,0,0b0101},   {776,0,0b0111},   {777,0,0b1001},   {778,0,0b1011},   {779,0,0b1101},   {780,1,0b1111},   {781,0,0b0001},   {782,0,0b0011},   {783,0,0b0101},   {784,0,0b0111},   {785,0,0b1001},   {786,0,0b1011},   {787,0,0b1101},   {788,1,0b1111},   {789,0,0b0001},   {790,0,0b0011},   {791,0,0b0101},   {792,0,0b0111},   {793,0,0b1001},   {794,0,0b1011},   {795,0,0b1101},   {796,0,0b1111},   {797,1,0b1111},   {798,0,0b0001},   {799,0,0b0011},   {800,0,0b0101},   {801,0,0b0111},   {802,1,0b1001},   {803,1,0b0101},   {804,1,0b1101},   {805,1,0b0101},   {806,1,0b1011},   {817,1,0b0011},   {827,1,0b1011},   {839,1,0b0001},   {849,1,0b1001},   {861,1,0b0001},   {870,1,0b1001},   {880,1,0b1111},   {892,1,0b0111},   {902,1,0b1111},   {914,1,0b0101},   {924,1,0b1101},   {935,1,0b0101},   {945,1,0b1011},   {957,1,0b0011},   {967,1,0b1011},   {979,1,0b0001},   {989,1,0b1001},   {1001,1,0b0001},   {1010,1,0b0111},   {1020,1,0b1111},   {1032,1,0b0111},   {1042,1,0b1111},   {1054,1,0b0101},   {1064,1,0b1101}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    const uint64_t value = to_big_endian_order(0b00010001000100010001000100010001UL);
                    REQUIRE_EQ(res_size, sizeof(value));
                    REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }
                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, nullptr, 0);
                {
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
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
                    const std::vector<uint32_t> occupieds_pos = {5, 11, 16, 21, 27, 32, 38, 43, 49, 54, 60, 65, 71, 76, 82, 87, 92, 98, 103, 109, 114, 120, 125, 131, 136, 142, 147, 153, 158, 163, 169, 174, 180, 185, 191, 196, 202, 207, 213, 218, 224, 229, 234, 240, 245, 251, 256, 262, 267, 273, 278, 284, 289, 295, 300, 305, 311, 316, 322, 327, 333, 338, 344, 349, 355, 360, 366, 371, 376, 382, 383, 384, 385, 386, 387, 388, 393, 398, 404, 409, 415, 420, 426, 431, 437, 442, 447, 453, 458, 464, 469, 475, 480, 486, 491, 497, 502, 508, 513, 518, 524, 529, 535, 540};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  9,1,0b1001},   { 21,1,0b0001},   { 31,1,0b1001},   { 41,1,0b1111},   { 53,1,0b0111},   { 63,1,0b1111},   { 74,1,0b0101},   { 84,1,0b1101},   { 96,1,0b0101},   {106,0,0b1011},   {107,1,0b1011},   {118,1,0b0011},   {128,0,0b1011},   {129,1,0b1011},   {139,1,0b0011},   {149,0,0b1001},   {150,1,0b1001},   {161,1,0b0001},   {171,0,0b1001},   {172,1,0b1001},   {181,1,0b1111},   {193,0,0b0111},   {194,1,0b0111},   {202,1,0b1111},   {214,0,0b0101},   {215,1,0b0101},   {224,1,0b1101},   {236,0,0b0101},   {237,1,0b0101},   {246,1,0b1011},   {258,0,0b0011},   {259,1,0b0011},   {267,1,0b1011},   {279,0,0b0011},   {280,1,0b0011},   {289,1,0b1001},   {301,0,0b0001},   {302,1,0b0001},   {311,1,0b1001},   {321,0,0b1111},   {322,1,0b1111},   {333,1,0b0111},   {342,1,0b1111},   {354,1,0b0101},   {364,1,0b1101},   {376,1,0b0101},   {386,1,0b1011},   {398,1,0b0011},   {407,1,0b1011},   {419,1,0b0011},   {429,1,0b1001},   {441,1,0b0001},   {451,1,0b1001},   {461,1,0b1111},   {472,1,0b0111},   {482,1,0b1111},   {494,1,0b0101},   {504,1,0b1101},   {516,1,0b0101},   {526,1,0b1011},   {537,1,0b0011},   {547,1,0b1011},   {559,1,0b0001},   {569,1,0b1001},   {581,1,0b0001},   {591,1,0b1001},   {601,1,0b1111},   {612,1,0b0111},   {622,1,0b1111},   {634,1,0b0101},   {644,1,0b1101},   {656,1,0b0101},   {666,1,0b1011},   {677,1,0b0011},   {687,1,0b1011},   {699,1,0b0001},   {709,1,0b1001},   {721,1,0b0001},   {731,1,0b1001},   {740,1,0b1111},   {752,0,0b0111},   {753,0,0b1001},   {754,0,0b1011},   {755,0,0b1101},   {756,1,0b1111},   {757,0,0b0001},   {758,0,0b0011},   {759,0,0b0101},   {760,0,0b0111},   {761,0,0b1001},   {762,0,0b1011},   {763,0,0b1101},   {764,1,0b1111},   {765,0,0b0001},   {766,0,0b0011},   {767,0,0b0101},   {768,0,0b0111},   {769,0,0b1001},   {770,0,0b1011},   {771,0,0b1101},   {772,1,0b1111},   {773,0,0b0001},   {774,0,0b0011},   {775,0,0b0101},   {776,0,0b0111},   {777,0,0b1001},   {778,0,0b1011},   {779,0,0b1101},   {780,1,0b1111},   {781,0,0b0001},   {782,0,0b0011},   {783,0,0b0101},   {784,0,0b0111},   {785,0,0b1001},   {786,0,0b1011},   {787,0,0b1101},   {788,1,0b1111},   {789,0,0b0001},   {790,0,0b0011},   {791,0,0b0101},   {792,0,0b0111},   {793,0,0b1001},   {794,0,0b1011},   {795,0,0b1101},   {796,0,0b1111},   {797,1,0b1111},   {798,0,0b0001},   {799,0,0b0011},   {800,0,0b0101},   {801,0,0b0111},   {802,1,0b1001},   {803,1,0b0101},   {804,1,0b1101},   {805,1,0b0101},   {806,1,0b1011},   {817,1,0b0011},   {827,1,0b1011},   {839,1,0b0001},   {849,1,0b1001},   {861,1,0b0001},   {870,1,0b1001},   {880,1,0b1111},   {892,1,0b0111},   {902,1,0b1111},   {914,1,0b0101},   {924,1,0b1101},   {935,1,0b0101},   {945,1,0b1011},   {957,1,0b0011},   {967,1,0b1011},   {979,1,0b0001},   {989,1,0b1001},   {1001,1,0b0001},   {1010,1,0b0111},   {1020,1,0b1111},   {1032,1,0b0111},   {1042,1,0b1111},   {1054,1,0b0101},   {1064,1,0b1101}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
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
        }

        SUBCASE("shrink by two") {
            s.ShrinkInfixSize(infix_size - 2);
            REQUIRE_EQ(s.infix_size_, infix_size - 2);

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, nullptr, 0);
                {
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    const uint64_t value = 0;
                    REQUIRE_EQ(res_size, sizeof(value));
                    REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }
                wh_int_iter_skip1(it);
                {
                    const std::vector<uint32_t> occupieds_pos = {5, 11, 16, 21, 27, 32, 38, 43, 49, 54, 60, 65, 71, 76, 82, 87, 92, 98, 103, 109, 114, 120, 125, 131, 136, 142, 147, 153, 158, 163, 169, 174, 180, 185, 191, 196, 202, 207, 213, 218, 224, 229, 234, 240, 245, 251, 256, 262, 267, 273, 278, 284, 289, 295, 300, 305, 311, 316, 322, 327, 333, 338, 344, 349, 355, 360, 366, 371, 376, 382, 383, 384, 385, 386, 387, 388, 393, 398, 404, 409, 415, 420, 426, 431, 437, 442, 447, 453, 458, 464, 469, 475, 480, 486, 491, 497, 502, 508, 513, 518, 524, 529, 535, 540};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  9,1,0b101},   { 21,1,0b001},   { 31,1,0b101},   { 41,1,0b111},   { 53,1,0b011},   { 63,1,0b111},   { 74,1,0b011},   { 84,1,0b111},   { 96,1,0b011},   {106,0,0b101},   {107,1,0b101},   {118,1,0b001},   {128,0,0b101},   {129,1,0b101},   {139,1,0b001},   {149,0,0b101},   {150,1,0b101},   {161,1,0b001},   {171,0,0b101},   {172,1,0b101},   {181,1,0b111},   {193,0,0b011},   {194,1,0b011},   {202,1,0b111},   {214,0,0b011},   {215,1,0b011},   {224,1,0b111},   {236,0,0b011},   {237,1,0b011},   {246,1,0b101},   {258,0,0b001},   {259,1,0b001},   {267,1,0b101},   {279,0,0b001},   {280,1,0b001},   {289,1,0b101},   {301,0,0b001},   {302,1,0b001},   {311,1,0b101},   {321,0,0b111},   {322,1,0b111},   {333,1,0b011},   {342,1,0b111},   {354,1,0b011},   {364,1,0b111},   {376,1,0b011},   {386,1,0b101},   {398,1,0b001},   {407,1,0b101},   {419,1,0b001},   {429,1,0b101},   {441,1,0b001},   {451,1,0b101},   {461,1,0b111},   {472,1,0b011},   {482,1,0b111},   {494,1,0b011},   {504,1,0b111},   {516,1,0b011},   {526,1,0b101},   {537,1,0b001},   {547,1,0b101},   {559,1,0b001},   {569,1,0b101},   {581,1,0b001},   {591,1,0b101},   {601,1,0b111},   {612,1,0b011},   {622,1,0b111},   {634,1,0b011},   {644,1,0b111},   {656,1,0b011},   {666,1,0b101},   {677,1,0b001},   {687,1,0b101},   {699,1,0b001},   {709,1,0b101},   {721,1,0b001},   {731,1,0b101},   {740,1,0b111},   {752,0,0b011},   {753,0,0b101},   {754,0,0b101},   {755,0,0b111},   {756,1,0b111},   {757,0,0b001},   {758,0,0b001},   {759,0,0b011},   {760,0,0b011},   {761,0,0b101},   {762,0,0b101},   {763,0,0b111},   {764,1,0b111},   {765,0,0b001},   {766,0,0b001},   {767,0,0b011},   {768,0,0b011},   {769,0,0b101},   {770,0,0b101},   {771,0,0b111},   {772,1,0b111},   {773,0,0b001},   {774,0,0b001},   {775,0,0b011},   {776,0,0b011},   {777,0,0b101},   {778,0,0b101},   {779,0,0b111},   {780,1,0b111},   {781,0,0b001},   {782,0,0b001},   {783,0,0b011},   {784,0,0b011},   {785,0,0b101},   {786,0,0b101},   {787,0,0b111},   {788,1,0b111},   {789,0,0b001},   {790,0,0b001},   {791,0,0b011},   {792,0,0b011},   {793,0,0b101},   {794,0,0b101},   {795,0,0b111},   {796,0,0b111},   {797,1,0b111},   {798,0,0b001},   {799,0,0b001},   {800,0,0b011},   {801,0,0b011},   {802,1,0b101},   {803,1,0b011},   {804,1,0b111},   {805,1,0b011},   {806,1,0b101},   {817,1,0b001},   {827,1,0b101},   {839,1,0b001},   {849,1,0b101},   {861,1,0b001},   {870,1,0b101},   {880,1,0b111},   {892,1,0b011},   {902,1,0b111},   {914,1,0b011},   {924,1,0b111},   {935,1,0b011},   {945,1,0b101},   {957,1,0b001},   {967,1,0b101},   {979,1,0b001},   {989,1,0b101},   {1001,1,0b001},   {1010,1,0b011},   {1020,1,0b111},   {1032,1,0b011},   {1042,1,0b111},   {1054,1,0b011},   {1064,1,0b111}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    const uint64_t value = to_big_endian_order(0b00010001000100010001000100010001UL);
                    REQUIRE_EQ(memcmp(reinterpret_cast<const uint8_t *>(&value), res_key, sizeof(value)), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }
                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, nullptr, 0);
                {
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
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
                    const std::vector<uint32_t> occupieds_pos = {5, 11, 16, 21, 27, 32, 38, 43, 49, 54, 60, 65, 71, 76, 82, 87, 92, 98, 103, 109, 114, 120, 125, 131, 136, 142, 147, 153, 158, 163, 169, 174, 180, 185, 191, 196, 202, 207, 213, 218, 224, 229, 234, 240, 245, 251, 256, 262, 267, 273, 278, 284, 289, 295, 300, 305, 311, 316, 322, 327, 333, 338, 344, 349, 355, 360, 366, 371, 376, 382, 383, 384, 385, 386, 387, 388, 393, 398, 404, 409, 415, 420, 426, 431, 437, 442, 447, 453, 458, 464, 469, 475, 480, 486, 491, 497, 502, 508, 513, 518, 524, 529, 535, 540};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  9,1,0b101},   { 21,1,0b001},   { 31,1,0b101},   { 41,1,0b111},   { 53,1,0b011},   { 63,1,0b111},   { 74,1,0b011},   { 84,1,0b111},   { 96,1,0b011},   {106,0,0b101},   {107,1,0b101},   {118,1,0b001},   {128,0,0b101},   {129,1,0b101},   {139,1,0b001},   {149,0,0b101},   {150,1,0b101},   {161,1,0b001},   {171,0,0b101},   {172,1,0b101},   {181,1,0b111},   {193,0,0b011},   {194,1,0b011},   {202,1,0b111},   {214,0,0b011},   {215,1,0b011},   {224,1,0b111},   {236,0,0b011},   {237,1,0b011},   {246,1,0b101},   {258,0,0b001},   {259,1,0b001},   {267,1,0b101},   {279,0,0b001},   {280,1,0b001},   {289,1,0b101},   {301,0,0b001},   {302,1,0b001},   {311,1,0b101},   {321,0,0b111},   {322,1,0b111},   {333,1,0b011},   {342,1,0b111},   {354,1,0b011},   {364,1,0b111},   {376,1,0b011},   {386,1,0b101},   {398,1,0b001},   {407,1,0b101},   {419,1,0b001},   {429,1,0b101},   {441,1,0b001},   {451,1,0b101},   {461,1,0b111},   {472,1,0b011},   {482,1,0b111},   {494,1,0b011},   {504,1,0b111},   {516,1,0b011},   {526,1,0b101},   {537,1,0b001},   {547,1,0b101},   {559,1,0b001},   {569,1,0b101},   {581,1,0b001},   {591,1,0b101},   {601,1,0b111},   {612,1,0b011},   {622,1,0b111},   {634,1,0b011},   {644,1,0b111},   {656,1,0b011},   {666,1,0b101},   {677,1,0b001},   {687,1,0b101},   {699,1,0b001},   {709,1,0b101},   {721,1,0b001},   {731,1,0b101},   {740,1,0b111},   {752,0,0b011},   {753,0,0b101},   {754,0,0b101},   {755,0,0b111},   {756,1,0b111},   {757,0,0b001},   {758,0,0b001},   {759,0,0b011},   {760,0,0b011},   {761,0,0b101},   {762,0,0b101},   {763,0,0b111},   {764,1,0b111},   {765,0,0b001},   {766,0,0b001},   {767,0,0b011},   {768,0,0b011},   {769,0,0b101},   {770,0,0b101},   {771,0,0b111},   {772,1,0b111},   {773,0,0b001},   {774,0,0b001},   {775,0,0b011},   {776,0,0b011},   {777,0,0b101},   {778,0,0b101},   {779,0,0b111},   {780,1,0b111},   {781,0,0b001},   {782,0,0b001},   {783,0,0b011},   {784,0,0b011},   {785,0,0b101},   {786,0,0b101},   {787,0,0b111},   {788,1,0b111},   {789,0,0b001},   {790,0,0b001},   {791,0,0b011},   {792,0,0b011},   {793,0,0b101},   {794,0,0b101},   {795,0,0b111},   {796,0,0b111},   {797,1,0b111},   {798,0,0b001},   {799,0,0b001},   {800,0,0b011},   {801,0,0b011},   {802,1,0b101},   {803,1,0b011},   {804,1,0b111},   {805,1,0b011},   {806,1,0b101},   {817,1,0b001},   {827,1,0b101},   {839,1,0b001},   {849,1,0b101},   {861,1,0b001},   {870,1,0b101},   {880,1,0b111},   {892,1,0b011},   {902,1,0b111},   {914,1,0b011},   {924,1,0b111},   {935,1,0b011},   {945,1,0b101},   {957,1,0b001},   {967,1,0b101},   {979,1,0b001},   {989,1,0b101},   {1001,1,0b001},   {1010,1,0b011},   {1020,1,0b111},   {1032,1,0b011},   {1042,1,0b111},   {1054,1,0b011},   {1064,1,0b111}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
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
    }


    template <bool O>
    static void BulkLoad() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t n_keys = 2600;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);
        SUBCASE("fixed length") {
            std::vector<uint64_t> keys;
            for (int32_t i = 0; i < n_keys; i++)
                keys.push_back(rng());
            std::sort(keys.begin(), keys.end());
            if constexpr (!O) {
                for (int32_t i = 0; i < n_keys; i++)
                    keys[i] = to_big_endian_order(keys[i]);
            }

            Diva<O> s(infix_size, keys.begin(), keys.end(), sizeof(uint64_t), seed, load_factor);

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, nullptr, 0);
                {
                    const uint8_t expected_boundary[] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_int_iter_skip1(it);
                {
                    const uint8_t expected_boundary[] = {0b00000000, 0b00010000, 0b00001100, 0b00111101, 0b11110111, 0b11101011, 0b00011100, 0b10100010};
                    const std::vector<uint32_t> occupieds_pos = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 20, 22, 23, 25, 27, 28, 29, 32, 34, 35, 36, 37, 38, 39, 40, 41, 42, 44, 45, 46, 47, 49, 51, 52, 54, 55, 56, 57, 59, 60, 61, 63, 64, 70, 71, 72, 73, 74, 75, 78, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 92, 94, 95, 98, 99, 100, 101, 102, 103, 104, 107, 108, 111, 113, 114, 115, 116, 117, 118, 120, 121, 122, 123, 125, 126, 127, 131, 132, 133, 134, 135, 137, 138, 139, 141, 144, 145, 146, 147, 148, 149, 151, 152, 153, 154, 155, 157, 159, 160, 162, 164, 165, 167, 168, 170, 174, 175, 176, 177, 179, 180, 181, 182, 184, 185, 186, 187, 189, 190, 192, 196, 197, 199, 201, 202, 203, 204, 205, 206, 207, 208, 210, 212, 213, 214, 216, 218, 220, 221, 222, 223, 225, 226, 228, 229, 230, 231, 234, 235, 237, 238, 239, 241, 242, 243, 244, 246, 248, 249, 250, 251, 253, 254, 255, 256, 257, 259, 261, 262, 263, 264, 265, 267, 268, 270, 271, 272, 274, 276, 277, 278, 280, 281, 282, 283, 285, 287, 288, 289, 291, 292, 293, 294, 296, 297, 298, 301, 302, 303, 304, 305, 308, 309, 310, 311, 313, 314, 315, 320, 321, 322, 323, 325, 326, 327, 328, 331, 332, 334, 335, 336, 338, 339, 340, 341, 342, 344, 345, 346, 348, 349, 351, 354, 355, 356, 357, 358, 359, 360, 363, 364, 365, 366, 367, 369, 371, 372, 373, 374, 375, 376, 378, 379, 381, 382, 383, 384, 385, 386, 388, 389, 390, 391, 394, 395, 396, 397, 398, 399, 400, 402, 403, 404, 406, 408, 409, 410, 411, 414, 415, 416, 417, 419, 420, 421, 422, 424, 428, 429, 430, 432, 433, 435, 437, 438, 439, 440, 441, 443, 444, 445, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 459, 460, 461, 462, 463, 464, 466, 467, 468, 469, 470, 471, 472, 473, 474, 476, 477, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 492, 494, 495, 496, 498, 499, 500, 501, 502, 503, 504, 506, 508, 509, 510, 511, 512, 513, 514, 516, 517, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 531, 532, 535, 537, 538, 540, 541, 542, 546, 548, 549, 550, 551, 552, 553, 554, 555, 557, 558, 559, 561, 562, 564, 565, 566, 568, 570, 572, 573, 574, 575, 578, 579, 580, 584, 585, 586, 587, 588, 589, 590, 591, 594, 595, 596, 597, 598, 599, 601, 602, 603, 604, 607, 608, 612, 613, 614, 616, 617, 618, 619, 620, 621, 623, 624, 625, 628, 629, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650, 651, 653, 655, 657, 658, 659, 660, 662, 664, 665, 666, 668, 669, 670, 672, 673, 674, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 689, 690, 691, 692, 693, 694, 697, 698, 700, 701, 702, 703, 705, 706, 707, 708, 709, 710, 711, 713, 714, 715, 718, 720, 721, 722, 724, 725, 727, 728, 730, 731, 732, 733, 736, 737, 738, 744, 745, 746, 747, 748, 750, 753, 754, 755, 756, 758, 759, 761, 762, 763, 765, 767, 768, 769, 770, 771, 772, 773, 774, 775, 777, 778, 779, 780, 781, 783, 784};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,0,0b00001},   {  2,0,0b01101},   {  3,0,0b10011},   {  4,0,0b10101},   {  5,1,0b11111},   {  6,1,0b11011},   {  7,0,0b10101},   {  8,1,0b10111},   {  9,1,0b01101},   { 10,0,0b00011},   { 11,0,0b10101},   { 12,1,0b11101},   { 13,1,0b00011},   { 14,0,0b00001},   { 15,0,0b00001},   { 16,1,0b01101},   { 17,1,0b10011},   { 18,0,0b10101},   { 19,1,0b11011},   { 20,1,0b00011},   { 21,1,0b01011},   { 22,0,0b00111},   { 23,0,0b01011},   { 24,0,0b11011},   { 25,1,0b11111},   { 26,0,0b00101},   { 27,1,0b11101},   { 28,1,0b01001},   { 29,1,0b10001},   { 30,1,0b10011},   { 31,1,0b11011},   { 32,0,0b00001},   { 33,0,0b00111},   { 34,0,0b11011},   { 35,0,0b11011},   { 36,1,0b11101},   { 37,1,0b00101},   { 38,1,0b11111},   { 39,0,0b01001},   { 40,0,0b10011},   { 41,0,0b10011},   { 42,1,0b11111},   { 43,1,0b11101},   { 44,0,0b01101},   { 45,0,0b01101},   { 46,1,0b11101},   { 47,1,0b01101},   { 48,0,0b00011},   { 49,0,0b11001},   { 50,1,0b11011},   { 51,1,0b01001},   { 52,1,0b00111},   { 53,0,0b00011},   { 54,1,0b00111},   { 55,1,0b10011},   { 56,0,0b00001},   { 57,1,0b11001},   { 58,0,0b00001},   { 59,0,0b00001},   { 60,1,0b01001},   { 61,0,0b00101},   { 62,0,0b10011},   { 63,1,0b10111},   { 64,1,0b00011},   { 65,1,0b00111},   { 66,0,0b10001},   { 67,1,0b10001},   { 68,1,0b01101},   { 69,0,0b00001},   { 70,0,0b01101},   { 71,1,0b11011},   { 72,0,0b01101},   { 73,0,0b10111},   { 74,0,0b11001},   { 75,1,0b11101},   { 76,0,0b00111},   { 77,0,0b00111},   { 78,0,0b01011},   { 79,1,0b11011},   { 80,0,0b01001},   { 81,0,0b01101},   { 82,0,0b01101},   { 83,1,0b11111},   { 84,1,0b11111},   { 85,0,0b10011},   { 86,0,0b11001},   { 87,1,0b11001},   { 88,1,0b00101},   { 89,0,0b00001},   { 90,1,0b00111},   { 91,1,0b01011},   { 92,0,0b01011},   { 93,0,0b11011},   { 94,1,0b11011},   { 95,1,0b10111},   { 96,0,0b01001},   { 97,1,0b10111},   { 98,0,0b10011},   { 99,1,0b11011},   {100,1,0b11001},   {101,1,0b00101},   {102,1,0b11001},   {103,1,0b01101},   {104,0,0b10111},   {105,1,0b11111},   {106,1,0b11001},   {109,0,0b00101},   {110,1,0b01111},   {111,1,0b01001},   {112,0,0b11101},   {113,1,0b11101},   {114,0,0b01111},   {115,0,0b10001},   {116,1,0b11101},   {117,0,0b00101},   {118,1,0b11001},   {119,1,0b11011},   {120,1,0b00111},   {121,0,0b01011},   {122,0,0b01011},   {123,0,0b01101},   {124,1,0b11111},   {125,1,0b10101},   {126,0,0b10011},   {127,1,0b10101},   {128,1,0b11001},   {129,0,0b00011},   {130,1,0b10111},   {131,1,0b01111},   {134,1,0b01011},   {135,1,0b00111},   {137,1,0b10111},   {138,0,0b01001},   {139,1,0b01111},   {140,1,0b00101},   {141,0,0b01001},   {142,1,0b01011},   {143,0,0b01111},   {144,1,0b10011},   {146,1,0b01011},   {148,0,0b00001},   {149,0,0b10011},   {150,1,0b11001},   {152,1,0b01011},   {154,0,0b10001},   {155,0,0b10101},   {156,1,0b10101},   {157,1,0b10011},   {158,1,0b11001},   {159,0,0b10001},   {160,0,0b10101},   {161,1,0b11111},   {162,0,0b00001},   {163,0,0b00111},   {164,1,0b01011},   {165,1,0b01011},   {166,1,0b11101},   {167,0,0b00111},   {168,1,0b01011},   {169,0,0b00101},   {170,1,0b10101},   {171,1,0b11101},   {172,0,0b00101},   {173,1,0b11001},   {174,0,0b00111},   {175,1,0b11101},   {176,0,0b00111},   {177,1,0b11001},   {179,0,0b01001},   {180,1,0b11011},   {181,0,0b00011},   {182,1,0b11111},   {183,1,0b11111},   {184,0,0b10111},   {185,1,0b10111},   {186,0,0b00101},   {187,1,0b11101},   {188,0,0b10001},   {189,0,0b10101},   {190,1,0b11111},   {191,0,0b01111},   {192,0,0b10111},   {193,1,0b11111},   {194,1,0b10111},   {195,1,0b01011},   {197,0,0b01011},   {198,1,0b10101},   {199,1,0b10111},   {200,0,0b10101},   {201,1,0b11001},   {202,1,0b00101},   {203,1,0b11011},   {204,1,0b01011},   {207,0,0b00001},   {208,0,0b01011},   {209,0,0b01011},   {210,1,0b01111},   {211,1,0b11111},   {212,1,0b10111},   {213,1,0b01111},   {214,1,0b11011},   {215,0,0b01011},   {216,1,0b01101},   {218,1,0b01001},   {219,0,0b11011},   {220,0,0b11011},   {221,1,0b11111},   {222,1,0b10101},   {224,1,0b01011},   {226,1,0b11111},   {229,1,0b11001},   {230,1,0b01011},   {233,0,0b01011},   {234,1,0b10111},   {238,1,0b01111},   {239,0,0b11101},   {240,0,0b11101},   {241,0,0b11111},   {242,1,0b11111},   {243,0,0b00001},   {244,0,0b01001},   {245,1,0b10101},   {246,1,0b01111},   {247,0,0b00011},   {248,1,0b11001},   {249,0,0b00011},   {250,0,0b00101},   {251,1,0b11011},   {252,0,0b00101},   {253,1,0b01111},   {254,0,0b00001},   {255,0,0b11001},   {256,1,0b11111},   {257,0,0b00001},   {258,0,0b01011},   {259,1,0b11101},   {260,1,0b01011},   {261,0,0b01101},   {262,1,0b01111},   {263,0,0b11011},   {264,1,0b11101},   {265,1,0b00011},   {266,0,0b00011},   {267,1,0b11011},   {268,0,0b00101},   {269,1,0b10001},   {270,1,0b11101},   {271,1,0b11001},   {272,0,0b01101},   {273,1,0b11001},   {275,1,0b10101},   {277,0,0b11001},   {278,1,0b11001},   {279,1,0b10101},   {280,0,0b00011},   {281,1,0b00101},   {282,1,0b00101},   {283,0,0b10001},   {284,1,0b11111},   {285,1,0b10101},   {286,1,0b10111},   {287,0,0b00101},   {288,1,0b01101},   {290,1,0b01011},   {292,1,0b01111},   {293,1,0b00111},   {296,0,0b01011},   {297,1,0b01101},   {298,1,0b11001},   {301,0,0b00001},   {302,1,0b10101},   {303,0,0b00001},   {304,1,0b01011},   {305,1,0b11001},   {306,1,0b01011},   {308,0,0b01001},   {309,1,0b10001},   {310,1,0b01001},   {312,0,0b01011},   {313,1,0b01011},   {314,0,0b01011},   {315,0,0b01011},   {316,1,0b01111},   {317,0,0b00011},   {318,1,0b11011},   {319,1,0b10011},   {320,0,0b10001},   {321,1,0b11001},   {322,1,0b00001},   {325,0,0b01101},   {326,0,0b01101},   {327,1,0b11001},   {328,0,0b01101},   {329,1,0b10111},   {330,0,0b00001},   {331,1,0b10011},   {332,0,0b00011},   {333,1,0b10111},   {334,1,0b11101},   {335,0,0b01111},   {336,0,0b10111},   {337,1,0b11001},   {338,1,0b10011},   {339,1,0b01101},   {340,1,0b01011},   {341,1,0b11111},   {342,0,0b01001},   {343,1,0b11001},   {344,1,0b00001},   {346,0,0b10001},   {347,0,0b10111},   {348,1,0b11101},   {349,0,0b00111},   {350,1,0b10001},   {351,1,0b10001},   {352,0,0b01011},   {353,1,0b11001},   {354,1,0b00011},   {355,1,0b00111},   {357,0,0b10011},   {358,1,0b11101},   {359,1,0b00011},   {360,0,0b01111},   {361,1,0b10001},   {362,1,0b01111},   {363,1,0b10101},   {366,1,0b11111},   {367,1,0b00111},   {370,1,0b11001},   {371,1,0b11101},   {373,1,0b00011},   {375,1,0b00101},   {378,1,0b00111},   {379,1,0b11111},   {381,0,0b01001},   {382,0,0b10011},   {383,0,0b11001},   {384,1,0b11011},   {385,0,0b00111},   {386,1,0b01101},   {387,0,0b00011},   {388,0,0b00101},   {389,0,0b01101},   {390,0,0b10111},   {391,0,0b11001},   {392,1,0b11111},   {393,1,0b10111},   {394,1,0b00001},   {395,0,0b01101},   {396,1,0b01101},   {397,0,0b00001},   {398,0,0b01011},   {399,1,0b10111},   {400,1,0b11111},   {401,0,0b00101},   {402,1,0b01001},   {403,1,0b01101},   {404,0,0b10001},   {405,0,0b10011},   {406,1,0b10101},   {407,1,0b01011},   {408,1,0b10001},   {409,0,0b01011},   {410,1,0b01111},   {411,1,0b11011},   {412,0,0b00101},   {413,1,0b10001},   {414,0,0b00111},   {415,0,0b11001},   {416,0,0b11001},   {417,1,0b11101},   {418,1,0b01001},   {419,0,0b00111},   {420,1,0b11011},   {421,1,0b11101},   {422,1,0b01111},   {423,1,0b11001},   {424,0,0b00011},   {425,1,0b10111},   {426,0,0b00101},   {427,1,0b11001},   {428,0,0b10011},   {429,1,0b10111},   {430,1,0b10011},   {431,1,0b01011},   {432,0,0b01111},   {433,1,0b11111},   {438,1,0b10111},   {440,1,0b00011},   {441,1,0b10011},   {442,1,0b01111},   {445,1,0b11111},   {446,1,0b01001},   {447,0,0b00001},   {448,0,0b00001},   {449,0,0b01101},   {450,0,0b01111},   {451,0,0b10101},   {452,0,0b11101},   {453,1,0b11111},   {454,0,0b10001},   {455,1,0b11111},   {456,0,0b10001},   {457,1,0b11101},   {458,0,0b00101},   {459,1,0b01001},   {460,0,0b00101},   {461,0,0b10011},   {462,0,0b11001},   {463,0,0b11101},   {464,1,0b11101},   {465,1,0b01111},   {466,0,0b00011},   {467,1,0b10111},   {468,1,0b00001},   {469,1,0b01111},   {470,1,0b00111},   {471,1,0b10011},   {472,0,0b11001},   {473,0,0b11001},   {474,1,0b11101},   {475,1,0b10111},   {476,1,0b11111},   {477,0,0b00011},   {478,1,0b01001},   {479,0,0b00001},   {480,0,0b10101},   {481,1,0b11101},   {482,0,0b10001},   {483,1,0b10011},   {484,1,0b00001},   {485,1,0b00001},   {486,0,0b01111},   {487,1,0b01111},   {488,1,0b00111},   {489,0,0b00011},   {490,0,0b00101},   {491,0,0b01101},   {492,1,0b10001},   {493,0,0b00011},   {494,1,0b10111},   {495,0,0b01001},   {496,1,0b10101},   {497,0,0b00111},   {498,1,0b01011},   {499,0,0b00001},   {500,1,0b10001},   {501,0,0b01011},   {502,0,0b10011},   {503,1,0b11111},   {504,1,0b11011},   {505,1,0b10011},   {506,1,0b10001},   {507,0,0b00111},   {508,1,0b10001},   {509,1,0b01101},   {510,0,0b10111},   {511,1,0b11101},   {512,1,0b11101},   {513,0,0b00111},   {514,0,0b11001},   {515,1,0b11101},   {516,1,0b01011},   {517,0,0b10001},   {518,1,0b10111},   {519,0,0b00011},   {520,1,0b00111},   {521,1,0b00101},   {522,0,0b10101},   {523,1,0b11001},   {524,0,0b00101},   {525,1,0b10101},   {526,1,0b10001},   {527,0,0b01001},   {528,1,0b11111},   {529,0,0b01101},   {530,1,0b01111},   {531,1,0b11001},   {532,0,0b00101},   {533,1,0b00111},   {534,1,0b01011},   {535,0,0b00111},   {536,0,0b10001},   {537,0,0b10101},   {538,1,0b10111},   {539,0,0b00011},   {540,0,0b01001},   {541,1,0b10011},   {542,1,0b11101},   {543,0,0b01001},   {544,0,0b01001},   {545,0,0b01011},   {546,1,0b11011},   {547,0,0b01101},   {548,0,0b10101},   {549,1,0b10101},   {550,0,0b00001},   {551,0,0b00101},   {552,1,0b01111},   {553,1,0b01011},   {554,0,0b10101},   {555,1,0b11101},   {556,1,0b10011},   {557,0,0b01001},   {558,0,0b10001},   {559,1,0b10101},   {560,0,0b00101},   {561,0,0b01111},   {562,0,0b10101},   {563,0,0b10111},   {564,1,0b11101},   {565,1,0b01111},   {566,1,0b00101},   {567,0,0b00101},   {568,0,0b00111},   {569,1,0b01001},   {570,1,0b01011},   {571,1,0b01111},   {572,1,0b00101},   {573,1,0b00111},   {574,1,0b11011},   {575,0,0b00111},   {576,1,0b10101},   {577,1,0b01111},   {578,1,0b11001},   {579,1,0b00001},   {580,1,0b00001},   {581,1,0b00001},   {582,0,0b01011},   {583,1,0b11011},   {584,0,0b00011},   {585,0,0b01001},   {586,0,0b10011},   {587,1,0b11011},   {588,0,0b10001},   {589,1,0b11001},   {590,1,0b00101},   {591,1,0b10111},   {592,0,0b10011},   {593,0,0b10101},   {594,1,0b11001},   {595,0,0b10011},   {596,1,0b10101},   {597,1,0b10011},   {598,1,0b11001},   {599,0,0b00111},   {600,0,0b10101},   {601,0,0b10101},   {602,1,0b11101},   {603,0,0b10011},   {604,1,0b10011},   {605,1,0b01001},   {606,1,0b01011},   {607,1,0b00111},   {608,0,0b00001},   {609,0,0b00011},   {610,0,0b10001},   {611,1,0b11001},   {612,0,0b00001},   {613,0,0b01011},   {614,1,0b11001},   {615,1,0b00111},   {616,1,0b11001},   {617,0,0b00111},   {618,0,0b01101},   {619,0,0b10001},   {620,1,0b10001},   {621,1,0b11011},   {622,0,0b01101},   {623,1,0b11101},   {624,0,0b00011},   {625,1,0b01111},   {626,1,0b11101},   {627,0,0b00111},   {628,1,0b01001},   {629,0,0b00001},   {630,1,0b11011},   {631,0,0b00101},   {632,1,0b10111},   {633,0,0b00111},   {634,1,0b11011},   {635,1,0b11101},   {636,1,0b00011},   {637,1,0b01111},   {638,0,0b00011},   {639,0,0b01101},   {640,1,0b11011},   {641,1,0b00011},   {642,0,0b00111},   {643,0,0b01001},   {644,0,0b01011},   {645,0,0b10111},   {646,0,0b11001},   {647,1,0b11101},   {648,1,0b01011},   {649,1,0b10111},   {650,1,0b00011},   {651,0,0b00101},   {652,0,0b10111},   {653,0,0b10111},   {654,1,0b11011},   {655,0,0b01011},   {656,1,0b11011},   {657,0,0b00111},   {658,0,0b01101},   {659,0,0b10011},   {660,1,0b11111},   {661,1,0b01011},   {662,0,0b10101},   {663,1,0b11011},   {664,0,0b01011},   {665,0,0b01111},   {666,1,0b11001},   {667,1,0b11011},   {668,1,0b00111},   {669,1,0b00101},   {670,1,0b01001},   {671,1,0b01011},   {672,1,0b11101},   {673,1,0b10011},   {674,0,0b01001},   {675,1,0b11001},   {676,0,0b00101},   {677,1,0b01111},   {678,0,0b00001},   {679,1,0b10001},   {680,0,0b00101},   {681,1,0b11011},   {682,1,0b10101},   {683,0,0b01111},   {684,0,0b10111},   {685,1,0b11111},   {686,0,0b00101},   {687,0,0b01011},   {688,0,0b10011},   {689,1,0b11011},   {690,1,0b01001},   {691,0,0b00101},   {692,1,0b11001},   {693,0,0b10011},   {694,1,0b10101},   {695,1,0b10001},   {696,1,0b10111},   {697,1,0b00111},   {698,1,0b11001},   {699,1,0b00001},   {700,1,0b01101},   {701,1,0b11111},   {702,1,0b00001},   {703,0,0b00111},   {704,1,0b11001},   {705,1,0b10001},   {706,0,0b11101},   {707,1,0b11101},   {708,0,0b10111},   {709,1,0b11111},   {710,0,0b00001},   {711,0,0b00101},   {712,1,0b01011},   {713,0,0b00011},   {714,1,0b10111},   {715,0,0b00011},   {716,1,0b00101},   {717,0,0b01001},   {718,0,0b10101},   {719,1,0b11111},   {720,0,0b01011},   {721,1,0b10111},   {722,1,0b01111},   {723,0,0b01011},   {724,1,0b10101},   {725,0,0b01111},   {726,1,0b11011},   {727,0,0b00111},   {728,0,0b10011},   {729,1,0b10111},   {730,0,0b00111},   {731,0,0b10001},   {732,1,0b10011},   {733,1,0b00001},   {734,0,0b01111},   {735,0,0b01111},   {736,1,0b10001},   {737,1,0b01001},   {738,0,0b00001},   {739,1,0b00111},   {740,1,0b11011},   {741,0,0b10001},   {742,0,0b10001},   {743,0,0b10001},   {744,1,0b10111},   {745,1,0b10001},   {746,1,0b10001},   {747,0,0b01001},   {748,0,0b01101},   {749,0,0b11001},   {750,1,0b11011},   {751,1,0b10001},   {752,0,0b00001},   {753,0,0b00011},   {754,1,0b00101},   {755,1,0b00111},   {756,0,0b00011},   {757,1,0b01001},   {758,1,0b00111},   {759,0,0b00001},   {760,1,0b11101},   {761,1,0b01111},   {762,1,0b00111},   {763,0,0b01001},   {764,1,0b10001},   {765,1,0b00111},   {766,1,0b01011},   {767,1,0b10011},   {768,0,0b01101},   {769,1,0b10001},   {770,1,0b10001},   {771,0,0b00011},   {772,0,0b00101},   {773,1,0b11101},   {774,1,0b10111},   {775,1,0b10101},   {776,1,0b10001},   {777,1,0b11101},   {778,0,0b11001},   {779,1,0b11101},   {780,0,0b01011},   {781,1,0b11001},   {782,0,0b10001},   {783,1,0b10101},   {784,1,0b00001},   {785,0,0b10001},   {786,1,0b11101},   {787,1,0b11111},   {788,0,0b00011},   {789,1,0b10001},   {790,1,0b01011},   {791,1,0b10101},   {792,0,0b11001},   {793,1,0b11101},   {794,0,0b10001},   {795,1,0b10101},   {796,1,0b01001},   {797,1,0b11011},   {798,0,0b00111},   {799,0,0b01001},   {800,0,0b10111},   {801,1,0b11111},   {802,1,0b11101},   {803,1,0b10101},   {804,0,0b00011},   {805,1,0b10001},   {806,1,0b11111},   {807,1,0b10111},   {808,1,0b01111},   {809,0,0b01011},   {810,1,0b11011},   {811,1,0b10101},   {812,0,0b00011},   {813,1,0b01101},   {814,0,0b00101},   {815,0,0b01001},   {816,1,0b10001},   {817,0,0b01101},   {818,1,0b11111},   {819,1,0b01001},   {820,0,0b10101},   {821,0,0b10111},   {822,1,0b11111},   {823,0,0b00001},   {824,1,0b10011},   {825,1,0b00011},   {826,1,0b01101},   {827,1,0b11011},   {828,1,0b00001},   {829,1,0b11011},   {830,1,0b10011},   {831,0,0b01101},   {832,1,0b10001},   {833,0,0b00011},   {834,1,0b10011},   {835,0,0b00101},   {836,1,0b01001},   {837,0,0b01111},   {838,1,0b10011},   {839,0,0b10011},   {840,1,0b11111},   {841,1,0b01001},   {842,1,0b01101},   {843,0,0b01111},   {844,1,0b10101},   {845,0,0b00011},   {846,1,0b11111},   {847,1,0b00111},   {848,0,0b00001},   {849,0,0b00111},   {850,0,0b10101},   {851,0,0b10111},   {852,1,0b11101},   {853,0,0b01111},   {854,1,0b11001},   {855,1,0b10001},   {856,1,0b01111},   {857,0,0b10111},   {858,0,0b11001},   {859,1,0b11001},   {860,0,0b10101},   {861,1,0b11111},   {862,1,0b10011},   {863,0,0b01011},   {864,1,0b11011},   {865,1,0b11011},   {866,0,0b10011},   {867,1,0b10101},   {868,0,0b00101},   {869,1,0b00101},   {870,0,0b00011},   {871,0,0b10111},   {872,1,0b11011},   {873,0,0b01001},   {874,0,0b01001},   {875,1,0b11101},   {876,0,0b01001},   {877,0,0b10101},   {878,1,0b11101},   {879,0,0b11001},   {880,0,0b11111},   {881,1,0b11111},   {882,0,0b01011},   {883,1,0b11101},   {884,0,0b00111},   {885,1,0b10111},   {886,0,0b00101},   {887,1,0b01011},   {888,0,0b00001},   {889,1,0b00011},   {890,0,0b00111},   {891,0,0b01001},   {892,1,0b01101},   {893,0,0b00011},   {894,1,0b10011},   {895,1,0b01111},   {896,1,0b10111},   {897,0,0b01001},   {898,0,0b01111},   {899,0,0b10011},   {900,1,0b11111},   {901,1,0b11001},   {902,0,0b00011},   {903,1,0b10001},   {904,1,0b01011},   {905,0,0b00101},   {906,1,0b01001},   {907,0,0b10011},   {908,0,0b10111},   {909,0,0b11011},   {910,1,0b11111},   {911,1,0b11111},   {912,1,0b00111},   {913,1,0b01101},   {914,1,0b11101},   {915,0,0b10101},   {916,0,0b11011},   {917,1,0b11011},   {918,0,0b01111},   {919,0,0b10001},   {920,1,0b11101},   {921,0,0b01011},   {922,1,0b10111},   {923,0,0b00001},   {924,0,0b01011},   {925,1,0b10001},   {926,0,0b00011},   {927,1,0b01101},   {928,1,0b11001},   {929,0,0b00111},   {930,0,0b01101},   {931,0,0b11011},   {932,1,0b11111},   {933,1,0b11011},   {934,1,0b11011},   {935,0,0b10101},   {936,1,0b11001},   {937,1,0b00111},   {938,1,0b01011},   {939,0,0b00101},   {940,1,0b00111},   {941,1,0b01001},   {942,1,0b11111},   {943,0,0b01101},   {944,1,0b11101},   {945,0,0b00001},   {946,1,0b11001},   {947,0,0b01001},   {948,1,0b01111},   {949,1,0b00001},   {950,0,0b00101},   {951,0,0b01111},   {952,0,0b11001},   {953,1,0b11001},   {954,0,0b10001},   {955,1,0b11011},   {956,0,0b01101},   {957,1,0b01101},   {958,1,0b01101},   {959,0,0b00111},   {960,0,0b01011},   {961,1,0b10011},   {962,0,0b00111},   {963,0,0b10001},   {964,0,0b10101},   {965,1,0b11001},   {966,1,0b11011},   {967,1,0b11101},   {968,1,0b01001},   {969,0,0b00101},   {970,0,0b00111},   {971,1,0b10111},   {972,1,0b00001},   {973,0,0b01101},   {974,1,0b10101},   {975,1,0b11111},   {976,1,0b01111},   {977,0,0b00101},   {978,0,0b10001},   {979,1,0b11111},   {980,1,0b10111},   {981,0,0b01011},   {982,0,0b01101},   {983,1,0b10101},   {984,1,0b11101},   {985,1,0b10011},   {986,0,0b10101},   {987,1,0b11111},   {988,0,0b10011},   {989,1,0b11111},   {990,0,0b00111},   {991,1,0b10111},   {992,0,0b10101},   {993,0,0b10101},   {994,1,0b11011},   {995,0,0b01001},   {996,1,0b10101},   {997,1,0b01011},   {998,1,0b10101},   {999,1,0b11001},   {1000,1,0b01111},   {1001,1,0b00011},   {1002,0,0b00001},   {1003,0,0b00011},   {1004,0,0b00101},   {1005,1,0b01011},   {1006,1,0b00101},   {1007,1,0b00101},   {1008,0,0b00111},   {1009,1,0b01011},   {1010,0,0b11001},   {1011,1,0b11111},   {1012,0,0b10101},   {1013,0,0b11101},   {1014,1,0b11111},   {1015,1,0b10101},   {1016,0,0b01011},   {1017,0,0b10111},   {1018,1,0b11111},   {1019,1,0b11011},   {1020,1,0b10101},   {1021,0,0b01001},   {1022,0,0b10001},   {1023,1,0b11111},   {1024,1,0b11001},   {1025,0,0b00101},   {1026,0,0b10001},   {1027,1,0b11001},   {1028,0,0b10111},   {1029,1,0b10111},   {1030,0,0b11011},   {1031,1,0b11111},   {1032,1,0b11111},   {1033,0,0b01011},   {1034,0,0b01011},   {1035,1,0b10011},   {1036,0,0b01101},   {1037,1,0b10011},   {1038,0,0b00001},   {1039,1,0b01111},   {1040,1,0b00011},   {1041,0,0b10101},   {1042,1,0b11101},   {1043,0,0b11001},   {1044,1,0b11011},   {1045,1,0b01101},   {1046,0,0b00101},   {1047,0,0b01011},   {1048,0,0b10101},   {1049,0,0b11101},   {1050,1,0b11101},   {1051,0,0b00001},   {1052,0,0b01101},   {1053,1,0b10001},   {1054,0,0b00101},   {1055,0,0b00111},   {1056,1,0b01001},   {1057,0,0b11101},   {1058,1,0b11101},   {1059,1,0b10001},   {1060,1,0b10101},   {1061,0,0b00101},   {1062,1,0b10101},   {1063,0,0b00011},   {1064,0,0b00101},   {1065,1,0b10111},   {1066,0,0b01011},   {1067,1,0b01011},   {1068,0,0b00101},   {1069,1,0b01011},   {1070,1,0b01001},   {1071,0,0b00011},   {1072,1,0b00101},   {1073,0,0b10001},   {1074,1,0b11101},   {1075,1,0b01011},   {1076,1,0b11001}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_int_iter_skip1(it);
                {
                    const uint8_t expected_boundary[] = {0b01100010, 0b00101001, 0b11111000, 0b10110111, 0b10010000, 0b00101011, 0b01101111, 0b10111011};
                    const std::vector<uint32_t> occupieds_pos = {0, 2, 4, 6, 7, 8, 9, 10, 12, 13, 14, 15, 17, 18, 20, 21, 23, 26, 27, 28, 29, 30, 31, 32, 33, 34, 36, 37, 39, 40, 41, 42, 43, 44, 47, 49, 50, 52, 53, 54, 55, 56, 57, 58, 60, 63, 64, 65, 66, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 95, 96, 97, 98, 100, 101, 102, 104, 105, 107, 108, 109, 110, 111, 112, 113, 115, 116, 118, 119, 120, 121, 122, 123, 125, 126, 127, 128, 131, 134, 135, 136, 138, 139, 140, 141, 142, 143, 145, 146, 147, 148, 150, 152, 154, 155, 156, 157, 158, 159, 160, 163, 164, 165, 166, 170, 171, 172, 173, 175, 176, 177, 179, 180, 181, 185, 186, 187, 188, 189, 190, 193, 195, 196, 198, 199, 200, 201, 202, 203, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 219, 220, 222, 223, 224, 225, 226, 227, 229, 230, 231, 232, 233, 234, 235, 236, 237, 239, 240, 241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 254, 255, 256, 257, 258, 260, 261, 262, 264, 265, 266, 268, 269, 271, 273, 274, 275, 276, 277, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 291, 292, 297, 298, 300, 301, 302, 303, 304, 305, 308, 309, 310, 311, 312, 313, 314, 316, 317, 318, 321, 322, 323, 324, 326, 327, 328, 329, 331, 332, 335, 338, 340, 341, 343, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 358, 361, 362, 364, 365, 367, 368, 369, 370, 371, 372, 373, 374, 377, 378, 380, 382, 383, 384, 387, 388, 389, 391, 392, 393, 394, 395, 396, 398, 399, 400, 401, 403, 404, 406, 409, 410, 411, 413, 415, 417, 418, 420, 421, 424, 425, 429, 430, 431, 432, 433, 434, 435, 437, 438, 439, 440, 442, 443, 444, 445, 446, 447, 448, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 461, 462, 463, 464, 466, 468, 471, 472, 474, 475, 477, 478, 479, 480, 481, 482, 484, 485, 486, 487, 489, 490, 491, 492, 493, 494, 497, 498, 499, 502, 503, 505, 506, 507, 508, 509, 511, 512, 513, 516, 517, 521, 523, 525, 526, 527, 528, 530, 532, 537, 538, 539, 541, 542, 543, 544, 545, 549, 551, 552, 553, 554, 556, 557, 558, 559, 560, 562, 564, 567, 568, 570, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 589, 590, 591, 592, 593, 594, 595, 596, 597, 598, 601, 602, 603, 604, 606, 607, 608, 609, 610, 612, 613, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 629, 630, 631, 632, 634, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 647, 649, 650, 651, 654, 655, 656, 657, 658, 659, 661, 662, 664, 668, 669, 670, 671, 673, 675, 676, 677, 678, 680, 681, 682, 683, 684, 686, 687, 688, 689, 691, 692, 693, 694, 698, 700, 702, 703, 704, 705, 706, 708, 709, 710, 714, 715, 717, 718, 719, 720, 721, 722, 724, 725, 730, 731, 732, 733, 734, 735, 736, 737, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753, 756, 757, 758, 762, 763, 765, 767, 768, 770, 771, 772, 774, 776, 777, 778, 779, 781, 783, 784, 786, 787, 789, 790, 792, 793, 796, 797, 800, 801, 802, 803, 805, 809, 811};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b11101},   {  1,1,0b11111},   {  2,0,0b10101},   {  3,1,0b11011},   {  5,0,0b01001},   {  6,0,0b01011},   {  7,1,0b10011},   {  8,0,0b00101},   {  9,1,0b10001},   { 10,0,0b01101},   { 11,1,0b11001},   { 12,0,0b00001},   { 13,1,0b11111},   { 14,1,0b10001},   { 15,1,0b00001},   { 16,0,0b01001},   { 17,1,0b11111},   { 18,0,0b01101},   { 19,1,0b01111},   { 20,0,0b01111},   { 21,0,0b10111},   { 22,1,0b11111},   { 23,1,0b01111},   { 24,0,0b00001},   { 25,0,0b01001},   { 26,1,0b11001},   { 27,1,0b10001},   { 28,1,0b10111},   { 29,1,0b01011},   { 30,0,0b00011},   { 31,1,0b11001},   { 34,0,0b01001},   { 35,0,0b01111},   { 36,0,0b10101},   { 37,1,0b11001},   { 38,0,0b00011},   { 39,0,0b00101},   { 40,0,0b01101},   { 41,0,0b10011},   { 42,1,0b11011},   { 43,1,0b11001},   { 44,1,0b00011},   { 45,1,0b01111},   { 46,1,0b11011},   { 47,1,0b11111},   { 48,1,0b10011},   { 49,1,0b11111},   { 50,1,0b00001},   { 51,1,0b10001},   { 52,0,0b01101},   { 53,0,0b01111},   { 54,1,0b10101},   { 55,1,0b10101},   { 56,0,0b10111},   { 57,1,0b11101},   { 58,1,0b10101},   { 59,0,0b00001},   { 60,1,0b10001},   { 61,1,0b01111},   { 62,0,0b01101},   { 63,0,0b10111},   { 64,1,0b11001},   { 65,0,0b01001},   { 66,0,0b01011},   { 67,1,0b01111},   { 68,1,0b00111},   { 69,1,0b00111},   { 70,0,0b00111},   { 71,1,0b01101},   { 72,1,0b11101},   { 73,1,0b10101},   { 74,0,0b10001},   { 75,1,0b11111},   { 76,1,0b11111},   { 77,0,0b01001},   { 78,0,0b10111},   { 79,1,0b11001},   { 80,0,0b00111},   { 81,1,0b11111},   { 83,0,0b01011},   { 84,1,0b10101},   { 85,0,0b00001},   { 86,0,0b01101},   { 87,0,0b10011},   { 88,1,0b10101},   { 89,1,0b01011},   { 90,0,0b01101},   { 91,1,0b01111},   { 92,1,0b00101},   { 93,1,0b11101},   { 94,0,0b00111},   { 95,1,0b01001},   { 96,0,0b00101},   { 97,1,0b10001},   { 98,0,0b10001},   { 99,0,0b10001},   {100,1,0b11001},   {101,1,0b01101},   {102,1,0b01001},   {103,0,0b00101},   {104,0,0b00111},   {105,0,0b00111},   {106,1,0b10011},   {107,1,0b00001},   {108,0,0b01101},   {109,1,0b10101},   {110,0,0b10101},   {111,1,0b11001},   {112,0,0b01111},   {113,1,0b01111},   {114,1,0b00101},   {115,0,0b10001},   {116,0,0b10101},   {117,0,0b10111},   {118,1,0b11011},   {119,1,0b10111},   {120,1,0b10101},   {121,0,0b01111},   {122,0,0b01111},   {123,0,0b10101},   {124,1,0b10101},   {125,0,0b00111},   {126,0,0b01011},   {127,1,0b10001},   {128,0,0b01011},   {129,1,0b11101},   {130,0,0b01001},   {131,1,0b10111},   {132,0,0b10111},   {133,1,0b11001},   {134,0,0b01111},   {135,1,0b11001},   {136,0,0b00101},   {137,1,0b11101},   {138,1,0b00011},   {139,1,0b11011},   {140,1,0b00011},   {141,0,0b10001},   {142,1,0b10111},   {143,0,0b00001},   {144,0,0b01101},   {145,0,0b10001},   {146,1,0b11011},   {147,0,0b00101},   {148,0,0b10111},   {149,1,0b11011},   {150,0,0b01111},   {151,1,0b11001},   {152,0,0b10011},   {153,1,0b10101},   {154,1,0b10111},   {155,0,0b01011},   {156,1,0b01111},   {157,0,0b00111},   {158,0,0b10001},   {159,1,0b11011},   {160,1,0b00001},   {161,0,0b11111},   {162,1,0b11111},   {163,0,0b01001},   {164,0,0b10001},   {165,1,0b10011},   {166,1,0b01111},   {167,0,0b00111},   {168,0,0b01111},   {169,1,0b10101},   {170,0,0b01101},   {171,1,0b11001},   {172,1,0b11011},   {173,1,0b01011},   {174,1,0b10101},   {175,1,0b01101},   {176,1,0b01101},   {177,0,0b00111},   {178,1,0b11001},   {179,0,0b01011},   {180,1,0b11011},   {181,0,0b00011},   {182,1,0b01001},   {183,0,0b01101},   {184,0,0b11001},   {185,1,0b11101},   {186,1,0b10011},   {187,0,0b00011},   {188,1,0b00011},   {189,0,0b00011},   {190,0,0b00101},   {191,1,0b10101},   {192,1,0b00001},   {193,1,0b10001},   {194,1,0b11011},   {195,0,0b10001},   {196,0,0b10111},   {197,0,0b11101},   {198,1,0b11101},   {199,1,0b00101},   {200,0,0b01101},   {201,1,0b11111},   {202,1,0b01101},   {203,1,0b00101},   {204,1,0b01001},   {205,1,0b10001},   {206,0,0b01001},   {207,1,0b01011},   {208,0,0b10111},   {209,1,0b10111},   {210,1,0b01101},   {211,0,0b01011},   {212,1,0b10101},   {213,0,0b01011},   {214,1,0b10001},   {215,0,0b00001},   {216,0,0b11011},   {217,0,0b11011},   {218,1,0b11011},   {219,1,0b11101},   {220,0,0b00001},   {221,1,0b00101},   {222,0,0b00011},   {223,1,0b01001},   {224,1,0b10101},   {225,1,0b11001},   {226,1,0b11101},   {227,1,0b00001},   {228,1,0b01001},   {229,0,0b00111},   {230,0,0b01001},   {231,1,0b11011},   {232,1,0b01111},   {233,0,0b01001},   {234,0,0b01011},   {235,1,0b01111},   {236,0,0b00111},   {237,0,0b00111},   {238,1,0b11011},   {239,1,0b10101},   {240,0,0b00001},   {241,0,0b10001},   {242,1,0b11011},   {243,0,0b00101},   {244,1,0b10011},   {245,0,0b00111},   {246,1,0b10001},   {247,1,0b11001},   {248,1,0b11001},   {249,1,0b00111},   {250,1,0b10111},   {251,0,0b00001},   {252,1,0b10101},   {253,1,0b10011},   {254,1,0b00101},   {255,1,0b10101},   {256,1,0b01111},   {257,0,0b01001},   {258,1,0b10001},   {259,0,0b00011},   {260,0,0b10001},   {261,1,0b11111},   {262,0,0b00011},   {263,0,0b01011},   {264,1,0b10011},   {265,1,0b00011},   {266,0,0b01101},   {267,1,0b11001},   {268,0,0b00101},   {269,0,0b10111},   {270,1,0b11011},   {271,1,0b11101},   {272,1,0b10001},   {273,0,0b00001},   {274,1,0b11101},   {275,0,0b10111},   {276,1,0b11111},   {277,0,0b01101},   {278,0,0b10111},   {279,1,0b11111},   {280,0,0b00111},   {281,1,0b11101},   {282,0,0b01011},   {283,1,0b10101},   {284,0,0b01101},   {285,1,0b10001},   {286,0,0b00101},   {287,0,0b10001},   {288,0,0b11001},   {289,1,0b11101},   {290,1,0b00111},   {291,0,0b01011},   {292,1,0b11001},   {293,0,0b01011},   {294,1,0b11001},   {295,0,0b00001},   {296,0,0b00011},   {297,0,0b00101},   {298,1,0b11011},   {299,0,0b00011},   {300,0,0b00111},   {301,1,0b10001},   {302,1,0b11011},   {303,0,0b11001},   {304,1,0b11111},   {305,1,0b11101},   {306,0,0b11001},   {307,1,0b11111},   {308,0,0b01001},   {309,1,0b01101},   {310,1,0b11011},   {311,1,0b10101},   {312,0,0b00101},   {313,0,0b01101},   {314,1,0b10111},   {315,0,0b00111},   {316,0,0b11011},   {317,1,0b11011},   {318,1,0b00011},   {319,0,0b01111},   {320,0,0b11001},   {321,1,0b11111},   {322,1,0b10011},   {323,1,0b01101},   {324,1,0b11101},   {325,1,0b00001},   {326,1,0b01101},   {327,0,0b01001},   {328,1,0b11011},   {329,0,0b00111},   {330,1,0b10001},   {331,1,0b10011},   {332,0,0b01111},   {333,0,0b10101},   {334,1,0b11101},   {335,1,0b11001},   {336,0,0b01101},   {337,1,0b10111},   {338,1,0b00001},   {339,1,0b00011},   {340,1,0b01111},   {341,1,0b11011},   {342,0,0b10101},   {343,1,0b10111},   {344,1,0b10101},   {345,1,0b11011},   {346,0,0b01011},   {347,1,0b11001},   {348,1,0b01001},   {349,1,0b01101},   {350,0,0b11001},   {351,1,0b11111},   {352,1,0b01001},   {353,1,0b11011},   {354,1,0b00101},   {355,1,0b00111},   {356,0,0b01111},   {357,1,0b11001},   {358,0,0b10011},   {359,1,0b11011},   {360,0,0b01001},   {361,1,0b10001},   {362,1,0b10101},   {363,0,0b01101},   {364,1,0b11101},   {365,1,0b01111},   {366,1,0b01101},   {367,0,0b00111},   {368,0,0b01101},   {369,1,0b11001},   {370,0,0b10011},   {371,1,0b11101},   {372,0,0b01001},   {373,0,0b01011},   {374,1,0b01111},   {375,0,0b10101},   {376,1,0b11101},   {377,0,0b00101},   {378,0,0b01011},   {379,1,0b11001},   {380,1,0b10001},   {381,0,0b00101},   {382,1,0b11101},   {383,0,0b01111},   {384,1,0b11111},   {385,0,0b10111},   {386,1,0b11111},   {387,1,0b01101},   {388,0,0b10011},   {389,1,0b10101},   {390,0,0b00001},   {391,0,0b00011},   {392,1,0b10011},   {393,0,0b01001},   {394,0,0b10011},   {395,1,0b11001},   {396,0,0b01011},   {397,1,0b01111},   {398,0,0b00111},   {399,1,0b11011},   {400,1,0b10111},   {401,0,0b01111},   {402,1,0b01111},   {403,1,0b00101},   {404,1,0b11111},   {405,1,0b11101},   {406,1,0b10101},   {407,1,0b01101},   {408,0,0b01111},   {409,0,0b10001},   {410,1,0b10001},   {411,0,0b10101},   {412,1,0b11111},   {413,0,0b01111},   {414,1,0b11101},   {415,0,0b00111},   {416,0,0b01001},   {417,1,0b10001},   {418,0,0b10011},   {419,1,0b11101},   {420,0,0b00001},   {421,1,0b01111},   {422,0,0b11001},   {423,1,0b11101},   {424,1,0b01101},   {425,1,0b11111},   {426,1,0b01101},   {427,0,0b00011},   {428,0,0b00101},   {429,0,0b11101},   {430,1,0b11111},   {431,0,0b00011},   {432,0,0b01001},   {433,1,0b11011},   {434,0,0b01101},   {435,1,0b11111},   {436,1,0b11001},   {437,0,0b01111},   {438,1,0b11101},   {439,1,0b11101},   {440,1,0b00101},   {441,0,0b01011},   {442,1,0b11011},   {443,0,0b00011},   {444,1,0b00101},   {445,1,0b11011},   {446,0,0b11111},   {447,1,0b11111},   {448,1,0b10101},   {449,0,0b00011},   {450,0,0b01111},   {451,0,0b10001},   {452,1,0b11011},   {453,0,0b01101},   {454,0,0b10101},   {455,1,0b11111},   {456,1,0b10111},   {457,1,0b01111},   {458,0,0b01101},   {459,1,0b10101},   {460,0,0b10101},   {461,1,0b11101},   {462,1,0b00111},   {463,1,0b11101},   {464,1,0b01101},   {465,0,0b00001},   {466,0,0b10001},   {467,1,0b10101},   {468,1,0b10001},   {469,0,0b00011},   {470,1,0b01101},   {471,1,0b00001},   {472,0,0b00001},   {473,1,0b01101},   {474,1,0b10011},   {475,1,0b11101},   {476,0,0b00001},   {477,1,0b11011},   {478,1,0b00001},   {479,1,0b01101},   {480,1,0b10011},   {481,1,0b00111},   {482,1,0b01111},   {483,1,0b01001},   {484,0,0b00111},   {485,1,0b11101},   {486,0,0b01011},   {487,1,0b10101},   {488,1,0b00111},   {489,0,0b00111},   {490,1,0b01001},   {491,1,0b11101},   {492,0,0b10001},   {493,1,0b11101},   {494,1,0b01001},   {495,1,0b10001},   {496,1,0b10101},   {497,0,0b00011},   {498,1,0b00111},   {500,0,0b00011},   {501,1,0b01101},   {502,1,0b10001},   {504,0,0b01111},   {505,1,0b10111},   {507,1,0b00011},   {508,1,0b10001},   {509,1,0b10101},   {513,0,0b01101},   {514,1,0b10001},   {515,0,0b00101},   {516,0,0b00111},   {517,1,0b11101},   {518,1,0b11111},   {519,0,0b00011},   {520,1,0b00011},   {521,0,0b00101},   {522,1,0b01111},   {523,0,0b00001},   {524,1,0b10111},   {525,0,0b00001},   {526,1,0b00101},   {527,1,0b01001},   {528,0,0b00101},   {529,1,0b01111},   {530,1,0b10111},   {531,1,0b00011},   {532,0,0b10001},   {533,1,0b11101},   {534,1,0b01001},   {535,0,0b00111},   {536,0,0b10111},   {537,1,0b11111},   {538,1,0b11001},   {539,0,0b00001},   {540,1,0b01101},   {542,1,0b11001},   {544,1,0b00011},   {545,0,0b00111},   {546,1,0b10111},   {548,0,0b00101},   {549,0,0b10101},   {550,1,0b11011},   {551,1,0b01101},   {553,0,0b01011},   {554,0,0b10001},   {555,1,0b11011},   {556,0,0b01111},   {557,1,0b11111},   {558,0,0b00011},   {559,0,0b01101},   {560,0,0b10011},   {561,1,0b10011},   {562,0,0b00001},   {563,1,0b11111},   {564,1,0b11001},   {565,1,0b01001},   {569,1,0b10101},   {570,0,0b10011},   {571,1,0b11001},   {572,1,0b10011},   {573,1,0b01011},   {574,1,0b01111},   {576,0,0b00011},   {577,0,0b01101},   {578,1,0b11111},   {579,0,0b00111},   {580,1,0b01001},   {581,1,0b01001},   {582,1,0b11001},   {583,1,0b10001},   {584,1,0b10011},   {586,0,0b01001},   {587,1,0b10101},   {588,0,0b10111},   {589,1,0b11111},   {590,1,0b10111},   {591,0,0b00001},   {592,0,0b00001},   {593,1,0b01011},   {594,1,0b01001},   {595,0,0b00011},   {596,1,0b10001},   {597,0,0b11101},   {598,1,0b11111},   {599,1,0b11111},   {600,1,0b11101},   {601,1,0b10111},   {602,1,0b01011},   {603,0,0b11101},   {604,1,0b11111},   {605,1,0b01011},   {606,1,0b11011},   {607,1,0b01011},   {608,0,0b10111},   {609,1,0b11011},   {610,0,0b01101},   {611,1,0b01111},   {612,1,0b01001},   {613,0,0b01111},   {614,1,0b10111},   {615,0,0b10011},   {616,0,0b10111},   {617,1,0b11001},   {618,1,0b01111},   {619,0,0b10011},   {620,1,0b10011},   {621,0,0b00101},   {622,1,0b01111},   {625,1,0b00011},   {626,0,0b00001},   {627,1,0b11011},   {629,1,0b11111},   {630,1,0b00101},   {633,0,0b01011},   {634,1,0b01101},   {635,0,0b01111},   {636,1,0b10101},   {637,1,0b10111},   {638,0,0b10101},   {639,0,0b11011},   {640,1,0b11011},   {641,1,0b00111},   {642,0,0b01111},   {643,1,0b01111},   {644,0,0b00011},   {645,0,0b00011},   {646,0,0b01111},   {647,0,0b10011},   {648,1,0b11011},   {649,1,0b01111},   {650,1,0b01111},   {651,1,0b00111},   {652,0,0b00001},   {653,1,0b00011},   {654,1,0b10011},   {655,0,0b11011},   {656,1,0b11101},   {657,0,0b00001},   {658,1,0b00011},   {659,1,0b00101},   {660,1,0b01011},   {661,1,0b00101},   {662,0,0b00011},   {663,1,0b01001},   {664,0,0b01101},   {665,1,0b11011},   {666,0,0b10001},   {667,1,0b10011},   {668,1,0b11101},   {670,1,0b11111},   {671,1,0b11001},   {673,0,0b10001},   {674,1,0b11011},   {675,0,0b00111},   {676,1,0b11001},   {677,0,0b00101},   {678,0,0b00101},   {679,1,0b01101},   {680,1,0b00011},   {681,1,0b10001},   {682,0,0b01111},   {683,1,0b01111},   {684,1,0b00001},   {686,1,0b10001},   {691,0,0b10001},   {692,1,0b11001},   {694,1,0b11001},   {696,0,0b10001},   {697,0,0b10001},   {698,1,0b10111},   {699,1,0b11011},   {700,1,0b11111},   {701,1,0b11111},   {703,1,0b00001},   {706,0,0b00111},   {707,0,0b01111},   {708,1,0b10111},   {712,0,0b00101},   {713,1,0b01101},   {714,1,0b01101},   {715,1,0b00011},   {718,0,0b10011},   {719,0,0b11011},   {720,1,0b11111},   {721,1,0b00001},   {722,0,0b00001},   {723,0,0b10011},   {724,1,0b11011},   {725,1,0b01001},   {726,0,0b00001},   {727,1,0b00011},   {728,1,0b10111},   {731,1,0b00101},   {732,1,0b11101},   {734,1,0b01111},   {735,0,0b10011},   {736,1,0b10111},   {738,0,0b00101},   {739,0,0b01101},   {740,1,0b10011},   {741,0,0b01101},   {742,1,0b01111},   {743,0,0b00101},   {744,1,0b01111},   {745,0,0b10101},   {746,1,0b10111},   {747,0,0b00111},   {748,1,0b10011},   {749,0,0b00011},   {750,1,0b10001},   {751,0,0b00011},   {752,1,0b10001},   {753,1,0b01111},   {754,1,0b11001},   {756,1,0b01101},   {760,0,0b00001},   {761,0,0b01001},   {762,0,0b11111},   {763,1,0b11111},   {764,0,0b01101},   {765,1,0b10101},   {766,1,0b01011},   {767,0,0b00111},   {768,0,0b01101},   {769,1,0b11011},   {770,1,0b00001},   {771,0,0b01011},   {772,0,0b01111},   {773,1,0b11001},   {774,0,0b00111},   {775,0,0b01011},   {776,0,0b11011},   {777,1,0b11011},   {778,0,0b01011},   {779,1,0b01011},   {780,0,0b01101},   {781,0,0b10001},   {782,1,0b11001},   {783,1,0b00101},   {784,1,0b10011},   {785,0,0b01101},   {786,1,0b11111},   {787,0,0b00111},   {788,0,0b01011},   {789,1,0b11001},   {790,1,0b11111},   {791,1,0b10111},   {792,0,0b00011},   {793,1,0b11011},   {794,0,0b11001},   {795,1,0b11011},   {796,0,0b00101},   {797,0,0b00101},   {798,1,0b11111},   {799,0,0b01011},   {800,1,0b10111},   {801,1,0b01101},   {802,1,0b01001},   {803,0,0b01001},   {804,1,0b10011},   {805,1,0b11001},   {806,0,0b10111},   {807,1,0b11011},   {808,0,0b10101},   {809,1,0b11101},   {810,1,0b11101},   {811,0,0b00111},   {812,0,0b01011},   {813,1,0b10001},   {814,0,0b00001},   {815,0,0b00011},   {816,1,0b00011},   {817,1,0b10111},   {818,1,0b00111},   {819,0,0b01001},   {820,1,0b01011},   {821,0,0b00011},   {822,1,0b10011},   {823,0,0b01001},   {824,0,0b11001},   {825,1,0b11001},   {826,1,0b10111},   {827,1,0b01011},   {828,0,0b00011},   {829,1,0b10001},   {830,0,0b00101},   {831,1,0b11001},   {832,1,0b10011},   {833,1,0b10111},   {834,0,0b00001},   {835,0,0b01001},   {836,1,0b11011},   {837,0,0b00101},   {838,0,0b01011},   {839,1,0b10001},   {840,0,0b01111},   {841,1,0b11001},   {842,1,0b00101},   {843,1,0b10011},   {844,1,0b11001},   {845,1,0b10011},   {846,1,0b10011},   {847,1,0b01111},   {848,1,0b01101},   {849,1,0b10101},   {850,0,0b00011},   {851,0,0b00111},   {852,1,0b10001},   {853,1,0b00101},   {854,0,0b00001},   {855,0,0b01101},   {856,1,0b10111},   {857,1,0b11001},   {858,0,0b11001},   {859,1,0b11011},   {860,1,0b10111},   {861,1,0b10011},   {862,1,0b00001},   {863,0,0b00111},   {864,1,0b11001},   {865,0,0b00011},   {866,1,0b10111},   {867,0,0b01101},   {868,1,0b10001},   {869,1,0b01001},   {870,0,0b10111},   {871,1,0b10111},   {872,1,0b10001},   {873,1,0b00011},   {874,1,0b01011},   {875,1,0b01101},   {876,0,0b00011},   {877,1,0b11111},   {878,1,0b11101},   {879,1,0b10111},   {880,1,0b00101},   {881,0,0b01001},   {882,1,0b11101},   {883,1,0b00111},   {884,0,0b10011},   {885,1,0b11101},   {886,0,0b00111},   {887,1,0b10101},   {888,0,0b00011},   {889,1,0b01111},   {890,0,0b01001},   {891,1,0b01111},   {892,0,0b00011},   {893,1,0b10011},   {894,0,0b01011},   {895,1,0b11001},   {896,0,0b00001},   {897,1,0b00101},   {898,1,0b01011},   {899,0,0b00011},   {900,0,0b00011},   {901,1,0b01111},   {902,0,0b01011},   {903,1,0b01101},   {904,0,0b00001},   {905,1,0b10011},   {906,0,0b00001},   {907,0,0b00001},   {908,0,0b00111},   {909,0,0b10011},   {910,0,0b11001},   {911,1,0b11001},   {912,1,0b10001},   {913,1,0b10111},   {914,0,0b00001},   {915,0,0b00011},   {916,1,0b11011},   {917,1,0b11101},   {918,0,0b00011},   {919,0,0b10001},   {920,1,0b10111},   {921,0,0b01001},   {922,0,0b10011},   {923,1,0b10111},   {924,0,0b00111},   {925,1,0b11101},   {926,1,0b00111},   {927,0,0b00001},   {928,1,0b00011},   {929,0,0b00011},   {930,1,0b11011},   {931,0,0b00001},   {932,0,0b01011},   {933,1,0b01011},   {934,0,0b00001},   {935,0,0b00111},   {936,0,0b01011},   {937,0,0b11101},   {938,1,0b11111},   {939,1,0b11101},   {940,0,0b00101},   {941,1,0b00111},   {942,1,0b00111},   {943,0,0b11001},   {944,1,0b11111},   {945,0,0b00111},   {946,1,0b11101},   {947,1,0b10101},   {948,1,0b11011},   {949,0,0b00101},   {950,1,0b11011},   {951,1,0b01001},   {952,1,0b00001},   {953,1,0b11011},   {954,1,0b11101},   {955,0,0b00001},   {956,1,0b00001},   {957,0,0b00101},   {958,0,0b00101},   {959,1,0b01111},   {960,1,0b00111},   {961,1,0b11101},   {962,0,0b11011},   {963,1,0b11111},   {964,0,0b00001},   {965,1,0b01101},   {966,0,0b00101},   {967,0,0b00111},   {968,1,0b10101},   {969,1,0b11001},   {970,0,0b00001},   {971,1,0b01011},   {972,0,0b00011},   {973,0,0b01011},   {974,1,0b10001},   {975,0,0b01111},   {976,0,0b10011},   {977,1,0b11011},   {978,0,0b01101},   {979,1,0b11101},   {980,0,0b10001},   {981,1,0b11101},   {982,0,0b01111},   {983,1,0b01111},   {984,0,0b01001},   {985,0,0b01001},   {986,0,0b01101},   {987,1,0b11111},   {988,1,0b10111},   {989,1,0b00111},   {990,0,0b00101},   {991,1,0b01101},   {992,0,0b01001},   {993,0,0b10101},   {994,1,0b11001},   {995,1,0b00101},   {996,1,0b00101},   {997,0,0b01011},   {998,0,0b01011},   {999,1,0b01011},   {1000,1,0b10111},   {1001,0,0b01001},   {1002,1,0b01111},   {1003,0,0b00001},   {1004,0,0b01001},   {1005,0,0b01001},   {1006,0,0b01011},   {1007,1,0b11011},   {1008,1,0b10101},   {1009,1,0b11111},   {1010,0,0b01001},   {1011,1,0b01101},   {1012,0,0b00001},   {1013,1,0b11101},   {1014,1,0b00001},   {1015,1,0b10111},   {1016,1,0b00001},   {1017,1,0b11011},   {1018,1,0b10011},   {1019,0,0b00001},   {1020,1,0b11101},   {1021,0,0b01001},   {1022,1,0b10101},   {1023,0,0b11001},   {1024,1,0b11101},   {1025,0,0b00001},   {1026,1,0b01011},   {1027,0,0b01111},   {1028,1,0b10101},   {1029,1,0b01001},   {1030,1,0b01101},   {1031,0,0b01111},   {1032,0,0b11011},   {1033,0,0b11101},   {1034,1,0b11111},   {1035,0,0b00001},   {1036,0,0b00011},   {1037,1,0b01111},   {1038,0,0b11001},   {1039,1,0b11111},   {1040,1,0b00111},   {1041,0,0b00001},   {1042,1,0b00011},   {1043,0,0b01011},   {1044,0,0b10011},   {1045,1,0b11101},   {1046,1,0b11011},   {1047,1,0b01001},   {1048,1,0b00101},   {1049,1,0b11001},   {1050,1,0b01011},   {1051,1,0b01111},   {1052,1,0b10101},   {1056,1,0b11011},   {1057,0,0b01011},   {1058,0,0b01101},   {1059,1,0b11011},   {1061,0,0b00111},   {1062,0,0b11011},   {1063,1,0b11101},   {1064,0,0b00101},   {1065,1,0b00101},   {1066,0,0b00101},   {1067,1,0b11001},   {1068,0,0b00011},   {1069,1,0b01101},   {1070,0,0b01101},   {1071,1,0b11011},   {1073,0,0b00101},   {1074,0,0b10001},   {1075,1,0b11001},   {1076,1,0b00011}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_int_iter_skip1(it);
                {
                    const uint8_t expected_boundary[] = {0b11000111, 0b10011111, 0b10110001, 0b01100101, 0b10001101, 0b10001110, 0b10111111, 0b01000111};
                    const std::vector<uint32_t> occupieds_pos = {1, 2, 5, 9, 11, 13, 14, 16, 17, 21, 23, 28, 30, 31, 32, 33, 34, 37, 39, 41, 45, 47, 50, 52, 54, 58, 65, 66, 69, 72, 75, 76, 79, 86, 90, 97, 104, 105, 108, 110, 111, 113, 123, 126, 128, 129, 130, 131, 133, 134, 137, 138, 139, 140, 141, 142, 144, 146, 147, 148, 149, 150, 151, 152, 153, 154, 158, 160, 161, 163, 165, 172, 173, 174, 176, 181, 182, 183, 184, 186, 187, 190, 192, 193, 194, 195, 196, 198, 201, 207, 213, 214, 216, 217, 218, 220, 222, 223, 225, 226, 227, 232, 234, 237, 238, 240, 243, 244, 246, 248, 249, 250, 251, 252, 253, 256, 257, 258, 260, 261, 262, 266, 268, 274, 275, 276, 277, 279, 280, 281, 283, 285, 288, 289, 290, 291, 294, 295, 297, 298, 300, 301, 303, 305, 306, 313, 316, 318, 319, 325, 326, 327, 331, 333, 335, 336, 337, 338, 339, 343, 344, 345, 346, 351, 352, 361, 365, 366, 367, 369, 370, 374, 375, 382, 384, 387, 388, 389, 391, 392, 393, 394, 399, 401, 402, 406, 410, 411, 413, 414, 415, 423, 425, 426, 430, 431, 439, 442, 445, 446, 448, 450, 451, 452, 453, 454, 456, 462, 463, 466, 467, 469, 470, 472, 473, 474, 475, 476, 477, 479, 480, 482, 483, 486, 487, 493, 494, 495, 497, 498, 501, 504, 505, 506, 508, 510, 516, 519, 521, 525, 526, 532, 536, 539, 542, 545, 547, 551, 552, 553, 557, 558, 559, 564, 567, 572, 575, 578, 579, 580, 581, 583, 584, 586, 587, 589, 590, 593, 595, 596, 597, 599, 602, 607, 608, 609, 611, 612, 613, 614, 619, 620, 621, 624, 626, 629, 632, 638, 639, 641, 642, 644, 649, 651, 652, 653, 654, 657, 658, 660, 661, 664, 666, 668, 670, 671, 673, 674, 676, 678, 683, 684, 688, 690, 691, 693, 695, 700, 701, 702, 703, 705, 706, 707, 712, 713, 714, 716, 717, 718, 719, 720, 721, 722, 723, 724, 725, 726, 728, 731, 732, 733, 738, 740, 743, 744, 749, 750, 751, 757, 760, 761, 763, 764, 766, 767, 769, 770, 773, 775, 780, 781, 783, 784, 785, 786, 787, 788, 794, 796, 799, 800, 802, 803, 805, 806, 807, 813, 814, 817, 818, 820, 823, 825, 826, 831, 834, 840, 841, 843, 846, 849, 851, 855, 858, 861, 862, 864, 867, 868, 874, 875, 876, 880, 887, 891, 894, 895, 896, 899};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,1,0b00101},   {  2,0,0b01111},   {  3,0,0b10101},   {  4,1,0b11011},   {  5,1,0b00101},   { 10,1,0b01001},   { 13,1,0b10111},   { 15,1,0b00101},   { 16,1,0b10111},   { 19,1,0b10001},   { 20,1,0b10001},   { 25,0,0b00011},   { 26,0,0b00101},   { 27,0,0b11011},   { 28,1,0b11101},   { 29,0,0b00011},   { 30,1,0b11001},   { 33,0,0b10101},   { 34,1,0b11101},   { 35,1,0b11111},   { 37,1,0b11011},   { 38,1,0b11011},   { 39,0,0b00001},   { 40,1,0b10011},   { 41,1,0b01111},   { 44,1,0b11101},   { 46,1,0b00101},   { 48,1,0b10101},   { 53,0,0b11001},   { 54,1,0b11011},   { 56,1,0b11111},   { 59,0,0b01011},   { 60,1,0b01111},   { 62,0,0b00001},   { 63,1,0b01001},   { 64,1,0b00111},   { 69,0,0b10001},   { 70,1,0b10001},   { 77,1,0b11001},   { 78,0,0b10011},   { 79,1,0b11001},   { 82,0,0b00101},   { 83,1,0b11011},   { 85,1,0b01101},   { 89,1,0b11001},   { 90,1,0b10111},   { 94,1,0b10011},   {102,1,0b10111},   {107,1,0b00111},   {115,1,0b01111},   {124,1,0b01011},   {125,1,0b10001},   {128,1,0b01111},   {131,1,0b11111},   {132,1,0b10011},   {134,1,0b01101},   {146,1,0b11101},   {150,1,0b00011},   {152,1,0b11011},   {153,1,0b11011},   {155,1,0b00001},   {156,1,0b11011},   {158,1,0b11011},   {159,1,0b01101},   {163,1,0b00001},   {164,1,0b10001},   {165,1,0b00001},   {167,1,0b01001},   {168,1,0b00111},   {169,1,0b11111},   {171,1,0b00011},   {174,0,0b01011},   {175,1,0b10011},   {176,1,0b10111},   {177,1,0b10011},   {178,1,0b01111},   {179,1,0b00001},   {180,1,0b01111},   {181,1,0b00101},   {182,1,0b10111},   {183,1,0b10011},   {188,0,0b10001},   {189,1,0b10111},   {190,0,0b10111},   {191,1,0b11001},   {192,1,0b10001},   {194,0,0b11011},   {195,1,0b11111},   {196,0,0b00011},   {197,1,0b10111},   {205,1,0b10101},   {206,1,0b10001},   {207,1,0b10011},   {210,1,0b01001},   {216,1,0b10001},   {217,1,0b11101},   {218,1,0b10011},   {219,0,0b01011},   {220,1,0b10111},   {222,1,0b00001},   {223,1,0b01101},   {226,1,0b10101},   {229,1,0b11001},   {230,1,0b00111},   {231,1,0b11001},   {232,1,0b10111},   {233,1,0b00011},   {236,1,0b01011},   {239,1,0b10001},   {247,0,0b01101},   {248,0,0b10001},   {249,1,0b11101},   {254,1,0b01011},   {255,1,0b11101},   {257,0,0b01111},   {258,1,0b10001},   {259,0,0b00001},   {260,1,0b00011},   {261,1,0b10011},   {262,1,0b01001},   {264,1,0b10111},   {266,1,0b00101},   {268,1,0b00001},   {269,0,0b01011},   {270,1,0b10111},   {271,1,0b10111},   {276,1,0b00111},   {279,1,0b11111},   {282,1,0b11111},   {284,0,0b01001},   {285,1,0b11011},   {286,1,0b01101},   {290,1,0b00101},   {291,1,0b10111},   {293,1,0b00101},   {296,1,0b01101},   {297,1,0b10101},   {298,1,0b11011},   {299,0,0b01011},   {300,0,0b01101},   {301,1,0b01101},   {302,0,0b01011},   {303,1,0b11011},   {304,0,0b00011},   {305,1,0b10101},   {306,1,0b11101},   {307,0,0b01001},   {308,1,0b01101},   {309,0,0b01111},   {310,1,0b10111},   {311,0,0b10101},   {312,0,0b11011},   {313,1,0b11101},   {314,1,0b00111},   {315,0,0b00101},   {316,1,0b11111},   {317,1,0b10111},   {319,1,0b10111},   {327,1,0b10111},   {328,1,0b11111},   {329,0,0b00011},   {330,1,0b11101},   {331,0,0b10001},   {332,0,0b10111},   {333,1,0b11001},   {334,0,0b10111},   {335,1,0b11111},   {336,1,0b11011},   {337,1,0b00011},   {338,0,0b00011},   {339,0,0b00111},   {340,1,0b10101},   {341,0,0b10011},   {342,1,0b10101},   {343,1,0b01001},   {344,1,0b10011},   {346,0,0b01001},   {347,1,0b11111},   {348,1,0b11011},   {350,1,0b00111},   {352,1,0b10001},   {354,1,0b11111},   {355,1,0b10101},   {358,1,0b00001},   {359,1,0b01011},   {361,0,0b00001},   {362,1,0b01001},   {364,1,0b11001},   {365,0,0b01001},   {366,1,0b01011},   {373,1,0b00111},   {377,0,0b00101},   {378,1,0b01101},   {379,0,0b00001},   {380,1,0b11001},   {381,1,0b10101},   {387,1,0b11101},   {389,1,0b01101},   {390,1,0b01101},   {395,1,0b00001},   {397,1,0b01001},   {399,1,0b11101},   {401,1,0b01011},   {402,1,0b11101},   {403,1,0b11111},   {404,1,0b00111},   {409,1,0b00111},   {410,1,0b11011},   {411,1,0b11111},   {412,0,0b01001},   {413,1,0b10101},   {418,1,0b10111},   {420,1,0b11011},   {430,0,0b00101},   {431,1,0b01101},   {435,0,0b10001},   {436,1,0b10111},   {437,1,0b11101},   {438,1,0b11111},   {440,1,0b01011},   {441,0,0b01001},   {442,0,0b10101},   {443,1,0b10111},   {446,1,0b01111},   {447,0,0b00101},   {448,0,0b01001},   {449,0,0b11011},   {450,1,0b11101},   {455,1,0b10011},   {458,1,0b00101},   {461,1,0b10111},   {463,1,0b11011},   {464,0,0b01111},   {465,1,0b01111},   {466,0,0b00101},   {467,0,0b11001},   {468,1,0b11111},   {469,1,0b10001},   {470,1,0b10001},   {471,1,0b01101},   {476,1,0b01111},   {478,0,0b00011},   {479,1,0b01001},   {480,1,0b01111},   {484,1,0b01101},   {489,1,0b01101},   {490,1,0b10101},   {492,1,0b00011},   {494,0,0b10111},   {495,1,0b11111},   {496,1,0b10101},   {504,1,0b10001},   {507,0,0b10101},   {508,1,0b11001},   {509,1,0b01111},   {513,1,0b10111},   {514,0,0b00001},   {515,1,0b11001},   {524,0,0b00001},   {525,0,0b10101},   {526,0,0b11011},   {527,1,0b11101},   {528,1,0b11111},   {531,1,0b10001},   {532,1,0b11001},   {534,1,0b10111},   {537,0,0b01101},   {538,1,0b10101},   {539,0,0b10101},   {540,1,0b11101},   {541,1,0b00001},   {542,1,0b01001},   {543,1,0b11111},   {544,1,0b11101},   {551,0,0b01111},   {552,0,0b10111},   {553,1,0b11111},   {554,1,0b00011},   {556,0,0b01011},   {557,0,0b10001},   {558,1,0b11101},   {559,1,0b00111},   {560,1,0b10001},   {561,1,0b10011},   {563,1,0b00011},   {564,0,0b00101},   {565,1,0b01011},   {566,1,0b10111},   {567,1,0b01001},   {568,1,0b11111},   {569,1,0b10001},   {571,1,0b00101},   {572,0,0b00101},   {573,0,0b11111},   {574,1,0b11111},   {575,1,0b11101},   {576,1,0b11011},   {580,1,0b01101},   {581,1,0b00111},   {588,1,0b01001},   {589,1,0b11001},   {590,1,0b01101},   {593,1,0b11111},   {594,0,0b01101},   {595,1,0b01111},   {598,1,0b01001},   {601,1,0b11001},   {602,1,0b00001},   {603,1,0b01011},   {606,1,0b00101},   {608,0,0b00011},   {609,1,0b11111},   {615,0,0b00101},   {616,1,0b10101},   {619,1,0b01001},   {621,1,0b10011},   {626,1,0b10011},   {627,0,0b10001},   {628,1,0b11111},   {635,1,0b00101},   {639,1,0b00101},   {643,1,0b11111},   {646,1,0b11111},   {650,1,0b10011},   {652,1,0b11001},   {657,0,0b10011},   {658,1,0b11011},   {659,1,0b10001},   {660,0,0b00001},   {661,1,0b01011},   {664,1,0b11101},   {666,1,0b10011},   {667,1,0b11111},   {673,1,0b11011},   {676,1,0b11001},   {682,0,0b10101},   {683,1,0b11111},   {686,1,0b01111},   {689,1,0b10011},   {691,1,0b10001},   {692,0,0b00111},   {693,1,0b10001},   {694,1,0b10101},   {695,1,0b01011},   {697,1,0b11111},   {699,0,0b11001},   {700,1,0b11011},   {701,1,0b10001},   {703,0,0b10101},   {704,1,0b11111},   {705,1,0b01111},   {707,1,0b11101},   {710,0,0b00101},   {711,1,0b00111},   {712,1,0b00101},   {713,0,0b00011},   {714,1,0b01111},   {715,1,0b01011},   {718,0,0b00001},   {719,1,0b01101},   {724,1,0b10101},   {725,1,0b11111},   {726,1,0b00001},   {729,0,0b10001},   {730,1,0b10101},   {731,1,0b10001},   {732,1,0b10001},   {733,1,0b11101},   {738,1,0b11011},   {740,1,0b10011},   {741,0,0b00111},   {742,1,0b01101},   {744,0,0b00001},   {745,1,0b00111},   {747,1,0b01011},   {750,1,0b00001},   {754,1,0b11001},   {761,0,0b10101},   {762,1,0b10101},   {763,1,0b00111},   {765,1,0b01111},   {766,1,0b10001},   {768,0,0b00001},   {769,1,0b00101},   {774,0,0b10111},   {775,1,0b10111},   {777,0,0b00101},   {778,0,0b01001},   {779,1,0b11111},   {780,1,0b01011},   {781,0,0b10011},   {782,0,0b10011},   {783,1,0b11111},   {784,1,0b11101},   {785,0,0b00001},   {786,1,0b11111},   {787,1,0b01101},   {788,1,0b01001},   {789,1,0b01011},   {792,0,0b01001},   {793,0,0b01011},   {794,1,0b11101},   {795,0,0b01001},   {796,1,0b10001},   {797,1,0b10011},   {799,1,0b10111},   {800,0,0b01101},   {801,1,0b11111},   {803,1,0b01001},   {804,1,0b10111},   {806,0,0b00111},   {807,1,0b10001},   {809,0,0b01001},   {810,0,0b01011},   {811,1,0b10111},   {815,1,0b10001},   {816,0,0b01001},   {817,1,0b10101},   {821,1,0b00011},   {823,1,0b11011},   {824,1,0b11101},   {827,1,0b00001},   {829,0,0b11011},   {830,1,0b11111},   {835,1,0b01101},   {836,1,0b00101},   {837,1,0b01101},   {839,1,0b10011},   {841,1,0b00011},   {842,1,0b10101},   {843,1,0b01011},   {849,1,0b00001},   {851,1,0b10001},   {852,1,0b10001},   {854,1,0b01101},   {855,1,0b00111},   {857,1,0b10111},   {858,1,0b00011},   {859,0,0b00101},   {860,0,0b10101},   {861,1,0b11011},   {862,1,0b01101},   {863,1,0b00001},   {864,1,0b00001},   {865,1,0b00001},   {866,0,0b01011},   {867,1,0b10001},   {868,0,0b00011},   {869,0,0b00111},   {870,1,0b11001},   {871,1,0b00101},   {872,0,0b00111},   {873,1,0b11111},   {874,0,0b10011},   {875,0,0b10011},   {876,1,0b11101},   {877,0,0b10101},   {878,1,0b10101},   {880,1,0b11011},   {883,1,0b01111},   {886,0,0b00101},   {887,1,0b01001},   {888,1,0b00111},   {894,0,0b00111},   {895,1,0b10101},   {896,1,0b11111},   {897,1,0b11101},   {903,1,0b10001},   {907,1,0b00001},   {908,0,0b01111},   {909,1,0b01111},   {910,1,0b10111},   {911,0,0b10001},   {912,1,0b10011},   {914,0,0b00101},   {915,1,0b11001},   {916,1,0b10101},   {917,0,0b11011},   {918,1,0b11111},   {919,1,0b11111},   {922,0,0b00101},   {923,1,0b10001},   {925,1,0b10101},   {931,1,0b11001},   {932,1,0b01101},   {934,1,0b11001},   {935,0,0b10101},   {936,1,0b11111},   {937,1,0b00001},   {938,1,0b10011},   {939,1,0b01001},   {940,1,0b01101},   {947,1,0b00101},   {950,1,0b10111},   {953,0,0b00011},   {954,1,0b01111},   {955,1,0b10011},   {957,0,0b11111},   {958,1,0b11111},   {959,1,0b10011},   {960,1,0b10001},   {962,0,0b00111},   {963,0,0b01111},   {964,1,0b10011},   {965,1,0b11111},   {970,1,0b01001},   {971,1,0b10011},   {975,0,0b00111},   {976,1,0b01111},   {977,1,0b01111},   {978,1,0b01011},   {982,1,0b11101},   {984,0,0b10001},   {985,1,0b10011},   {986,1,0b00011},   {991,0,0b11011},   {992,1,0b11101},   {995,1,0b00011},   {1002,1,0b00101},   {1003,0,0b01011},   {1004,1,0b10011},   {1006,1,0b10011},   {1009,1,0b11001},   {1013,1,0b10011},   {1015,1,0b01101},   {1020,1,0b01001},   {1024,0,0b00101},   {1025,1,0b10111},   {1027,1,0b00101},   {1028,1,0b00001},   {1031,0,0b00001},   {1032,0,0b10101},   {1033,1,0b11001},   {1034,0,0b01011},   {1035,0,0b01011},   {1036,1,0b11001},   {1037,1,0b10001},   {1043,0,0b00101},   {1044,0,0b10011},   {1045,1,0b10011},   {1046,1,0b00111},   {1047,1,0b11011},   {1050,1,0b01111},   {1058,1,0b11111},   {1063,1,0b01101},   {1067,0,0b01101},   {1068,1,0b11101},   {1069,1,0b11111},   {1070,0,0b00111},   {1071,1,0b01011},   {1073,1,0b01101}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_int_iter_skip1(it);
                {
                    const uint8_t expected_boundary[] = {0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111};
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }
                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, nullptr, 0);
                {
                    const uint8_t expected_boundary[] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_iter_skip1(it);
                {
                    const uint8_t expected_boundary[] = {0b00000000, 0b00010000, 0b00001100, 0b00111101, 0b11110111, 0b11101011, 0b00011100, 0b10100010};
                    const std::vector<uint32_t> occupieds_pos = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 20, 22, 23, 25, 27, 28, 29, 32, 34, 35, 36, 37, 38, 39, 40, 41, 42, 44, 45, 46, 47, 49, 51, 52, 54, 55, 56, 57, 59, 60, 61, 63, 64, 70, 71, 72, 73, 74, 75, 78, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 92, 94, 95, 98, 99, 100, 101, 102, 103, 104, 107, 108, 111, 113, 114, 115, 116, 117, 118, 120, 121, 122, 123, 125, 126, 127, 131, 132, 133, 134, 135, 137, 138, 139, 141, 144, 145, 146, 147, 148, 149, 151, 152, 153, 154, 155, 157, 159, 160, 162, 164, 165, 167, 168, 170, 174, 175, 176, 177, 179, 180, 181, 182, 184, 185, 186, 187, 189, 190, 192, 196, 197, 199, 201, 202, 203, 204, 205, 206, 207, 208, 210, 212, 213, 214, 216, 218, 220, 221, 222, 223, 225, 226, 228, 229, 230, 231, 234, 235, 237, 238, 239, 241, 242, 243, 244, 246, 248, 249, 250, 251, 253, 254, 255, 256, 257, 259, 261, 262, 263, 264, 265, 267, 268, 270, 271, 272, 274, 276, 277, 278, 280, 281, 282, 283, 285, 287, 288, 289, 291, 292, 293, 294, 296, 297, 298, 301, 302, 303, 304, 305, 308, 309, 310, 311, 313, 314, 315, 320, 321, 322, 323, 325, 326, 327, 328, 331, 332, 334, 335, 336, 338, 339, 340, 341, 342, 344, 345, 346, 348, 349, 351, 354, 355, 356, 357, 358, 359, 360, 363, 364, 365, 366, 367, 369, 371, 372, 373, 374, 375, 376, 378, 379, 381, 382, 383, 384, 385, 386, 388, 389, 390, 391, 394, 395, 396, 397, 398, 399, 400, 402, 403, 404, 406, 408, 409, 410, 411, 414, 415, 416, 417, 419, 420, 421, 422, 424, 428, 429, 430, 432, 433, 435, 437, 438, 439, 440, 441, 443, 444, 445, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 459, 460, 461, 462, 463, 464, 466, 467, 468, 469, 470, 471, 472, 473, 474, 476, 477, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 492, 494, 495, 496, 498, 499, 500, 501, 502, 503, 504, 506, 508, 509, 510, 511, 512, 513, 514, 516, 517, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 531, 532, 535, 537, 538, 540, 541, 542, 546, 548, 549, 550, 551, 552, 553, 554, 555, 557, 558, 559, 561, 562, 564, 565, 566, 568, 570, 572, 573, 574, 575, 578, 579, 580, 584, 585, 586, 587, 588, 589, 590, 591, 594, 595, 596, 597, 598, 599, 601, 602, 603, 604, 607, 608, 612, 613, 614, 616, 617, 618, 619, 620, 621, 623, 624, 625, 628, 629, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650, 651, 653, 655, 657, 658, 659, 660, 662, 664, 665, 666, 668, 669, 670, 672, 673, 674, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 689, 690, 691, 692, 693, 694, 697, 698, 700, 701, 702, 703, 705, 706, 707, 708, 709, 710, 711, 713, 714, 715, 718, 720, 721, 722, 724, 725, 727, 728, 730, 731, 732, 733, 736, 737, 738, 744, 745, 746, 747, 748, 750, 753, 754, 755, 756, 758, 759, 761, 762, 763, 765, 767, 768, 769, 770, 771, 772, 773, 774, 775, 777, 778, 779, 780, 781, 783, 784};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,0,0b00001},   {  2,0,0b01101},   {  3,0,0b10011},   {  4,0,0b10101},   {  5,1,0b11111},   {  6,1,0b11011},   {  7,0,0b10101},   {  8,1,0b10111},   {  9,1,0b01101},   { 10,0,0b00011},   { 11,0,0b10101},   { 12,1,0b11101},   { 13,1,0b00011},   { 14,0,0b00001},   { 15,0,0b00001},   { 16,1,0b01101},   { 17,1,0b10011},   { 18,0,0b10101},   { 19,1,0b11011},   { 20,1,0b00011},   { 21,1,0b01011},   { 22,0,0b00111},   { 23,0,0b01011},   { 24,0,0b11011},   { 25,1,0b11111},   { 26,0,0b00101},   { 27,1,0b11101},   { 28,1,0b01001},   { 29,1,0b10001},   { 30,1,0b10011},   { 31,1,0b11011},   { 32,0,0b00001},   { 33,0,0b00111},   { 34,0,0b11011},   { 35,0,0b11011},   { 36,1,0b11101},   { 37,1,0b00101},   { 38,1,0b11111},   { 39,0,0b01001},   { 40,0,0b10011},   { 41,0,0b10011},   { 42,1,0b11111},   { 43,1,0b11101},   { 44,0,0b01101},   { 45,0,0b01101},   { 46,1,0b11101},   { 47,1,0b01101},   { 48,0,0b00011},   { 49,0,0b11001},   { 50,1,0b11011},   { 51,1,0b01001},   { 52,1,0b00111},   { 53,0,0b00011},   { 54,1,0b00111},   { 55,1,0b10011},   { 56,0,0b00001},   { 57,1,0b11001},   { 58,0,0b00001},   { 59,0,0b00001},   { 60,1,0b01001},   { 61,0,0b00101},   { 62,0,0b10011},   { 63,1,0b10111},   { 64,1,0b00011},   { 65,1,0b00111},   { 66,0,0b10001},   { 67,1,0b10001},   { 68,1,0b01101},   { 69,0,0b00001},   { 70,0,0b01101},   { 71,1,0b11011},   { 72,0,0b01101},   { 73,0,0b10111},   { 74,0,0b11001},   { 75,1,0b11101},   { 76,0,0b00111},   { 77,0,0b00111},   { 78,0,0b01011},   { 79,1,0b11011},   { 80,0,0b01001},   { 81,0,0b01101},   { 82,0,0b01101},   { 83,1,0b11111},   { 84,1,0b11111},   { 85,0,0b10011},   { 86,0,0b11001},   { 87,1,0b11001},   { 88,1,0b00101},   { 89,0,0b00001},   { 90,1,0b00111},   { 91,1,0b01011},   { 92,0,0b01011},   { 93,0,0b11011},   { 94,1,0b11011},   { 95,1,0b10111},   { 96,0,0b01001},   { 97,1,0b10111},   { 98,0,0b10011},   { 99,1,0b11011},   {100,1,0b11001},   {101,1,0b00101},   {102,1,0b11001},   {103,1,0b01101},   {104,0,0b10111},   {105,1,0b11111},   {106,1,0b11001},   {109,0,0b00101},   {110,1,0b01111},   {111,1,0b01001},   {112,0,0b11101},   {113,1,0b11101},   {114,0,0b01111},   {115,0,0b10001},   {116,1,0b11101},   {117,0,0b00101},   {118,1,0b11001},   {119,1,0b11011},   {120,1,0b00111},   {121,0,0b01011},   {122,0,0b01011},   {123,0,0b01101},   {124,1,0b11111},   {125,1,0b10101},   {126,0,0b10011},   {127,1,0b10101},   {128,1,0b11001},   {129,0,0b00011},   {130,1,0b10111},   {131,1,0b01111},   {134,1,0b01011},   {135,1,0b00111},   {137,1,0b10111},   {138,0,0b01001},   {139,1,0b01111},   {140,1,0b00101},   {141,0,0b01001},   {142,1,0b01011},   {143,0,0b01111},   {144,1,0b10011},   {146,1,0b01011},   {148,0,0b00001},   {149,0,0b10011},   {150,1,0b11001},   {152,1,0b01011},   {154,0,0b10001},   {155,0,0b10101},   {156,1,0b10101},   {157,1,0b10011},   {158,1,0b11001},   {159,0,0b10001},   {160,0,0b10101},   {161,1,0b11111},   {162,0,0b00001},   {163,0,0b00111},   {164,1,0b01011},   {165,1,0b01011},   {166,1,0b11101},   {167,0,0b00111},   {168,1,0b01011},   {169,0,0b00101},   {170,1,0b10101},   {171,1,0b11101},   {172,0,0b00101},   {173,1,0b11001},   {174,0,0b00111},   {175,1,0b11101},   {176,0,0b00111},   {177,1,0b11001},   {179,0,0b01001},   {180,1,0b11011},   {181,0,0b00011},   {182,1,0b11111},   {183,1,0b11111},   {184,0,0b10111},   {185,1,0b10111},   {186,0,0b00101},   {187,1,0b11101},   {188,0,0b10001},   {189,0,0b10101},   {190,1,0b11111},   {191,0,0b01111},   {192,0,0b10111},   {193,1,0b11111},   {194,1,0b10111},   {195,1,0b01011},   {197,0,0b01011},   {198,1,0b10101},   {199,1,0b10111},   {200,0,0b10101},   {201,1,0b11001},   {202,1,0b00101},   {203,1,0b11011},   {204,1,0b01011},   {207,0,0b00001},   {208,0,0b01011},   {209,0,0b01011},   {210,1,0b01111},   {211,1,0b11111},   {212,1,0b10111},   {213,1,0b01111},   {214,1,0b11011},   {215,0,0b01011},   {216,1,0b01101},   {218,1,0b01001},   {219,0,0b11011},   {220,0,0b11011},   {221,1,0b11111},   {222,1,0b10101},   {224,1,0b01011},   {226,1,0b11111},   {229,1,0b11001},   {230,1,0b01011},   {233,0,0b01011},   {234,1,0b10111},   {238,1,0b01111},   {239,0,0b11101},   {240,0,0b11101},   {241,0,0b11111},   {242,1,0b11111},   {243,0,0b00001},   {244,0,0b01001},   {245,1,0b10101},   {246,1,0b01111},   {247,0,0b00011},   {248,1,0b11001},   {249,0,0b00011},   {250,0,0b00101},   {251,1,0b11011},   {252,0,0b00101},   {253,1,0b01111},   {254,0,0b00001},   {255,0,0b11001},   {256,1,0b11111},   {257,0,0b00001},   {258,0,0b01011},   {259,1,0b11101},   {260,1,0b01011},   {261,0,0b01101},   {262,1,0b01111},   {263,0,0b11011},   {264,1,0b11101},   {265,1,0b00011},   {266,0,0b00011},   {267,1,0b11011},   {268,0,0b00101},   {269,1,0b10001},   {270,1,0b11101},   {271,1,0b11001},   {272,0,0b01101},   {273,1,0b11001},   {275,1,0b10101},   {277,0,0b11001},   {278,1,0b11001},   {279,1,0b10101},   {280,0,0b00011},   {281,1,0b00101},   {282,1,0b00101},   {283,0,0b10001},   {284,1,0b11111},   {285,1,0b10101},   {286,1,0b10111},   {287,0,0b00101},   {288,1,0b01101},   {290,1,0b01011},   {292,1,0b01111},   {293,1,0b00111},   {296,0,0b01011},   {297,1,0b01101},   {298,1,0b11001},   {301,0,0b00001},   {302,1,0b10101},   {303,0,0b00001},   {304,1,0b01011},   {305,1,0b11001},   {306,1,0b01011},   {308,0,0b01001},   {309,1,0b10001},   {310,1,0b01001},   {312,0,0b01011},   {313,1,0b01011},   {314,0,0b01011},   {315,0,0b01011},   {316,1,0b01111},   {317,0,0b00011},   {318,1,0b11011},   {319,1,0b10011},   {320,0,0b10001},   {321,1,0b11001},   {322,1,0b00001},   {325,0,0b01101},   {326,0,0b01101},   {327,1,0b11001},   {328,0,0b01101},   {329,1,0b10111},   {330,0,0b00001},   {331,1,0b10011},   {332,0,0b00011},   {333,1,0b10111},   {334,1,0b11101},   {335,0,0b01111},   {336,0,0b10111},   {337,1,0b11001},   {338,1,0b10011},   {339,1,0b01101},   {340,1,0b01011},   {341,1,0b11111},   {342,0,0b01001},   {343,1,0b11001},   {344,1,0b00001},   {346,0,0b10001},   {347,0,0b10111},   {348,1,0b11101},   {349,0,0b00111},   {350,1,0b10001},   {351,1,0b10001},   {352,0,0b01011},   {353,1,0b11001},   {354,1,0b00011},   {355,1,0b00111},   {357,0,0b10011},   {358,1,0b11101},   {359,1,0b00011},   {360,0,0b01111},   {361,1,0b10001},   {362,1,0b01111},   {363,1,0b10101},   {366,1,0b11111},   {367,1,0b00111},   {370,1,0b11001},   {371,1,0b11101},   {373,1,0b00011},   {375,1,0b00101},   {378,1,0b00111},   {379,1,0b11111},   {381,0,0b01001},   {382,0,0b10011},   {383,0,0b11001},   {384,1,0b11011},   {385,0,0b00111},   {386,1,0b01101},   {387,0,0b00011},   {388,0,0b00101},   {389,0,0b01101},   {390,0,0b10111},   {391,0,0b11001},   {392,1,0b11111},   {393,1,0b10111},   {394,1,0b00001},   {395,0,0b01101},   {396,1,0b01101},   {397,0,0b00001},   {398,0,0b01011},   {399,1,0b10111},   {400,1,0b11111},   {401,0,0b00101},   {402,1,0b01001},   {403,1,0b01101},   {404,0,0b10001},   {405,0,0b10011},   {406,1,0b10101},   {407,1,0b01011},   {408,1,0b10001},   {409,0,0b01011},   {410,1,0b01111},   {411,1,0b11011},   {412,0,0b00101},   {413,1,0b10001},   {414,0,0b00111},   {415,0,0b11001},   {416,0,0b11001},   {417,1,0b11101},   {418,1,0b01001},   {419,0,0b00111},   {420,1,0b11011},   {421,1,0b11101},   {422,1,0b01111},   {423,1,0b11001},   {424,0,0b00011},   {425,1,0b10111},   {426,0,0b00101},   {427,1,0b11001},   {428,0,0b10011},   {429,1,0b10111},   {430,1,0b10011},   {431,1,0b01011},   {432,0,0b01111},   {433,1,0b11111},   {438,1,0b10111},   {440,1,0b00011},   {441,1,0b10011},   {442,1,0b01111},   {445,1,0b11111},   {446,1,0b01001},   {447,0,0b00001},   {448,0,0b00001},   {449,0,0b01101},   {450,0,0b01111},   {451,0,0b10101},   {452,0,0b11101},   {453,1,0b11111},   {454,0,0b10001},   {455,1,0b11111},   {456,0,0b10001},   {457,1,0b11101},   {458,0,0b00101},   {459,1,0b01001},   {460,0,0b00101},   {461,0,0b10011},   {462,0,0b11001},   {463,0,0b11101},   {464,1,0b11101},   {465,1,0b01111},   {466,0,0b00011},   {467,1,0b10111},   {468,1,0b00001},   {469,1,0b01111},   {470,1,0b00111},   {471,1,0b10011},   {472,0,0b11001},   {473,0,0b11001},   {474,1,0b11101},   {475,1,0b10111},   {476,1,0b11111},   {477,0,0b00011},   {478,1,0b01001},   {479,0,0b00001},   {480,0,0b10101},   {481,1,0b11101},   {482,0,0b10001},   {483,1,0b10011},   {484,1,0b00001},   {485,1,0b00001},   {486,0,0b01111},   {487,1,0b01111},   {488,1,0b00111},   {489,0,0b00011},   {490,0,0b00101},   {491,0,0b01101},   {492,1,0b10001},   {493,0,0b00011},   {494,1,0b10111},   {495,0,0b01001},   {496,1,0b10101},   {497,0,0b00111},   {498,1,0b01011},   {499,0,0b00001},   {500,1,0b10001},   {501,0,0b01011},   {502,0,0b10011},   {503,1,0b11111},   {504,1,0b11011},   {505,1,0b10011},   {506,1,0b10001},   {507,0,0b00111},   {508,1,0b10001},   {509,1,0b01101},   {510,0,0b10111},   {511,1,0b11101},   {512,1,0b11101},   {513,0,0b00111},   {514,0,0b11001},   {515,1,0b11101},   {516,1,0b01011},   {517,0,0b10001},   {518,1,0b10111},   {519,0,0b00011},   {520,1,0b00111},   {521,1,0b00101},   {522,0,0b10101},   {523,1,0b11001},   {524,0,0b00101},   {525,1,0b10101},   {526,1,0b10001},   {527,0,0b01001},   {528,1,0b11111},   {529,0,0b01101},   {530,1,0b01111},   {531,1,0b11001},   {532,0,0b00101},   {533,1,0b00111},   {534,1,0b01011},   {535,0,0b00111},   {536,0,0b10001},   {537,0,0b10101},   {538,1,0b10111},   {539,0,0b00011},   {540,0,0b01001},   {541,1,0b10011},   {542,1,0b11101},   {543,0,0b01001},   {544,0,0b01001},   {545,0,0b01011},   {546,1,0b11011},   {547,0,0b01101},   {548,0,0b10101},   {549,1,0b10101},   {550,0,0b00001},   {551,0,0b00101},   {552,1,0b01111},   {553,1,0b01011},   {554,0,0b10101},   {555,1,0b11101},   {556,1,0b10011},   {557,0,0b01001},   {558,0,0b10001},   {559,1,0b10101},   {560,0,0b00101},   {561,0,0b01111},   {562,0,0b10101},   {563,0,0b10111},   {564,1,0b11101},   {565,1,0b01111},   {566,1,0b00101},   {567,0,0b00101},   {568,0,0b00111},   {569,1,0b01001},   {570,1,0b01011},   {571,1,0b01111},   {572,1,0b00101},   {573,1,0b00111},   {574,1,0b11011},   {575,0,0b00111},   {576,1,0b10101},   {577,1,0b01111},   {578,1,0b11001},   {579,1,0b00001},   {580,1,0b00001},   {581,1,0b00001},   {582,0,0b01011},   {583,1,0b11011},   {584,0,0b00011},   {585,0,0b01001},   {586,0,0b10011},   {587,1,0b11011},   {588,0,0b10001},   {589,1,0b11001},   {590,1,0b00101},   {591,1,0b10111},   {592,0,0b10011},   {593,0,0b10101},   {594,1,0b11001},   {595,0,0b10011},   {596,1,0b10101},   {597,1,0b10011},   {598,1,0b11001},   {599,0,0b00111},   {600,0,0b10101},   {601,0,0b10101},   {602,1,0b11101},   {603,0,0b10011},   {604,1,0b10011},   {605,1,0b01001},   {606,1,0b01011},   {607,1,0b00111},   {608,0,0b00001},   {609,0,0b00011},   {610,0,0b10001},   {611,1,0b11001},   {612,0,0b00001},   {613,0,0b01011},   {614,1,0b11001},   {615,1,0b00111},   {616,1,0b11001},   {617,0,0b00111},   {618,0,0b01101},   {619,0,0b10001},   {620,1,0b10001},   {621,1,0b11011},   {622,0,0b01101},   {623,1,0b11101},   {624,0,0b00011},   {625,1,0b01111},   {626,1,0b11101},   {627,0,0b00111},   {628,1,0b01001},   {629,0,0b00001},   {630,1,0b11011},   {631,0,0b00101},   {632,1,0b10111},   {633,0,0b00111},   {634,1,0b11011},   {635,1,0b11101},   {636,1,0b00011},   {637,1,0b01111},   {638,0,0b00011},   {639,0,0b01101},   {640,1,0b11011},   {641,1,0b00011},   {642,0,0b00111},   {643,0,0b01001},   {644,0,0b01011},   {645,0,0b10111},   {646,0,0b11001},   {647,1,0b11101},   {648,1,0b01011},   {649,1,0b10111},   {650,1,0b00011},   {651,0,0b00101},   {652,0,0b10111},   {653,0,0b10111},   {654,1,0b11011},   {655,0,0b01011},   {656,1,0b11011},   {657,0,0b00111},   {658,0,0b01101},   {659,0,0b10011},   {660,1,0b11111},   {661,1,0b01011},   {662,0,0b10101},   {663,1,0b11011},   {664,0,0b01011},   {665,0,0b01111},   {666,1,0b11001},   {667,1,0b11011},   {668,1,0b00111},   {669,1,0b00101},   {670,1,0b01001},   {671,1,0b01011},   {672,1,0b11101},   {673,1,0b10011},   {674,0,0b01001},   {675,1,0b11001},   {676,0,0b00101},   {677,1,0b01111},   {678,0,0b00001},   {679,1,0b10001},   {680,0,0b00101},   {681,1,0b11011},   {682,1,0b10101},   {683,0,0b01111},   {684,0,0b10111},   {685,1,0b11111},   {686,0,0b00101},   {687,0,0b01011},   {688,0,0b10011},   {689,1,0b11011},   {690,1,0b01001},   {691,0,0b00101},   {692,1,0b11001},   {693,0,0b10011},   {694,1,0b10101},   {695,1,0b10001},   {696,1,0b10111},   {697,1,0b00111},   {698,1,0b11001},   {699,1,0b00001},   {700,1,0b01101},   {701,1,0b11111},   {702,1,0b00001},   {703,0,0b00111},   {704,1,0b11001},   {705,1,0b10001},   {706,0,0b11101},   {707,1,0b11101},   {708,0,0b10111},   {709,1,0b11111},   {710,0,0b00001},   {711,0,0b00101},   {712,1,0b01011},   {713,0,0b00011},   {714,1,0b10111},   {715,0,0b00011},   {716,1,0b00101},   {717,0,0b01001},   {718,0,0b10101},   {719,1,0b11111},   {720,0,0b01011},   {721,1,0b10111},   {722,1,0b01111},   {723,0,0b01011},   {724,1,0b10101},   {725,0,0b01111},   {726,1,0b11011},   {727,0,0b00111},   {728,0,0b10011},   {729,1,0b10111},   {730,0,0b00111},   {731,0,0b10001},   {732,1,0b10011},   {733,1,0b00001},   {734,0,0b01111},   {735,0,0b01111},   {736,1,0b10001},   {737,1,0b01001},   {738,0,0b00001},   {739,1,0b00111},   {740,1,0b11011},   {741,0,0b10001},   {742,0,0b10001},   {743,0,0b10001},   {744,1,0b10111},   {745,1,0b10001},   {746,1,0b10001},   {747,0,0b01001},   {748,0,0b01101},   {749,0,0b11001},   {750,1,0b11011},   {751,1,0b10001},   {752,0,0b00001},   {753,0,0b00011},   {754,1,0b00101},   {755,1,0b00111},   {756,0,0b00011},   {757,1,0b01001},   {758,1,0b00111},   {759,0,0b00001},   {760,1,0b11101},   {761,1,0b01111},   {762,1,0b00111},   {763,0,0b01001},   {764,1,0b10001},   {765,1,0b00111},   {766,1,0b01011},   {767,1,0b10011},   {768,0,0b01101},   {769,1,0b10001},   {770,1,0b10001},   {771,0,0b00011},   {772,0,0b00101},   {773,1,0b11101},   {774,1,0b10111},   {775,1,0b10101},   {776,1,0b10001},   {777,1,0b11101},   {778,0,0b11001},   {779,1,0b11101},   {780,0,0b01011},   {781,1,0b11001},   {782,0,0b10001},   {783,1,0b10101},   {784,1,0b00001},   {785,0,0b10001},   {786,1,0b11101},   {787,1,0b11111},   {788,0,0b00011},   {789,1,0b10001},   {790,1,0b01011},   {791,1,0b10101},   {792,0,0b11001},   {793,1,0b11101},   {794,0,0b10001},   {795,1,0b10101},   {796,1,0b01001},   {797,1,0b11011},   {798,0,0b00111},   {799,0,0b01001},   {800,0,0b10111},   {801,1,0b11111},   {802,1,0b11101},   {803,1,0b10101},   {804,0,0b00011},   {805,1,0b10001},   {806,1,0b11111},   {807,1,0b10111},   {808,1,0b01111},   {809,0,0b01011},   {810,1,0b11011},   {811,1,0b10101},   {812,0,0b00011},   {813,1,0b01101},   {814,0,0b00101},   {815,0,0b01001},   {816,1,0b10001},   {817,0,0b01101},   {818,1,0b11111},   {819,1,0b01001},   {820,0,0b10101},   {821,0,0b10111},   {822,1,0b11111},   {823,0,0b00001},   {824,1,0b10011},   {825,1,0b00011},   {826,1,0b01101},   {827,1,0b11011},   {828,1,0b00001},   {829,1,0b11011},   {830,1,0b10011},   {831,0,0b01101},   {832,1,0b10001},   {833,0,0b00011},   {834,1,0b10011},   {835,0,0b00101},   {836,1,0b01001},   {837,0,0b01111},   {838,1,0b10011},   {839,0,0b10011},   {840,1,0b11111},   {841,1,0b01001},   {842,1,0b01101},   {843,0,0b01111},   {844,1,0b10101},   {845,0,0b00011},   {846,1,0b11111},   {847,1,0b00111},   {848,0,0b00001},   {849,0,0b00111},   {850,0,0b10101},   {851,0,0b10111},   {852,1,0b11101},   {853,0,0b01111},   {854,1,0b11001},   {855,1,0b10001},   {856,1,0b01111},   {857,0,0b10111},   {858,0,0b11001},   {859,1,0b11001},   {860,0,0b10101},   {861,1,0b11111},   {862,1,0b10011},   {863,0,0b01011},   {864,1,0b11011},   {865,1,0b11011},   {866,0,0b10011},   {867,1,0b10101},   {868,0,0b00101},   {869,1,0b00101},   {870,0,0b00011},   {871,0,0b10111},   {872,1,0b11011},   {873,0,0b01001},   {874,0,0b01001},   {875,1,0b11101},   {876,0,0b01001},   {877,0,0b10101},   {878,1,0b11101},   {879,0,0b11001},   {880,0,0b11111},   {881,1,0b11111},   {882,0,0b01011},   {883,1,0b11101},   {884,0,0b00111},   {885,1,0b10111},   {886,0,0b00101},   {887,1,0b01011},   {888,0,0b00001},   {889,1,0b00011},   {890,0,0b00111},   {891,0,0b01001},   {892,1,0b01101},   {893,0,0b00011},   {894,1,0b10011},   {895,1,0b01111},   {896,1,0b10111},   {897,0,0b01001},   {898,0,0b01111},   {899,0,0b10011},   {900,1,0b11111},   {901,1,0b11001},   {902,0,0b00011},   {903,1,0b10001},   {904,1,0b01011},   {905,0,0b00101},   {906,1,0b01001},   {907,0,0b10011},   {908,0,0b10111},   {909,0,0b11011},   {910,1,0b11111},   {911,1,0b11111},   {912,1,0b00111},   {913,1,0b01101},   {914,1,0b11101},   {915,0,0b10101},   {916,0,0b11011},   {917,1,0b11011},   {918,0,0b01111},   {919,0,0b10001},   {920,1,0b11101},   {921,0,0b01011},   {922,1,0b10111},   {923,0,0b00001},   {924,0,0b01011},   {925,1,0b10001},   {926,0,0b00011},   {927,1,0b01101},   {928,1,0b11001},   {929,0,0b00111},   {930,0,0b01101},   {931,0,0b11011},   {932,1,0b11111},   {933,1,0b11011},   {934,1,0b11011},   {935,0,0b10101},   {936,1,0b11001},   {937,1,0b00111},   {938,1,0b01011},   {939,0,0b00101},   {940,1,0b00111},   {941,1,0b01001},   {942,1,0b11111},   {943,0,0b01101},   {944,1,0b11101},   {945,0,0b00001},   {946,1,0b11001},   {947,0,0b01001},   {948,1,0b01111},   {949,1,0b00001},   {950,0,0b00101},   {951,0,0b01111},   {952,0,0b11001},   {953,1,0b11001},   {954,0,0b10001},   {955,1,0b11011},   {956,0,0b01101},   {957,1,0b01101},   {958,1,0b01101},   {959,0,0b00111},   {960,0,0b01011},   {961,1,0b10011},   {962,0,0b00111},   {963,0,0b10001},   {964,0,0b10101},   {965,1,0b11001},   {966,1,0b11011},   {967,1,0b11101},   {968,1,0b01001},   {969,0,0b00101},   {970,0,0b00111},   {971,1,0b10111},   {972,1,0b00001},   {973,0,0b01101},   {974,1,0b10101},   {975,1,0b11111},   {976,1,0b01111},   {977,0,0b00101},   {978,0,0b10001},   {979,1,0b11111},   {980,1,0b10111},   {981,0,0b01011},   {982,0,0b01101},   {983,1,0b10101},   {984,1,0b11101},   {985,1,0b10011},   {986,0,0b10101},   {987,1,0b11111},   {988,0,0b10011},   {989,1,0b11111},   {990,0,0b00111},   {991,1,0b10111},   {992,0,0b10101},   {993,0,0b10101},   {994,1,0b11011},   {995,0,0b01001},   {996,1,0b10101},   {997,1,0b01011},   {998,1,0b10101},   {999,1,0b11001},   {1000,1,0b01111},   {1001,1,0b00011},   {1002,0,0b00001},   {1003,0,0b00011},   {1004,0,0b00101},   {1005,1,0b01011},   {1006,1,0b00101},   {1007,1,0b00101},   {1008,0,0b00111},   {1009,1,0b01011},   {1010,0,0b11001},   {1011,1,0b11111},   {1012,0,0b10101},   {1013,0,0b11101},   {1014,1,0b11111},   {1015,1,0b10101},   {1016,0,0b01011},   {1017,0,0b10111},   {1018,1,0b11111},   {1019,1,0b11011},   {1020,1,0b10101},   {1021,0,0b01001},   {1022,0,0b10001},   {1023,1,0b11111},   {1024,1,0b11001},   {1025,0,0b00101},   {1026,0,0b10001},   {1027,1,0b11001},   {1028,0,0b10111},   {1029,1,0b10111},   {1030,0,0b11011},   {1031,1,0b11111},   {1032,1,0b11111},   {1033,0,0b01011},   {1034,0,0b01011},   {1035,1,0b10011},   {1036,0,0b01101},   {1037,1,0b10011},   {1038,0,0b00001},   {1039,1,0b01111},   {1040,1,0b00011},   {1041,0,0b10101},   {1042,1,0b11101},   {1043,0,0b11001},   {1044,1,0b11011},   {1045,1,0b01101},   {1046,0,0b00101},   {1047,0,0b01011},   {1048,0,0b10101},   {1049,0,0b11101},   {1050,1,0b11101},   {1051,0,0b00001},   {1052,0,0b01101},   {1053,1,0b10001},   {1054,0,0b00101},   {1055,0,0b00111},   {1056,1,0b01001},   {1057,0,0b11101},   {1058,1,0b11101},   {1059,1,0b10001},   {1060,1,0b10101},   {1061,0,0b00101},   {1062,1,0b10101},   {1063,0,0b00011},   {1064,0,0b00101},   {1065,1,0b10111},   {1066,0,0b01011},   {1067,1,0b01011},   {1068,0,0b00101},   {1069,1,0b01011},   {1070,1,0b01001},   {1071,0,0b00011},   {1072,1,0b00101},   {1073,0,0b10001},   {1074,1,0b11101},   {1075,1,0b01011},   {1076,1,0b11001}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_iter_skip1(it);
                {
                    const uint8_t expected_boundary[] = {0b01100010, 0b00101001, 0b11111000, 0b10110111, 0b10010000, 0b00101011, 0b01101111, 0b10111011};
                    const std::vector<uint32_t> occupieds_pos = {0, 2, 4, 6, 7, 8, 9, 10, 12, 13, 14, 15, 17, 18, 20, 21, 23, 26, 27, 28, 29, 30, 31, 32, 33, 34, 36, 37, 39, 40, 41, 42, 43, 44, 47, 49, 50, 52, 53, 54, 55, 56, 57, 58, 60, 63, 64, 65, 66, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 95, 96, 97, 98, 100, 101, 102, 104, 105, 107, 108, 109, 110, 111, 112, 113, 115, 116, 118, 119, 120, 121, 122, 123, 125, 126, 127, 128, 131, 134, 135, 136, 138, 139, 140, 141, 142, 143, 145, 146, 147, 148, 150, 152, 154, 155, 156, 157, 158, 159, 160, 163, 164, 165, 166, 170, 171, 172, 173, 175, 176, 177, 179, 180, 181, 185, 186, 187, 188, 189, 190, 193, 195, 196, 198, 199, 200, 201, 202, 203, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 219, 220, 222, 223, 224, 225, 226, 227, 229, 230, 231, 232, 233, 234, 235, 236, 237, 239, 240, 241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 254, 255, 256, 257, 258, 260, 261, 262, 264, 265, 266, 268, 269, 271, 273, 274, 275, 276, 277, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 291, 292, 297, 298, 300, 301, 302, 303, 304, 305, 308, 309, 310, 311, 312, 313, 314, 316, 317, 318, 321, 322, 323, 324, 326, 327, 328, 329, 331, 332, 335, 338, 340, 341, 343, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 358, 361, 362, 364, 365, 367, 368, 369, 370, 371, 372, 373, 374, 377, 378, 380, 382, 383, 384, 387, 388, 389, 391, 392, 393, 394, 395, 396, 398, 399, 400, 401, 403, 404, 406, 409, 410, 411, 413, 415, 417, 418, 420, 421, 424, 425, 429, 430, 431, 432, 433, 434, 435, 437, 438, 439, 440, 442, 443, 444, 445, 446, 447, 448, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 461, 462, 463, 464, 466, 468, 471, 472, 474, 475, 477, 478, 479, 480, 481, 482, 484, 485, 486, 487, 489, 490, 491, 492, 493, 494, 497, 498, 499, 502, 503, 505, 506, 507, 508, 509, 511, 512, 513, 516, 517, 521, 523, 525, 526, 527, 528, 530, 532, 537, 538, 539, 541, 542, 543, 544, 545, 549, 551, 552, 553, 554, 556, 557, 558, 559, 560, 562, 564, 567, 568, 570, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 589, 590, 591, 592, 593, 594, 595, 596, 597, 598, 601, 602, 603, 604, 606, 607, 608, 609, 610, 612, 613, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 629, 630, 631, 632, 634, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 647, 649, 650, 651, 654, 655, 656, 657, 658, 659, 661, 662, 664, 668, 669, 670, 671, 673, 675, 676, 677, 678, 680, 681, 682, 683, 684, 686, 687, 688, 689, 691, 692, 693, 694, 698, 700, 702, 703, 704, 705, 706, 708, 709, 710, 714, 715, 717, 718, 719, 720, 721, 722, 724, 725, 730, 731, 732, 733, 734, 735, 736, 737, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753, 756, 757, 758, 762, 763, 765, 767, 768, 770, 771, 772, 774, 776, 777, 778, 779, 781, 783, 784, 786, 787, 789, 790, 792, 793, 796, 797, 800, 801, 802, 803, 805, 809, 811};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b11101},   {  1,1,0b11111},   {  2,0,0b10101},   {  3,1,0b11011},   {  5,0,0b01001},   {  6,0,0b01011},   {  7,1,0b10011},   {  8,0,0b00101},   {  9,1,0b10001},   { 10,0,0b01101},   { 11,1,0b11001},   { 12,0,0b00001},   { 13,1,0b11111},   { 14,1,0b10001},   { 15,1,0b00001},   { 16,0,0b01001},   { 17,1,0b11111},   { 18,0,0b01101},   { 19,1,0b01111},   { 20,0,0b01111},   { 21,0,0b10111},   { 22,1,0b11111},   { 23,1,0b01111},   { 24,0,0b00001},   { 25,0,0b01001},   { 26,1,0b11001},   { 27,1,0b10001},   { 28,1,0b10111},   { 29,1,0b01011},   { 30,0,0b00011},   { 31,1,0b11001},   { 34,0,0b01001},   { 35,0,0b01111},   { 36,0,0b10101},   { 37,1,0b11001},   { 38,0,0b00011},   { 39,0,0b00101},   { 40,0,0b01101},   { 41,0,0b10011},   { 42,1,0b11011},   { 43,1,0b11001},   { 44,1,0b00011},   { 45,1,0b01111},   { 46,1,0b11011},   { 47,1,0b11111},   { 48,1,0b10011},   { 49,1,0b11111},   { 50,1,0b00001},   { 51,1,0b10001},   { 52,0,0b01101},   { 53,0,0b01111},   { 54,1,0b10101},   { 55,1,0b10101},   { 56,0,0b10111},   { 57,1,0b11101},   { 58,1,0b10101},   { 59,0,0b00001},   { 60,1,0b10001},   { 61,1,0b01111},   { 62,0,0b01101},   { 63,0,0b10111},   { 64,1,0b11001},   { 65,0,0b01001},   { 66,0,0b01011},   { 67,1,0b01111},   { 68,1,0b00111},   { 69,1,0b00111},   { 70,0,0b00111},   { 71,1,0b01101},   { 72,1,0b11101},   { 73,1,0b10101},   { 74,0,0b10001},   { 75,1,0b11111},   { 76,1,0b11111},   { 77,0,0b01001},   { 78,0,0b10111},   { 79,1,0b11001},   { 80,0,0b00111},   { 81,1,0b11111},   { 83,0,0b01011},   { 84,1,0b10101},   { 85,0,0b00001},   { 86,0,0b01101},   { 87,0,0b10011},   { 88,1,0b10101},   { 89,1,0b01011},   { 90,0,0b01101},   { 91,1,0b01111},   { 92,1,0b00101},   { 93,1,0b11101},   { 94,0,0b00111},   { 95,1,0b01001},   { 96,0,0b00101},   { 97,1,0b10001},   { 98,0,0b10001},   { 99,0,0b10001},   {100,1,0b11001},   {101,1,0b01101},   {102,1,0b01001},   {103,0,0b00101},   {104,0,0b00111},   {105,0,0b00111},   {106,1,0b10011},   {107,1,0b00001},   {108,0,0b01101},   {109,1,0b10101},   {110,0,0b10101},   {111,1,0b11001},   {112,0,0b01111},   {113,1,0b01111},   {114,1,0b00101},   {115,0,0b10001},   {116,0,0b10101},   {117,0,0b10111},   {118,1,0b11011},   {119,1,0b10111},   {120,1,0b10101},   {121,0,0b01111},   {122,0,0b01111},   {123,0,0b10101},   {124,1,0b10101},   {125,0,0b00111},   {126,0,0b01011},   {127,1,0b10001},   {128,0,0b01011},   {129,1,0b11101},   {130,0,0b01001},   {131,1,0b10111},   {132,0,0b10111},   {133,1,0b11001},   {134,0,0b01111},   {135,1,0b11001},   {136,0,0b00101},   {137,1,0b11101},   {138,1,0b00011},   {139,1,0b11011},   {140,1,0b00011},   {141,0,0b10001},   {142,1,0b10111},   {143,0,0b00001},   {144,0,0b01101},   {145,0,0b10001},   {146,1,0b11011},   {147,0,0b00101},   {148,0,0b10111},   {149,1,0b11011},   {150,0,0b01111},   {151,1,0b11001},   {152,0,0b10011},   {153,1,0b10101},   {154,1,0b10111},   {155,0,0b01011},   {156,1,0b01111},   {157,0,0b00111},   {158,0,0b10001},   {159,1,0b11011},   {160,1,0b00001},   {161,0,0b11111},   {162,1,0b11111},   {163,0,0b01001},   {164,0,0b10001},   {165,1,0b10011},   {166,1,0b01111},   {167,0,0b00111},   {168,0,0b01111},   {169,1,0b10101},   {170,0,0b01101},   {171,1,0b11001},   {172,1,0b11011},   {173,1,0b01011},   {174,1,0b10101},   {175,1,0b01101},   {176,1,0b01101},   {177,0,0b00111},   {178,1,0b11001},   {179,0,0b01011},   {180,1,0b11011},   {181,0,0b00011},   {182,1,0b01001},   {183,0,0b01101},   {184,0,0b11001},   {185,1,0b11101},   {186,1,0b10011},   {187,0,0b00011},   {188,1,0b00011},   {189,0,0b00011},   {190,0,0b00101},   {191,1,0b10101},   {192,1,0b00001},   {193,1,0b10001},   {194,1,0b11011},   {195,0,0b10001},   {196,0,0b10111},   {197,0,0b11101},   {198,1,0b11101},   {199,1,0b00101},   {200,0,0b01101},   {201,1,0b11111},   {202,1,0b01101},   {203,1,0b00101},   {204,1,0b01001},   {205,1,0b10001},   {206,0,0b01001},   {207,1,0b01011},   {208,0,0b10111},   {209,1,0b10111},   {210,1,0b01101},   {211,0,0b01011},   {212,1,0b10101},   {213,0,0b01011},   {214,1,0b10001},   {215,0,0b00001},   {216,0,0b11011},   {217,0,0b11011},   {218,1,0b11011},   {219,1,0b11101},   {220,0,0b00001},   {221,1,0b00101},   {222,0,0b00011},   {223,1,0b01001},   {224,1,0b10101},   {225,1,0b11001},   {226,1,0b11101},   {227,1,0b00001},   {228,1,0b01001},   {229,0,0b00111},   {230,0,0b01001},   {231,1,0b11011},   {232,1,0b01111},   {233,0,0b01001},   {234,0,0b01011},   {235,1,0b01111},   {236,0,0b00111},   {237,0,0b00111},   {238,1,0b11011},   {239,1,0b10101},   {240,0,0b00001},   {241,0,0b10001},   {242,1,0b11011},   {243,0,0b00101},   {244,1,0b10011},   {245,0,0b00111},   {246,1,0b10001},   {247,1,0b11001},   {248,1,0b11001},   {249,1,0b00111},   {250,1,0b10111},   {251,0,0b00001},   {252,1,0b10101},   {253,1,0b10011},   {254,1,0b00101},   {255,1,0b10101},   {256,1,0b01111},   {257,0,0b01001},   {258,1,0b10001},   {259,0,0b00011},   {260,0,0b10001},   {261,1,0b11111},   {262,0,0b00011},   {263,0,0b01011},   {264,1,0b10011},   {265,1,0b00011},   {266,0,0b01101},   {267,1,0b11001},   {268,0,0b00101},   {269,0,0b10111},   {270,1,0b11011},   {271,1,0b11101},   {272,1,0b10001},   {273,0,0b00001},   {274,1,0b11101},   {275,0,0b10111},   {276,1,0b11111},   {277,0,0b01101},   {278,0,0b10111},   {279,1,0b11111},   {280,0,0b00111},   {281,1,0b11101},   {282,0,0b01011},   {283,1,0b10101},   {284,0,0b01101},   {285,1,0b10001},   {286,0,0b00101},   {287,0,0b10001},   {288,0,0b11001},   {289,1,0b11101},   {290,1,0b00111},   {291,0,0b01011},   {292,1,0b11001},   {293,0,0b01011},   {294,1,0b11001},   {295,0,0b00001},   {296,0,0b00011},   {297,0,0b00101},   {298,1,0b11011},   {299,0,0b00011},   {300,0,0b00111},   {301,1,0b10001},   {302,1,0b11011},   {303,0,0b11001},   {304,1,0b11111},   {305,1,0b11101},   {306,0,0b11001},   {307,1,0b11111},   {308,0,0b01001},   {309,1,0b01101},   {310,1,0b11011},   {311,1,0b10101},   {312,0,0b00101},   {313,0,0b01101},   {314,1,0b10111},   {315,0,0b00111},   {316,0,0b11011},   {317,1,0b11011},   {318,1,0b00011},   {319,0,0b01111},   {320,0,0b11001},   {321,1,0b11111},   {322,1,0b10011},   {323,1,0b01101},   {324,1,0b11101},   {325,1,0b00001},   {326,1,0b01101},   {327,0,0b01001},   {328,1,0b11011},   {329,0,0b00111},   {330,1,0b10001},   {331,1,0b10011},   {332,0,0b01111},   {333,0,0b10101},   {334,1,0b11101},   {335,1,0b11001},   {336,0,0b01101},   {337,1,0b10111},   {338,1,0b00001},   {339,1,0b00011},   {340,1,0b01111},   {341,1,0b11011},   {342,0,0b10101},   {343,1,0b10111},   {344,1,0b10101},   {345,1,0b11011},   {346,0,0b01011},   {347,1,0b11001},   {348,1,0b01001},   {349,1,0b01101},   {350,0,0b11001},   {351,1,0b11111},   {352,1,0b01001},   {353,1,0b11011},   {354,1,0b00101},   {355,1,0b00111},   {356,0,0b01111},   {357,1,0b11001},   {358,0,0b10011},   {359,1,0b11011},   {360,0,0b01001},   {361,1,0b10001},   {362,1,0b10101},   {363,0,0b01101},   {364,1,0b11101},   {365,1,0b01111},   {366,1,0b01101},   {367,0,0b00111},   {368,0,0b01101},   {369,1,0b11001},   {370,0,0b10011},   {371,1,0b11101},   {372,0,0b01001},   {373,0,0b01011},   {374,1,0b01111},   {375,0,0b10101},   {376,1,0b11101},   {377,0,0b00101},   {378,0,0b01011},   {379,1,0b11001},   {380,1,0b10001},   {381,0,0b00101},   {382,1,0b11101},   {383,0,0b01111},   {384,1,0b11111},   {385,0,0b10111},   {386,1,0b11111},   {387,1,0b01101},   {388,0,0b10011},   {389,1,0b10101},   {390,0,0b00001},   {391,0,0b00011},   {392,1,0b10011},   {393,0,0b01001},   {394,0,0b10011},   {395,1,0b11001},   {396,0,0b01011},   {397,1,0b01111},   {398,0,0b00111},   {399,1,0b11011},   {400,1,0b10111},   {401,0,0b01111},   {402,1,0b01111},   {403,1,0b00101},   {404,1,0b11111},   {405,1,0b11101},   {406,1,0b10101},   {407,1,0b01101},   {408,0,0b01111},   {409,0,0b10001},   {410,1,0b10001},   {411,0,0b10101},   {412,1,0b11111},   {413,0,0b01111},   {414,1,0b11101},   {415,0,0b00111},   {416,0,0b01001},   {417,1,0b10001},   {418,0,0b10011},   {419,1,0b11101},   {420,0,0b00001},   {421,1,0b01111},   {422,0,0b11001},   {423,1,0b11101},   {424,1,0b01101},   {425,1,0b11111},   {426,1,0b01101},   {427,0,0b00011},   {428,0,0b00101},   {429,0,0b11101},   {430,1,0b11111},   {431,0,0b00011},   {432,0,0b01001},   {433,1,0b11011},   {434,0,0b01101},   {435,1,0b11111},   {436,1,0b11001},   {437,0,0b01111},   {438,1,0b11101},   {439,1,0b11101},   {440,1,0b00101},   {441,0,0b01011},   {442,1,0b11011},   {443,0,0b00011},   {444,1,0b00101},   {445,1,0b11011},   {446,0,0b11111},   {447,1,0b11111},   {448,1,0b10101},   {449,0,0b00011},   {450,0,0b01111},   {451,0,0b10001},   {452,1,0b11011},   {453,0,0b01101},   {454,0,0b10101},   {455,1,0b11111},   {456,1,0b10111},   {457,1,0b01111},   {458,0,0b01101},   {459,1,0b10101},   {460,0,0b10101},   {461,1,0b11101},   {462,1,0b00111},   {463,1,0b11101},   {464,1,0b01101},   {465,0,0b00001},   {466,0,0b10001},   {467,1,0b10101},   {468,1,0b10001},   {469,0,0b00011},   {470,1,0b01101},   {471,1,0b00001},   {472,0,0b00001},   {473,1,0b01101},   {474,1,0b10011},   {475,1,0b11101},   {476,0,0b00001},   {477,1,0b11011},   {478,1,0b00001},   {479,1,0b01101},   {480,1,0b10011},   {481,1,0b00111},   {482,1,0b01111},   {483,1,0b01001},   {484,0,0b00111},   {485,1,0b11101},   {486,0,0b01011},   {487,1,0b10101},   {488,1,0b00111},   {489,0,0b00111},   {490,1,0b01001},   {491,1,0b11101},   {492,0,0b10001},   {493,1,0b11101},   {494,1,0b01001},   {495,1,0b10001},   {496,1,0b10101},   {497,0,0b00011},   {498,1,0b00111},   {500,0,0b00011},   {501,1,0b01101},   {502,1,0b10001},   {504,0,0b01111},   {505,1,0b10111},   {507,1,0b00011},   {508,1,0b10001},   {509,1,0b10101},   {513,0,0b01101},   {514,1,0b10001},   {515,0,0b00101},   {516,0,0b00111},   {517,1,0b11101},   {518,1,0b11111},   {519,0,0b00011},   {520,1,0b00011},   {521,0,0b00101},   {522,1,0b01111},   {523,0,0b00001},   {524,1,0b10111},   {525,0,0b00001},   {526,1,0b00101},   {527,1,0b01001},   {528,0,0b00101},   {529,1,0b01111},   {530,1,0b10111},   {531,1,0b00011},   {532,0,0b10001},   {533,1,0b11101},   {534,1,0b01001},   {535,0,0b00111},   {536,0,0b10111},   {537,1,0b11111},   {538,1,0b11001},   {539,0,0b00001},   {540,1,0b01101},   {542,1,0b11001},   {544,1,0b00011},   {545,0,0b00111},   {546,1,0b10111},   {548,0,0b00101},   {549,0,0b10101},   {550,1,0b11011},   {551,1,0b01101},   {553,0,0b01011},   {554,0,0b10001},   {555,1,0b11011},   {556,0,0b01111},   {557,1,0b11111},   {558,0,0b00011},   {559,0,0b01101},   {560,0,0b10011},   {561,1,0b10011},   {562,0,0b00001},   {563,1,0b11111},   {564,1,0b11001},   {565,1,0b01001},   {569,1,0b10101},   {570,0,0b10011},   {571,1,0b11001},   {572,1,0b10011},   {573,1,0b01011},   {574,1,0b01111},   {576,0,0b00011},   {577,0,0b01101},   {578,1,0b11111},   {579,0,0b00111},   {580,1,0b01001},   {581,1,0b01001},   {582,1,0b11001},   {583,1,0b10001},   {584,1,0b10011},   {586,0,0b01001},   {587,1,0b10101},   {588,0,0b10111},   {589,1,0b11111},   {590,1,0b10111},   {591,0,0b00001},   {592,0,0b00001},   {593,1,0b01011},   {594,1,0b01001},   {595,0,0b00011},   {596,1,0b10001},   {597,0,0b11101},   {598,1,0b11111},   {599,1,0b11111},   {600,1,0b11101},   {601,1,0b10111},   {602,1,0b01011},   {603,0,0b11101},   {604,1,0b11111},   {605,1,0b01011},   {606,1,0b11011},   {607,1,0b01011},   {608,0,0b10111},   {609,1,0b11011},   {610,0,0b01101},   {611,1,0b01111},   {612,1,0b01001},   {613,0,0b01111},   {614,1,0b10111},   {615,0,0b10011},   {616,0,0b10111},   {617,1,0b11001},   {618,1,0b01111},   {619,0,0b10011},   {620,1,0b10011},   {621,0,0b00101},   {622,1,0b01111},   {625,1,0b00011},   {626,0,0b00001},   {627,1,0b11011},   {629,1,0b11111},   {630,1,0b00101},   {633,0,0b01011},   {634,1,0b01101},   {635,0,0b01111},   {636,1,0b10101},   {637,1,0b10111},   {638,0,0b10101},   {639,0,0b11011},   {640,1,0b11011},   {641,1,0b00111},   {642,0,0b01111},   {643,1,0b01111},   {644,0,0b00011},   {645,0,0b00011},   {646,0,0b01111},   {647,0,0b10011},   {648,1,0b11011},   {649,1,0b01111},   {650,1,0b01111},   {651,1,0b00111},   {652,0,0b00001},   {653,1,0b00011},   {654,1,0b10011},   {655,0,0b11011},   {656,1,0b11101},   {657,0,0b00001},   {658,1,0b00011},   {659,1,0b00101},   {660,1,0b01011},   {661,1,0b00101},   {662,0,0b00011},   {663,1,0b01001},   {664,0,0b01101},   {665,1,0b11011},   {666,0,0b10001},   {667,1,0b10011},   {668,1,0b11101},   {670,1,0b11111},   {671,1,0b11001},   {673,0,0b10001},   {674,1,0b11011},   {675,0,0b00111},   {676,1,0b11001},   {677,0,0b00101},   {678,0,0b00101},   {679,1,0b01101},   {680,1,0b00011},   {681,1,0b10001},   {682,0,0b01111},   {683,1,0b01111},   {684,1,0b00001},   {686,1,0b10001},   {691,0,0b10001},   {692,1,0b11001},   {694,1,0b11001},   {696,0,0b10001},   {697,0,0b10001},   {698,1,0b10111},   {699,1,0b11011},   {700,1,0b11111},   {701,1,0b11111},   {703,1,0b00001},   {706,0,0b00111},   {707,0,0b01111},   {708,1,0b10111},   {712,0,0b00101},   {713,1,0b01101},   {714,1,0b01101},   {715,1,0b00011},   {718,0,0b10011},   {719,0,0b11011},   {720,1,0b11111},   {721,1,0b00001},   {722,0,0b00001},   {723,0,0b10011},   {724,1,0b11011},   {725,1,0b01001},   {726,0,0b00001},   {727,1,0b00011},   {728,1,0b10111},   {731,1,0b00101},   {732,1,0b11101},   {734,1,0b01111},   {735,0,0b10011},   {736,1,0b10111},   {738,0,0b00101},   {739,0,0b01101},   {740,1,0b10011},   {741,0,0b01101},   {742,1,0b01111},   {743,0,0b00101},   {744,1,0b01111},   {745,0,0b10101},   {746,1,0b10111},   {747,0,0b00111},   {748,1,0b10011},   {749,0,0b00011},   {750,1,0b10001},   {751,0,0b00011},   {752,1,0b10001},   {753,1,0b01111},   {754,1,0b11001},   {756,1,0b01101},   {760,0,0b00001},   {761,0,0b01001},   {762,0,0b11111},   {763,1,0b11111},   {764,0,0b01101},   {765,1,0b10101},   {766,1,0b01011},   {767,0,0b00111},   {768,0,0b01101},   {769,1,0b11011},   {770,1,0b00001},   {771,0,0b01011},   {772,0,0b01111},   {773,1,0b11001},   {774,0,0b00111},   {775,0,0b01011},   {776,0,0b11011},   {777,1,0b11011},   {778,0,0b01011},   {779,1,0b01011},   {780,0,0b01101},   {781,0,0b10001},   {782,1,0b11001},   {783,1,0b00101},   {784,1,0b10011},   {785,0,0b01101},   {786,1,0b11111},   {787,0,0b00111},   {788,0,0b01011},   {789,1,0b11001},   {790,1,0b11111},   {791,1,0b10111},   {792,0,0b00011},   {793,1,0b11011},   {794,0,0b11001},   {795,1,0b11011},   {796,0,0b00101},   {797,0,0b00101},   {798,1,0b11111},   {799,0,0b01011},   {800,1,0b10111},   {801,1,0b01101},   {802,1,0b01001},   {803,0,0b01001},   {804,1,0b10011},   {805,1,0b11001},   {806,0,0b10111},   {807,1,0b11011},   {808,0,0b10101},   {809,1,0b11101},   {810,1,0b11101},   {811,0,0b00111},   {812,0,0b01011},   {813,1,0b10001},   {814,0,0b00001},   {815,0,0b00011},   {816,1,0b00011},   {817,1,0b10111},   {818,1,0b00111},   {819,0,0b01001},   {820,1,0b01011},   {821,0,0b00011},   {822,1,0b10011},   {823,0,0b01001},   {824,0,0b11001},   {825,1,0b11001},   {826,1,0b10111},   {827,1,0b01011},   {828,0,0b00011},   {829,1,0b10001},   {830,0,0b00101},   {831,1,0b11001},   {832,1,0b10011},   {833,1,0b10111},   {834,0,0b00001},   {835,0,0b01001},   {836,1,0b11011},   {837,0,0b00101},   {838,0,0b01011},   {839,1,0b10001},   {840,0,0b01111},   {841,1,0b11001},   {842,1,0b00101},   {843,1,0b10011},   {844,1,0b11001},   {845,1,0b10011},   {846,1,0b10011},   {847,1,0b01111},   {848,1,0b01101},   {849,1,0b10101},   {850,0,0b00011},   {851,0,0b00111},   {852,1,0b10001},   {853,1,0b00101},   {854,0,0b00001},   {855,0,0b01101},   {856,1,0b10111},   {857,1,0b11001},   {858,0,0b11001},   {859,1,0b11011},   {860,1,0b10111},   {861,1,0b10011},   {862,1,0b00001},   {863,0,0b00111},   {864,1,0b11001},   {865,0,0b00011},   {866,1,0b10111},   {867,0,0b01101},   {868,1,0b10001},   {869,1,0b01001},   {870,0,0b10111},   {871,1,0b10111},   {872,1,0b10001},   {873,1,0b00011},   {874,1,0b01011},   {875,1,0b01101},   {876,0,0b00011},   {877,1,0b11111},   {878,1,0b11101},   {879,1,0b10111},   {880,1,0b00101},   {881,0,0b01001},   {882,1,0b11101},   {883,1,0b00111},   {884,0,0b10011},   {885,1,0b11101},   {886,0,0b00111},   {887,1,0b10101},   {888,0,0b00011},   {889,1,0b01111},   {890,0,0b01001},   {891,1,0b01111},   {892,0,0b00011},   {893,1,0b10011},   {894,0,0b01011},   {895,1,0b11001},   {896,0,0b00001},   {897,1,0b00101},   {898,1,0b01011},   {899,0,0b00011},   {900,0,0b00011},   {901,1,0b01111},   {902,0,0b01011},   {903,1,0b01101},   {904,0,0b00001},   {905,1,0b10011},   {906,0,0b00001},   {907,0,0b00001},   {908,0,0b00111},   {909,0,0b10011},   {910,0,0b11001},   {911,1,0b11001},   {912,1,0b10001},   {913,1,0b10111},   {914,0,0b00001},   {915,0,0b00011},   {916,1,0b11011},   {917,1,0b11101},   {918,0,0b00011},   {919,0,0b10001},   {920,1,0b10111},   {921,0,0b01001},   {922,0,0b10011},   {923,1,0b10111},   {924,0,0b00111},   {925,1,0b11101},   {926,1,0b00111},   {927,0,0b00001},   {928,1,0b00011},   {929,0,0b00011},   {930,1,0b11011},   {931,0,0b00001},   {932,0,0b01011},   {933,1,0b01011},   {934,0,0b00001},   {935,0,0b00111},   {936,0,0b01011},   {937,0,0b11101},   {938,1,0b11111},   {939,1,0b11101},   {940,0,0b00101},   {941,1,0b00111},   {942,1,0b00111},   {943,0,0b11001},   {944,1,0b11111},   {945,0,0b00111},   {946,1,0b11101},   {947,1,0b10101},   {948,1,0b11011},   {949,0,0b00101},   {950,1,0b11011},   {951,1,0b01001},   {952,1,0b00001},   {953,1,0b11011},   {954,1,0b11101},   {955,0,0b00001},   {956,1,0b00001},   {957,0,0b00101},   {958,0,0b00101},   {959,1,0b01111},   {960,1,0b00111},   {961,1,0b11101},   {962,0,0b11011},   {963,1,0b11111},   {964,0,0b00001},   {965,1,0b01101},   {966,0,0b00101},   {967,0,0b00111},   {968,1,0b10101},   {969,1,0b11001},   {970,0,0b00001},   {971,1,0b01011},   {972,0,0b00011},   {973,0,0b01011},   {974,1,0b10001},   {975,0,0b01111},   {976,0,0b10011},   {977,1,0b11011},   {978,0,0b01101},   {979,1,0b11101},   {980,0,0b10001},   {981,1,0b11101},   {982,0,0b01111},   {983,1,0b01111},   {984,0,0b01001},   {985,0,0b01001},   {986,0,0b01101},   {987,1,0b11111},   {988,1,0b10111},   {989,1,0b00111},   {990,0,0b00101},   {991,1,0b01101},   {992,0,0b01001},   {993,0,0b10101},   {994,1,0b11001},   {995,1,0b00101},   {996,1,0b00101},   {997,0,0b01011},   {998,0,0b01011},   {999,1,0b01011},   {1000,1,0b10111},   {1001,0,0b01001},   {1002,1,0b01111},   {1003,0,0b00001},   {1004,0,0b01001},   {1005,0,0b01001},   {1006,0,0b01011},   {1007,1,0b11011},   {1008,1,0b10101},   {1009,1,0b11111},   {1010,0,0b01001},   {1011,1,0b01101},   {1012,0,0b00001},   {1013,1,0b11101},   {1014,1,0b00001},   {1015,1,0b10111},   {1016,1,0b00001},   {1017,1,0b11011},   {1018,1,0b10011},   {1019,0,0b00001},   {1020,1,0b11101},   {1021,0,0b01001},   {1022,1,0b10101},   {1023,0,0b11001},   {1024,1,0b11101},   {1025,0,0b00001},   {1026,1,0b01011},   {1027,0,0b01111},   {1028,1,0b10101},   {1029,1,0b01001},   {1030,1,0b01101},   {1031,0,0b01111},   {1032,0,0b11011},   {1033,0,0b11101},   {1034,1,0b11111},   {1035,0,0b00001},   {1036,0,0b00011},   {1037,1,0b01111},   {1038,0,0b11001},   {1039,1,0b11111},   {1040,1,0b00111},   {1041,0,0b00001},   {1042,1,0b00011},   {1043,0,0b01011},   {1044,0,0b10011},   {1045,1,0b11101},   {1046,1,0b11011},   {1047,1,0b01001},   {1048,1,0b00101},   {1049,1,0b11001},   {1050,1,0b01011},   {1051,1,0b01111},   {1052,1,0b10101},   {1056,1,0b11011},   {1057,0,0b01011},   {1058,0,0b01101},   {1059,1,0b11011},   {1061,0,0b00111},   {1062,0,0b11011},   {1063,1,0b11101},   {1064,0,0b00101},   {1065,1,0b00101},   {1066,0,0b00101},   {1067,1,0b11001},   {1068,0,0b00011},   {1069,1,0b01101},   {1070,0,0b01101},   {1071,1,0b11011},   {1073,0,0b00101},   {1074,0,0b10001},   {1075,1,0b11001},   {1076,1,0b00011}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_iter_skip1(it);
                {
                    const uint8_t expected_boundary[] = {0b11000111, 0b10011111, 0b10110001, 0b01100101, 0b10001101, 0b10001110, 0b10111111, 0b01000111};
                    const std::vector<uint32_t> occupieds_pos = {1, 2, 5, 9, 11, 13, 14, 16, 17, 21, 23, 28, 30, 31, 32, 33, 34, 37, 39, 41, 45, 47, 50, 52, 54, 58, 65, 66, 69, 72, 75, 76, 79, 86, 90, 97, 104, 105, 108, 110, 111, 113, 123, 126, 128, 129, 130, 131, 133, 134, 137, 138, 139, 140, 141, 142, 144, 146, 147, 148, 149, 150, 151, 152, 153, 154, 158, 160, 161, 163, 165, 172, 173, 174, 176, 181, 182, 183, 184, 186, 187, 190, 192, 193, 194, 195, 196, 198, 201, 207, 213, 214, 216, 217, 218, 220, 222, 223, 225, 226, 227, 232, 234, 237, 238, 240, 243, 244, 246, 248, 249, 250, 251, 252, 253, 256, 257, 258, 260, 261, 262, 266, 268, 274, 275, 276, 277, 279, 280, 281, 283, 285, 288, 289, 290, 291, 294, 295, 297, 298, 300, 301, 303, 305, 306, 313, 316, 318, 319, 325, 326, 327, 331, 333, 335, 336, 337, 338, 339, 343, 344, 345, 346, 351, 352, 361, 365, 366, 367, 369, 370, 374, 375, 382, 384, 387, 388, 389, 391, 392, 393, 394, 399, 401, 402, 406, 410, 411, 413, 414, 415, 423, 425, 426, 430, 431, 439, 442, 445, 446, 448, 450, 451, 452, 453, 454, 456, 462, 463, 466, 467, 469, 470, 472, 473, 474, 475, 476, 477, 479, 480, 482, 483, 486, 487, 493, 494, 495, 497, 498, 501, 504, 505, 506, 508, 510, 516, 519, 521, 525, 526, 532, 536, 539, 542, 545, 547, 551, 552, 553, 557, 558, 559, 564, 567, 572, 575, 578, 579, 580, 581, 583, 584, 586, 587, 589, 590, 593, 595, 596, 597, 599, 602, 607, 608, 609, 611, 612, 613, 614, 619, 620, 621, 624, 626, 629, 632, 638, 639, 641, 642, 644, 649, 651, 652, 653, 654, 657, 658, 660, 661, 664, 666, 668, 670, 671, 673, 674, 676, 678, 683, 684, 688, 690, 691, 693, 695, 700, 701, 702, 703, 705, 706, 707, 712, 713, 714, 716, 717, 718, 719, 720, 721, 722, 723, 724, 725, 726, 728, 731, 732, 733, 738, 740, 743, 744, 749, 750, 751, 757, 760, 761, 763, 764, 766, 767, 769, 770, 773, 775, 780, 781, 783, 784, 785, 786, 787, 788, 794, 796, 799, 800, 802, 803, 805, 806, 807, 813, 814, 817, 818, 820, 823, 825, 826, 831, 834, 840, 841, 843, 846, 849, 851, 855, 858, 861, 862, 864, 867, 868, 874, 875, 876, 880, 887, 891, 894, 895, 896, 899};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,1,0b00101},   {  2,0,0b01111},   {  3,0,0b10101},   {  4,1,0b11011},   {  5,1,0b00101},   { 10,1,0b01001},   { 13,1,0b10111},   { 15,1,0b00101},   { 16,1,0b10111},   { 19,1,0b10001},   { 20,1,0b10001},   { 25,0,0b00011},   { 26,0,0b00101},   { 27,0,0b11011},   { 28,1,0b11101},   { 29,0,0b00011},   { 30,1,0b11001},   { 33,0,0b10101},   { 34,1,0b11101},   { 35,1,0b11111},   { 37,1,0b11011},   { 38,1,0b11011},   { 39,0,0b00001},   { 40,1,0b10011},   { 41,1,0b01111},   { 44,1,0b11101},   { 46,1,0b00101},   { 48,1,0b10101},   { 53,0,0b11001},   { 54,1,0b11011},   { 56,1,0b11111},   { 59,0,0b01011},   { 60,1,0b01111},   { 62,0,0b00001},   { 63,1,0b01001},   { 64,1,0b00111},   { 69,0,0b10001},   { 70,1,0b10001},   { 77,1,0b11001},   { 78,0,0b10011},   { 79,1,0b11001},   { 82,0,0b00101},   { 83,1,0b11011},   { 85,1,0b01101},   { 89,1,0b11001},   { 90,1,0b10111},   { 94,1,0b10011},   {102,1,0b10111},   {107,1,0b00111},   {115,1,0b01111},   {124,1,0b01011},   {125,1,0b10001},   {128,1,0b01111},   {131,1,0b11111},   {132,1,0b10011},   {134,1,0b01101},   {146,1,0b11101},   {150,1,0b00011},   {152,1,0b11011},   {153,1,0b11011},   {155,1,0b00001},   {156,1,0b11011},   {158,1,0b11011},   {159,1,0b01101},   {163,1,0b00001},   {164,1,0b10001},   {165,1,0b00001},   {167,1,0b01001},   {168,1,0b00111},   {169,1,0b11111},   {171,1,0b00011},   {174,0,0b01011},   {175,1,0b10011},   {176,1,0b10111},   {177,1,0b10011},   {178,1,0b01111},   {179,1,0b00001},   {180,1,0b01111},   {181,1,0b00101},   {182,1,0b10111},   {183,1,0b10011},   {188,0,0b10001},   {189,1,0b10111},   {190,0,0b10111},   {191,1,0b11001},   {192,1,0b10001},   {194,0,0b11011},   {195,1,0b11111},   {196,0,0b00011},   {197,1,0b10111},   {205,1,0b10101},   {206,1,0b10001},   {207,1,0b10011},   {210,1,0b01001},   {216,1,0b10001},   {217,1,0b11101},   {218,1,0b10011},   {219,0,0b01011},   {220,1,0b10111},   {222,1,0b00001},   {223,1,0b01101},   {226,1,0b10101},   {229,1,0b11001},   {230,1,0b00111},   {231,1,0b11001},   {232,1,0b10111},   {233,1,0b00011},   {236,1,0b01011},   {239,1,0b10001},   {247,0,0b01101},   {248,0,0b10001},   {249,1,0b11101},   {254,1,0b01011},   {255,1,0b11101},   {257,0,0b01111},   {258,1,0b10001},   {259,0,0b00001},   {260,1,0b00011},   {261,1,0b10011},   {262,1,0b01001},   {264,1,0b10111},   {266,1,0b00101},   {268,1,0b00001},   {269,0,0b01011},   {270,1,0b10111},   {271,1,0b10111},   {276,1,0b00111},   {279,1,0b11111},   {282,1,0b11111},   {284,0,0b01001},   {285,1,0b11011},   {286,1,0b01101},   {290,1,0b00101},   {291,1,0b10111},   {293,1,0b00101},   {296,1,0b01101},   {297,1,0b10101},   {298,1,0b11011},   {299,0,0b01011},   {300,0,0b01101},   {301,1,0b01101},   {302,0,0b01011},   {303,1,0b11011},   {304,0,0b00011},   {305,1,0b10101},   {306,1,0b11101},   {307,0,0b01001},   {308,1,0b01101},   {309,0,0b01111},   {310,1,0b10111},   {311,0,0b10101},   {312,0,0b11011},   {313,1,0b11101},   {314,1,0b00111},   {315,0,0b00101},   {316,1,0b11111},   {317,1,0b10111},   {319,1,0b10111},   {327,1,0b10111},   {328,1,0b11111},   {329,0,0b00011},   {330,1,0b11101},   {331,0,0b10001},   {332,0,0b10111},   {333,1,0b11001},   {334,0,0b10111},   {335,1,0b11111},   {336,1,0b11011},   {337,1,0b00011},   {338,0,0b00011},   {339,0,0b00111},   {340,1,0b10101},   {341,0,0b10011},   {342,1,0b10101},   {343,1,0b01001},   {344,1,0b10011},   {346,0,0b01001},   {347,1,0b11111},   {348,1,0b11011},   {350,1,0b00111},   {352,1,0b10001},   {354,1,0b11111},   {355,1,0b10101},   {358,1,0b00001},   {359,1,0b01011},   {361,0,0b00001},   {362,1,0b01001},   {364,1,0b11001},   {365,0,0b01001},   {366,1,0b01011},   {373,1,0b00111},   {377,0,0b00101},   {378,1,0b01101},   {379,0,0b00001},   {380,1,0b11001},   {381,1,0b10101},   {387,1,0b11101},   {389,1,0b01101},   {390,1,0b01101},   {395,1,0b00001},   {397,1,0b01001},   {399,1,0b11101},   {401,1,0b01011},   {402,1,0b11101},   {403,1,0b11111},   {404,1,0b00111},   {409,1,0b00111},   {410,1,0b11011},   {411,1,0b11111},   {412,0,0b01001},   {413,1,0b10101},   {418,1,0b10111},   {420,1,0b11011},   {430,0,0b00101},   {431,1,0b01101},   {435,0,0b10001},   {436,1,0b10111},   {437,1,0b11101},   {438,1,0b11111},   {440,1,0b01011},   {441,0,0b01001},   {442,0,0b10101},   {443,1,0b10111},   {446,1,0b01111},   {447,0,0b00101},   {448,0,0b01001},   {449,0,0b11011},   {450,1,0b11101},   {455,1,0b10011},   {458,1,0b00101},   {461,1,0b10111},   {463,1,0b11011},   {464,0,0b01111},   {465,1,0b01111},   {466,0,0b00101},   {467,0,0b11001},   {468,1,0b11111},   {469,1,0b10001},   {470,1,0b10001},   {471,1,0b01101},   {476,1,0b01111},   {478,0,0b00011},   {479,1,0b01001},   {480,1,0b01111},   {484,1,0b01101},   {489,1,0b01101},   {490,1,0b10101},   {492,1,0b00011},   {494,0,0b10111},   {495,1,0b11111},   {496,1,0b10101},   {504,1,0b10001},   {507,0,0b10101},   {508,1,0b11001},   {509,1,0b01111},   {513,1,0b10111},   {514,0,0b00001},   {515,1,0b11001},   {524,0,0b00001},   {525,0,0b10101},   {526,0,0b11011},   {527,1,0b11101},   {528,1,0b11111},   {531,1,0b10001},   {532,1,0b11001},   {534,1,0b10111},   {537,0,0b01101},   {538,1,0b10101},   {539,0,0b10101},   {540,1,0b11101},   {541,1,0b00001},   {542,1,0b01001},   {543,1,0b11111},   {544,1,0b11101},   {551,0,0b01111},   {552,0,0b10111},   {553,1,0b11111},   {554,1,0b00011},   {556,0,0b01011},   {557,0,0b10001},   {558,1,0b11101},   {559,1,0b00111},   {560,1,0b10001},   {561,1,0b10011},   {563,1,0b00011},   {564,0,0b00101},   {565,1,0b01011},   {566,1,0b10111},   {567,1,0b01001},   {568,1,0b11111},   {569,1,0b10001},   {571,1,0b00101},   {572,0,0b00101},   {573,0,0b11111},   {574,1,0b11111},   {575,1,0b11101},   {576,1,0b11011},   {580,1,0b01101},   {581,1,0b00111},   {588,1,0b01001},   {589,1,0b11001},   {590,1,0b01101},   {593,1,0b11111},   {594,0,0b01101},   {595,1,0b01111},   {598,1,0b01001},   {601,1,0b11001},   {602,1,0b00001},   {603,1,0b01011},   {606,1,0b00101},   {608,0,0b00011},   {609,1,0b11111},   {615,0,0b00101},   {616,1,0b10101},   {619,1,0b01001},   {621,1,0b10011},   {626,1,0b10011},   {627,0,0b10001},   {628,1,0b11111},   {635,1,0b00101},   {639,1,0b00101},   {643,1,0b11111},   {646,1,0b11111},   {650,1,0b10011},   {652,1,0b11001},   {657,0,0b10011},   {658,1,0b11011},   {659,1,0b10001},   {660,0,0b00001},   {661,1,0b01011},   {664,1,0b11101},   {666,1,0b10011},   {667,1,0b11111},   {673,1,0b11011},   {676,1,0b11001},   {682,0,0b10101},   {683,1,0b11111},   {686,1,0b01111},   {689,1,0b10011},   {691,1,0b10001},   {692,0,0b00111},   {693,1,0b10001},   {694,1,0b10101},   {695,1,0b01011},   {697,1,0b11111},   {699,0,0b11001},   {700,1,0b11011},   {701,1,0b10001},   {703,0,0b10101},   {704,1,0b11111},   {705,1,0b01111},   {707,1,0b11101},   {710,0,0b00101},   {711,1,0b00111},   {712,1,0b00101},   {713,0,0b00011},   {714,1,0b01111},   {715,1,0b01011},   {718,0,0b00001},   {719,1,0b01101},   {724,1,0b10101},   {725,1,0b11111},   {726,1,0b00001},   {729,0,0b10001},   {730,1,0b10101},   {731,1,0b10001},   {732,1,0b10001},   {733,1,0b11101},   {738,1,0b11011},   {740,1,0b10011},   {741,0,0b00111},   {742,1,0b01101},   {744,0,0b00001},   {745,1,0b00111},   {747,1,0b01011},   {750,1,0b00001},   {754,1,0b11001},   {761,0,0b10101},   {762,1,0b10101},   {763,1,0b00111},   {765,1,0b01111},   {766,1,0b10001},   {768,0,0b00001},   {769,1,0b00101},   {774,0,0b10111},   {775,1,0b10111},   {777,0,0b00101},   {778,0,0b01001},   {779,1,0b11111},   {780,1,0b01011},   {781,0,0b10011},   {782,0,0b10011},   {783,1,0b11111},   {784,1,0b11101},   {785,0,0b00001},   {786,1,0b11111},   {787,1,0b01101},   {788,1,0b01001},   {789,1,0b01011},   {792,0,0b01001},   {793,0,0b01011},   {794,1,0b11101},   {795,0,0b01001},   {796,1,0b10001},   {797,1,0b10011},   {799,1,0b10111},   {800,0,0b01101},   {801,1,0b11111},   {803,1,0b01001},   {804,1,0b10111},   {806,0,0b00111},   {807,1,0b10001},   {809,0,0b01001},   {810,0,0b01011},   {811,1,0b10111},   {815,1,0b10001},   {816,0,0b01001},   {817,1,0b10101},   {821,1,0b00011},   {823,1,0b11011},   {824,1,0b11101},   {827,1,0b00001},   {829,0,0b11011},   {830,1,0b11111},   {835,1,0b01101},   {836,1,0b00101},   {837,1,0b01101},   {839,1,0b10011},   {841,1,0b00011},   {842,1,0b10101},   {843,1,0b01011},   {849,1,0b00001},   {851,1,0b10001},   {852,1,0b10001},   {854,1,0b01101},   {855,1,0b00111},   {857,1,0b10111},   {858,1,0b00011},   {859,0,0b00101},   {860,0,0b10101},   {861,1,0b11011},   {862,1,0b01101},   {863,1,0b00001},   {864,1,0b00001},   {865,1,0b00001},   {866,0,0b01011},   {867,1,0b10001},   {868,0,0b00011},   {869,0,0b00111},   {870,1,0b11001},   {871,1,0b00101},   {872,0,0b00111},   {873,1,0b11111},   {874,0,0b10011},   {875,0,0b10011},   {876,1,0b11101},   {877,0,0b10101},   {878,1,0b10101},   {880,1,0b11011},   {883,1,0b01111},   {886,0,0b00101},   {887,1,0b01001},   {888,1,0b00111},   {894,0,0b00111},   {895,1,0b10101},   {896,1,0b11111},   {897,1,0b11101},   {903,1,0b10001},   {907,1,0b00001},   {908,0,0b01111},   {909,1,0b01111},   {910,1,0b10111},   {911,0,0b10001},   {912,1,0b10011},   {914,0,0b00101},   {915,1,0b11001},   {916,1,0b10101},   {917,0,0b11011},   {918,1,0b11111},   {919,1,0b11111},   {922,0,0b00101},   {923,1,0b10001},   {925,1,0b10101},   {931,1,0b11001},   {932,1,0b01101},   {934,1,0b11001},   {935,0,0b10101},   {936,1,0b11111},   {937,1,0b00001},   {938,1,0b10011},   {939,1,0b01001},   {940,1,0b01101},   {947,1,0b00101},   {950,1,0b10111},   {953,0,0b00011},   {954,1,0b01111},   {955,1,0b10011},   {957,0,0b11111},   {958,1,0b11111},   {959,1,0b10011},   {960,1,0b10001},   {962,0,0b00111},   {963,0,0b01111},   {964,1,0b10011},   {965,1,0b11111},   {970,1,0b01001},   {971,1,0b10011},   {975,0,0b00111},   {976,1,0b01111},   {977,1,0b01111},   {978,1,0b01011},   {982,1,0b11101},   {984,0,0b10001},   {985,1,0b10011},   {986,1,0b00011},   {991,0,0b11011},   {992,1,0b11101},   {995,1,0b00011},   {1002,1,0b00101},   {1003,0,0b01011},   {1004,1,0b10011},   {1006,1,0b10011},   {1009,1,0b11001},   {1013,1,0b10011},   {1015,1,0b01101},   {1020,1,0b01001},   {1024,0,0b00101},   {1025,1,0b10111},   {1027,1,0b00101},   {1028,1,0b00001},   {1031,0,0b00001},   {1032,0,0b10101},   {1033,1,0b11001},   {1034,0,0b01011},   {1035,0,0b01011},   {1036,1,0b11001},   {1037,1,0b10001},   {1043,0,0b00101},   {1044,0,0b10011},   {1045,1,0b10011},   {1046,1,0b00111},   {1047,1,0b11011},   {1050,1,0b01111},   {1058,1,0b11111},   {1063,1,0b01101},   {1067,0,0b01101},   {1068,1,0b11101},   {1069,1,0b11111},   {1070,0,0b00111},   {1071,1,0b01011},   {1073,1,0b01101}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_iter_skip1(it);
                {
                    const uint8_t expected_boundary[] = {0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111};
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, sizeof(keys[0])), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }
                wh_iter_destroy(it);
            }
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

            Diva<O> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor);

            if constexpr (O) {
                wormhole_int_iter *it = wh_int_iter_create(s.better_tree_int_);
                wh_int_iter_seek(it, nullptr, 0);
                {
                    const size_t expected_boundary_length = 8;
                    const uint8_t expected_boundary[expected_boundary_length] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_int_iter_skip1(it);
                {
                    const size_t expected_boundary_length = 6;
                    const uint8_t expected_boundary[expected_boundary_length] = {0b00000000, 0b00010000, 0b00001100, 0b00111101, 0b11110111, 0b11101011};
                    const std::vector<uint32_t> occupieds_pos = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 20, 22, 23, 25, 27, 28, 29, 32, 34, 35, 36, 37, 38, 39, 40, 41, 42, 44, 45, 46, 47, 49, 51, 52, 54, 55, 56, 57, 59, 60, 61, 63, 64, 70, 71, 72, 73, 74, 75, 78, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 92, 94, 95, 98, 99, 100, 101, 102, 103, 104, 107, 108, 111, 113, 114, 115, 116, 117, 118, 120, 121, 122, 123, 125, 126, 127, 131, 132, 133, 134, 135, 137, 138, 139, 141, 144, 145, 146, 147, 148, 149, 151, 152, 153, 154, 155, 157, 159, 160, 162, 164, 165, 167, 168, 170, 174, 175, 176, 177, 179, 180, 181, 182, 184, 185, 186, 187, 189, 190, 192, 196, 197, 199, 201, 202, 203, 204, 205, 206, 207, 208, 210, 212, 213, 214, 216, 218, 220, 221, 222, 223, 225, 226, 228, 229, 230, 231, 234, 235, 237, 238, 239, 241, 242, 243, 244, 246, 248, 249, 250, 251, 253, 254, 255, 256, 257, 259, 261, 262, 263, 264, 265, 267, 268, 270, 271, 272, 274, 276, 277, 278, 280, 281, 282, 283, 285, 287, 288, 289, 291, 292, 293, 294, 296, 297, 298, 301, 302, 303, 304, 305, 308, 309, 310, 311, 313, 314, 315, 320, 321, 322, 323, 325, 326, 327, 328, 331, 332, 334, 335, 336, 338, 339, 340, 341, 342, 344, 345, 346, 348, 349, 351, 354, 355, 356, 357, 358, 359, 360, 363, 364, 365, 366, 367, 369, 371, 372, 373, 374, 375, 376, 378, 379, 381, 382, 383, 384, 385, 386, 388, 389, 390, 391, 394, 395, 396, 397, 398, 399, 400, 402, 403, 404, 406, 408, 409, 410, 411, 414, 415, 416, 417, 419, 420, 421, 422, 424, 428, 429, 430, 432, 433, 435, 437, 438, 439, 440, 441, 443, 444, 445, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 459, 460, 461, 462, 463, 464, 466, 467, 468, 469, 470, 471, 472, 473, 474, 476, 477, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 492, 494, 495, 496, 498, 499, 500, 501, 502, 503, 504, 506, 508, 509, 510, 511, 512, 513, 514, 516, 517, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 531, 532, 535, 537, 538, 540, 541, 542, 546, 548, 549, 550, 551, 552, 553, 554, 555, 557, 558, 559, 561, 562, 564, 565, 566, 568, 570, 572, 573, 574, 575, 578, 579, 580, 584, 585, 586, 587, 588, 589, 590, 591, 594, 595, 596, 597, 598, 599, 601, 602, 603, 604, 607, 608, 612, 613, 614, 616, 617, 618, 619, 620, 621, 623, 624, 625, 628, 629, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650, 651, 653, 655, 657, 658, 659, 660, 662, 664, 665, 666, 668, 669, 670, 672, 673, 674, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 689, 690, 691, 692, 693, 694, 697, 698, 700, 701, 702, 703, 705, 706, 707, 708, 709, 710, 711, 713, 714, 715, 718, 720, 721, 722, 724, 725, 727, 728, 730, 731, 732, 733, 736, 737, 738, 744, 745, 746, 747, 748, 750, 753, 754, 755, 756, 758, 759, 761, 762, 763, 765, 767, 768, 769, 770, 771, 772, 773, 774, 775, 777, 778, 779, 780, 781, 783, 784};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,0,0b00001},   {  2,0,0b01101},   {  3,0,0b10011},   {  4,0,0b10101},   {  5,1,0b11111},   {  6,1,0b11011},   {  7,0,0b10101},   {  8,1,0b10111},   {  9,1,0b01101},   { 10,0,0b00011},   { 11,0,0b10101},   { 12,1,0b11101},   { 13,1,0b00011},   { 14,0,0b00001},   { 15,0,0b00001},   { 16,1,0b01101},   { 17,1,0b10011},   { 18,0,0b10101},   { 19,1,0b11011},   { 20,1,0b00011},   { 21,1,0b01011},   { 22,0,0b00111},   { 23,0,0b01011},   { 24,0,0b11011},   { 25,1,0b11111},   { 26,0,0b00101},   { 27,1,0b11101},   { 28,1,0b01001},   { 29,1,0b10001},   { 30,1,0b10011},   { 31,1,0b11011},   { 32,0,0b00001},   { 33,0,0b00111},   { 34,0,0b11011},   { 35,0,0b11011},   { 36,1,0b11101},   { 37,1,0b00101},   { 38,1,0b11111},   { 39,0,0b01001},   { 40,0,0b10011},   { 41,0,0b10011},   { 42,1,0b11111},   { 43,1,0b11101},   { 44,0,0b01101},   { 45,0,0b01101},   { 46,1,0b11101},   { 47,1,0b01101},   { 48,0,0b00011},   { 49,0,0b11001},   { 50,1,0b11011},   { 51,1,0b01001},   { 52,1,0b00111},   { 53,0,0b00011},   { 54,1,0b00111},   { 55,1,0b10011},   { 56,0,0b00001},   { 57,1,0b11001},   { 58,0,0b00001},   { 59,0,0b00001},   { 60,1,0b01001},   { 61,0,0b00101},   { 62,0,0b10011},   { 63,1,0b10111},   { 64,1,0b00011},   { 65,1,0b00111},   { 66,0,0b10001},   { 67,1,0b10001},   { 68,1,0b01101},   { 69,0,0b00001},   { 70,0,0b01101},   { 71,1,0b11011},   { 72,0,0b01101},   { 73,0,0b10111},   { 74,0,0b11001},   { 75,1,0b11101},   { 76,0,0b00111},   { 77,0,0b00111},   { 78,0,0b01011},   { 79,1,0b11011},   { 80,0,0b01001},   { 81,0,0b01101},   { 82,0,0b01101},   { 83,1,0b11111},   { 84,1,0b11111},   { 85,0,0b10011},   { 86,0,0b11001},   { 87,1,0b11001},   { 88,1,0b00101},   { 89,0,0b00001},   { 90,1,0b00111},   { 91,1,0b01011},   { 92,0,0b01011},   { 93,0,0b11011},   { 94,1,0b11011},   { 95,1,0b10111},   { 96,0,0b01001},   { 97,1,0b10111},   { 98,0,0b10011},   { 99,1,0b11011},   {100,1,0b11001},   {101,1,0b00101},   {102,1,0b11001},   {103,1,0b01101},   {104,0,0b10111},   {105,1,0b11111},   {106,1,0b11001},   {109,0,0b00101},   {110,1,0b01111},   {111,1,0b01001},   {112,0,0b11101},   {113,1,0b11101},   {114,0,0b01111},   {115,0,0b10001},   {116,1,0b11101},   {117,0,0b00101},   {118,1,0b11001},   {119,1,0b11011},   {120,1,0b00111},   {121,0,0b01011},   {122,0,0b01011},   {123,0,0b01101},   {124,1,0b11111},   {125,1,0b10101},   {126,0,0b10011},   {127,1,0b10101},   {128,1,0b11001},   {129,0,0b00011},   {130,1,0b10111},   {131,1,0b01111},   {134,1,0b01011},   {135,1,0b00111},   {137,1,0b10111},   {138,0,0b01001},   {139,1,0b01111},   {140,1,0b00101},   {141,0,0b01001},   {142,1,0b01011},   {143,0,0b01111},   {144,1,0b10011},   {146,1,0b01011},   {148,0,0b00001},   {149,0,0b10011},   {150,1,0b11001},   {152,1,0b01011},   {154,0,0b10001},   {155,0,0b10101},   {156,1,0b10101},   {157,1,0b10011},   {158,1,0b11001},   {159,0,0b10001},   {160,0,0b10101},   {161,1,0b11111},   {162,0,0b00001},   {163,0,0b00111},   {164,1,0b01011},   {165,1,0b01011},   {166,1,0b11101},   {167,0,0b00111},   {168,1,0b01011},   {169,0,0b00101},   {170,1,0b10101},   {171,1,0b11101},   {172,0,0b00101},   {173,1,0b11001},   {174,0,0b00111},   {175,1,0b11101},   {176,0,0b00111},   {177,1,0b11001},   {179,0,0b01001},   {180,1,0b11011},   {181,0,0b00011},   {182,1,0b11111},   {183,1,0b11111},   {184,0,0b10111},   {185,1,0b10111},   {186,0,0b00101},   {187,1,0b11101},   {188,0,0b10001},   {189,0,0b10101},   {190,1,0b11111},   {191,0,0b01111},   {192,0,0b10111},   {193,1,0b11111},   {194,1,0b10111},   {195,1,0b01011},   {197,0,0b01011},   {198,1,0b10101},   {199,1,0b10111},   {200,0,0b10101},   {201,1,0b11001},   {202,1,0b00101},   {203,1,0b11011},   {204,1,0b01011},   {207,0,0b00001},   {208,0,0b01011},   {209,0,0b01011},   {210,1,0b01111},   {211,1,0b11111},   {212,1,0b10111},   {213,1,0b01111},   {214,1,0b11011},   {215,0,0b01011},   {216,1,0b01101},   {218,1,0b01001},   {219,0,0b11011},   {220,0,0b11011},   {221,1,0b11111},   {222,1,0b10101},   {224,1,0b01011},   {226,1,0b11111},   {229,1,0b11001},   {230,1,0b01011},   {233,0,0b01011},   {234,1,0b10111},   {238,1,0b01111},   {239,0,0b11101},   {240,0,0b11101},   {241,0,0b11111},   {242,1,0b11111},   {243,0,0b00001},   {244,0,0b01001},   {245,1,0b10101},   {246,1,0b01111},   {247,0,0b00011},   {248,1,0b11001},   {249,0,0b00011},   {250,0,0b00101},   {251,1,0b11011},   {252,0,0b00101},   {253,1,0b01111},   {254,0,0b00001},   {255,0,0b11001},   {256,1,0b11111},   {257,0,0b00001},   {258,0,0b01011},   {259,1,0b11101},   {260,1,0b01011},   {261,0,0b01101},   {262,1,0b01111},   {263,0,0b11011},   {264,1,0b11101},   {265,1,0b00011},   {266,0,0b00011},   {267,1,0b11011},   {268,0,0b00101},   {269,1,0b10001},   {270,1,0b11101},   {271,1,0b11001},   {272,0,0b01101},   {273,1,0b11001},   {275,1,0b10101},   {277,0,0b11001},   {278,1,0b11001},   {279,1,0b10101},   {280,0,0b00011},   {281,1,0b00101},   {282,1,0b00101},   {283,0,0b10001},   {284,1,0b11111},   {285,1,0b10101},   {286,1,0b10111},   {287,0,0b00101},   {288,1,0b01101},   {290,1,0b01011},   {292,1,0b01111},   {293,1,0b00111},   {296,0,0b01011},   {297,1,0b01101},   {298,1,0b11001},   {301,0,0b00001},   {302,1,0b10101},   {303,0,0b00001},   {304,1,0b01011},   {305,1,0b11001},   {306,1,0b01011},   {308,0,0b01001},   {309,1,0b10001},   {310,1,0b01001},   {312,0,0b01011},   {313,1,0b01011},   {314,0,0b01011},   {315,0,0b01011},   {316,1,0b01111},   {317,0,0b00011},   {318,1,0b11011},   {319,1,0b10011},   {320,0,0b10001},   {321,1,0b11001},   {322,1,0b00001},   {325,0,0b01101},   {326,0,0b01101},   {327,1,0b11001},   {328,0,0b01101},   {329,1,0b10111},   {330,0,0b00001},   {331,1,0b10011},   {332,0,0b00011},   {333,1,0b10111},   {334,1,0b11101},   {335,0,0b01111},   {336,0,0b10111},   {337,1,0b11001},   {338,1,0b10011},   {339,1,0b01101},   {340,1,0b01011},   {341,1,0b11111},   {342,0,0b01001},   {343,1,0b11001},   {344,1,0b00001},   {346,0,0b10001},   {347,0,0b10111},   {348,1,0b11101},   {349,0,0b00111},   {350,1,0b10001},   {351,1,0b10001},   {352,0,0b01011},   {353,1,0b11001},   {354,1,0b00011},   {355,1,0b00111},   {357,0,0b10011},   {358,1,0b11101},   {359,1,0b00011},   {360,0,0b01111},   {361,1,0b10001},   {362,1,0b01111},   {363,1,0b10101},   {366,1,0b11111},   {367,1,0b00111},   {370,1,0b11001},   {371,1,0b11101},   {373,1,0b00011},   {375,1,0b00101},   {378,1,0b00111},   {379,1,0b11111},   {381,0,0b01001},   {382,0,0b10011},   {383,0,0b11001},   {384,1,0b11011},   {385,0,0b00111},   {386,1,0b01101},   {387,0,0b00011},   {388,0,0b00101},   {389,0,0b01101},   {390,0,0b10111},   {391,0,0b11001},   {392,1,0b11111},   {393,1,0b10111},   {394,1,0b00001},   {395,0,0b01101},   {396,1,0b01101},   {397,0,0b00001},   {398,0,0b01011},   {399,1,0b10111},   {400,1,0b11111},   {401,0,0b00101},   {402,1,0b01001},   {403,1,0b01101},   {404,0,0b10001},   {405,0,0b10011},   {406,1,0b10101},   {407,1,0b01011},   {408,1,0b10001},   {409,0,0b01011},   {410,1,0b01111},   {411,1,0b11011},   {412,0,0b00101},   {413,1,0b10001},   {414,0,0b00111},   {415,0,0b11001},   {416,0,0b11001},   {417,1,0b11101},   {418,1,0b01001},   {419,0,0b00111},   {420,1,0b11011},   {421,1,0b11101},   {422,1,0b01111},   {423,1,0b11001},   {424,0,0b00011},   {425,1,0b10111},   {426,0,0b00101},   {427,1,0b11001},   {428,0,0b10011},   {429,1,0b10111},   {430,1,0b10011},   {431,1,0b01011},   {432,0,0b01111},   {433,1,0b11111},   {438,1,0b10111},   {440,1,0b00011},   {441,1,0b10011},   {442,1,0b01111},   {445,1,0b11111},   {446,1,0b01001},   {447,0,0b00001},   {448,0,0b00001},   {449,0,0b01101},   {450,0,0b01111},   {451,0,0b10101},   {452,0,0b11101},   {453,1,0b11111},   {454,0,0b10001},   {455,1,0b11111},   {456,0,0b10001},   {457,1,0b11101},   {458,0,0b00101},   {459,1,0b01001},   {460,0,0b00101},   {461,0,0b10011},   {462,0,0b11001},   {463,0,0b11101},   {464,1,0b11101},   {465,1,0b01111},   {466,0,0b00011},   {467,1,0b10111},   {468,1,0b00001},   {469,1,0b01111},   {470,1,0b00111},   {471,1,0b10011},   {472,0,0b11001},   {473,0,0b11001},   {474,1,0b11101},   {475,1,0b10111},   {476,1,0b11111},   {477,0,0b00011},   {478,1,0b01001},   {479,0,0b00001},   {480,0,0b10101},   {481,1,0b11101},   {482,0,0b10001},   {483,1,0b10011},   {484,1,0b00001},   {485,1,0b00001},   {486,0,0b01111},   {487,1,0b01111},   {488,1,0b00111},   {489,0,0b00011},   {490,0,0b00101},   {491,0,0b01101},   {492,1,0b10001},   {493,0,0b00011},   {494,1,0b10111},   {495,0,0b01001},   {496,1,0b10101},   {497,0,0b00111},   {498,1,0b01011},   {499,0,0b00001},   {500,1,0b10001},   {501,0,0b01011},   {502,0,0b10011},   {503,1,0b11111},   {504,1,0b11011},   {505,1,0b10011},   {506,1,0b10001},   {507,0,0b00111},   {508,1,0b10001},   {509,1,0b01101},   {510,0,0b10111},   {511,1,0b11101},   {512,1,0b11101},   {513,0,0b00111},   {514,0,0b11001},   {515,1,0b11101},   {516,1,0b01011},   {517,0,0b10001},   {518,1,0b10111},   {519,0,0b00011},   {520,1,0b00111},   {521,1,0b00101},   {522,0,0b10101},   {523,1,0b11001},   {524,0,0b00101},   {525,1,0b10101},   {526,1,0b10001},   {527,0,0b01001},   {528,1,0b11111},   {529,0,0b01101},   {530,1,0b01111},   {531,1,0b11001},   {532,0,0b00101},   {533,1,0b00111},   {534,1,0b01011},   {535,0,0b00111},   {536,0,0b10001},   {537,0,0b10101},   {538,1,0b10111},   {539,0,0b00011},   {540,0,0b01001},   {541,1,0b10011},   {542,1,0b11101},   {543,0,0b01001},   {544,0,0b01001},   {545,0,0b01011},   {546,1,0b11011},   {547,0,0b01101},   {548,0,0b10101},   {549,1,0b10101},   {550,0,0b00001},   {551,0,0b00101},   {552,1,0b01111},   {553,1,0b01011},   {554,0,0b10101},   {555,1,0b11101},   {556,1,0b10011},   {557,0,0b01001},   {558,0,0b10001},   {559,1,0b10101},   {560,0,0b00101},   {561,0,0b01111},   {562,0,0b10101},   {563,0,0b10111},   {564,1,0b11101},   {565,1,0b01111},   {566,1,0b00101},   {567,0,0b00101},   {568,0,0b00111},   {569,1,0b01001},   {570,1,0b01011},   {571,1,0b01111},   {572,1,0b00101},   {573,1,0b00111},   {574,1,0b11011},   {575,0,0b00111},   {576,1,0b10101},   {577,1,0b01111},   {578,1,0b11001},   {579,1,0b00001},   {580,1,0b00001},   {581,1,0b00001},   {582,0,0b01011},   {583,1,0b11011},   {584,0,0b00011},   {585,0,0b01001},   {586,0,0b10011},   {587,1,0b11011},   {588,0,0b10001},   {589,1,0b11001},   {590,1,0b00101},   {591,1,0b10111},   {592,0,0b10011},   {593,0,0b10101},   {594,1,0b11001},   {595,0,0b10011},   {596,1,0b10101},   {597,1,0b10011},   {598,1,0b11001},   {599,0,0b00111},   {600,0,0b10101},   {601,0,0b10101},   {602,1,0b11101},   {603,0,0b10011},   {604,1,0b10011},   {605,1,0b01001},   {606,1,0b01011},   {607,1,0b00111},   {608,0,0b00001},   {609,0,0b00011},   {610,0,0b10001},   {611,1,0b11001},   {612,0,0b00001},   {613,0,0b01011},   {614,1,0b11001},   {615,1,0b00111},   {616,1,0b11001},   {617,0,0b00111},   {618,0,0b01101},   {619,0,0b10001},   {620,1,0b10001},   {621,1,0b11011},   {622,0,0b01101},   {623,1,0b11101},   {624,0,0b00011},   {625,1,0b01111},   {626,1,0b11101},   {627,0,0b00111},   {628,1,0b01001},   {629,0,0b00001},   {630,1,0b11011},   {631,0,0b00101},   {632,1,0b10111},   {633,0,0b00111},   {634,1,0b11011},   {635,1,0b11101},   {636,1,0b00011},   {637,1,0b01111},   {638,0,0b00011},   {639,0,0b01101},   {640,1,0b11011},   {641,1,0b00011},   {642,0,0b00111},   {643,0,0b01001},   {644,0,0b01011},   {645,0,0b10111},   {646,0,0b11001},   {647,1,0b11101},   {648,1,0b01011},   {649,1,0b10111},   {650,1,0b00011},   {651,0,0b00101},   {652,0,0b10111},   {653,0,0b10111},   {654,1,0b11011},   {655,0,0b01011},   {656,1,0b11011},   {657,0,0b00111},   {658,0,0b01101},   {659,0,0b10011},   {660,1,0b11111},   {661,1,0b01011},   {662,0,0b10101},   {663,1,0b11011},   {664,0,0b01011},   {665,0,0b01111},   {666,1,0b11001},   {667,1,0b11011},   {668,1,0b00111},   {669,1,0b00101},   {670,1,0b01001},   {671,1,0b01011},   {672,1,0b11101},   {673,1,0b10011},   {674,0,0b01001},   {675,1,0b11001},   {676,0,0b00101},   {677,1,0b01111},   {678,0,0b00001},   {679,1,0b10001},   {680,0,0b00101},   {681,1,0b11011},   {682,1,0b10101},   {683,0,0b01111},   {684,0,0b10111},   {685,1,0b11111},   {686,0,0b00101},   {687,0,0b01011},   {688,0,0b10011},   {689,1,0b11011},   {690,1,0b01001},   {691,0,0b00101},   {692,1,0b11001},   {693,0,0b10011},   {694,1,0b10101},   {695,1,0b10001},   {696,1,0b10111},   {697,1,0b00111},   {698,1,0b11001},   {699,1,0b00001},   {700,1,0b01101},   {701,1,0b11111},   {702,1,0b00001},   {703,0,0b00111},   {704,1,0b11001},   {705,1,0b10001},   {706,0,0b11101},   {707,1,0b11101},   {708,0,0b10111},   {709,1,0b11111},   {710,0,0b00001},   {711,0,0b00101},   {712,1,0b01011},   {713,0,0b00011},   {714,1,0b10111},   {715,0,0b00011},   {716,1,0b00101},   {717,0,0b01001},   {718,0,0b10101},   {719,1,0b11111},   {720,0,0b01011},   {721,1,0b10111},   {722,1,0b01111},   {723,0,0b01011},   {724,1,0b10101},   {725,0,0b01111},   {726,1,0b11011},   {727,0,0b00111},   {728,0,0b10011},   {729,1,0b10111},   {730,0,0b00111},   {731,0,0b10001},   {732,1,0b10011},   {733,1,0b00001},   {734,0,0b01111},   {735,0,0b01111},   {736,1,0b10001},   {737,1,0b01001},   {738,0,0b00001},   {739,1,0b00111},   {740,1,0b11011},   {741,0,0b10001},   {742,0,0b10001},   {743,0,0b10001},   {744,1,0b10111},   {745,1,0b10001},   {746,1,0b10001},   {747,0,0b01001},   {748,0,0b01101},   {749,0,0b11001},   {750,1,0b11011},   {751,1,0b10001},   {752,0,0b00001},   {753,0,0b00011},   {754,1,0b00101},   {755,1,0b00111},   {756,0,0b00011},   {757,1,0b01001},   {758,1,0b00111},   {759,0,0b00001},   {760,1,0b11101},   {761,1,0b01111},   {762,1,0b00111},   {763,0,0b01001},   {764,1,0b10001},   {765,1,0b00111},   {766,1,0b01011},   {767,1,0b10011},   {768,0,0b01101},   {769,1,0b10001},   {770,1,0b10001},   {771,0,0b00011},   {772,0,0b00101},   {773,1,0b11101},   {774,1,0b10111},   {775,1,0b10101},   {776,1,0b10001},   {777,1,0b11101},   {778,0,0b11001},   {779,1,0b11101},   {780,0,0b01011},   {781,1,0b11001},   {782,0,0b10001},   {783,1,0b10101},   {784,1,0b00001},   {785,0,0b10001},   {786,1,0b11101},   {787,1,0b11111},   {788,0,0b00011},   {789,1,0b10001},   {790,1,0b01011},   {791,1,0b10101},   {792,0,0b11001},   {793,1,0b11101},   {794,0,0b10001},   {795,1,0b10101},   {796,1,0b01001},   {797,1,0b11011},   {798,0,0b00111},   {799,0,0b01001},   {800,0,0b10111},   {801,1,0b11111},   {802,1,0b11101},   {803,1,0b10101},   {804,0,0b00011},   {805,1,0b10001},   {806,1,0b11111},   {807,1,0b10111},   {808,1,0b01111},   {809,0,0b01011},   {810,1,0b11011},   {811,1,0b10101},   {812,0,0b00011},   {813,1,0b01101},   {814,0,0b00101},   {815,0,0b01001},   {816,1,0b10001},   {817,0,0b01101},   {818,1,0b11111},   {819,1,0b01001},   {820,0,0b10101},   {821,0,0b10111},   {822,1,0b11111},   {823,0,0b00001},   {824,1,0b10011},   {825,1,0b00011},   {826,1,0b01101},   {827,1,0b11011},   {828,1,0b00001},   {829,1,0b11011},   {830,1,0b10011},   {831,0,0b01101},   {832,1,0b10001},   {833,0,0b00011},   {834,1,0b10011},   {835,0,0b00101},   {836,1,0b01001},   {837,0,0b01111},   {838,1,0b10011},   {839,0,0b10011},   {840,1,0b11111},   {841,1,0b01001},   {842,1,0b01101},   {843,0,0b01111},   {844,1,0b10101},   {845,0,0b00011},   {846,1,0b11111},   {847,1,0b00111},   {848,0,0b00001},   {849,0,0b00111},   {850,0,0b10101},   {851,0,0b10111},   {852,1,0b11101},   {853,0,0b01111},   {854,1,0b11001},   {855,1,0b10001},   {856,1,0b01111},   {857,0,0b10111},   {858,0,0b11001},   {859,1,0b11001},   {860,0,0b10101},   {861,1,0b11111},   {862,1,0b10011},   {863,0,0b01011},   {864,1,0b11011},   {865,1,0b11011},   {866,0,0b10011},   {867,1,0b10101},   {868,0,0b00101},   {869,1,0b00101},   {870,0,0b00011},   {871,0,0b10111},   {872,1,0b11011},   {873,0,0b01001},   {874,0,0b01001},   {875,1,0b11101},   {876,0,0b01001},   {877,0,0b10101},   {878,1,0b11101},   {879,0,0b11001},   {880,0,0b11111},   {881,1,0b11111},   {882,0,0b01011},   {883,1,0b11101},   {884,0,0b00111},   {885,1,0b10111},   {886,0,0b00101},   {887,1,0b01011},   {888,0,0b00001},   {889,1,0b00011},   {890,0,0b00111},   {891,0,0b01001},   {892,1,0b01101},   {893,0,0b00011},   {894,1,0b10011},   {895,1,0b01111},   {896,1,0b10111},   {897,0,0b01001},   {898,0,0b01111},   {899,0,0b10011},   {900,1,0b11111},   {901,1,0b11001},   {902,0,0b00011},   {903,1,0b10001},   {904,1,0b01011},   {905,0,0b00101},   {906,1,0b01001},   {907,0,0b10011},   {908,0,0b10111},   {909,0,0b11011},   {910,1,0b11111},   {911,1,0b11111},   {912,1,0b00111},   {913,1,0b01101},   {914,1,0b11101},   {915,0,0b10101},   {916,0,0b11011},   {917,1,0b11011},   {918,0,0b01111},   {919,0,0b10001},   {920,1,0b11101},   {921,0,0b01011},   {922,1,0b10111},   {923,0,0b00001},   {924,0,0b01011},   {925,1,0b10001},   {926,0,0b00011},   {927,1,0b01101},   {928,1,0b11001},   {929,0,0b00111},   {930,0,0b01101},   {931,0,0b11011},   {932,1,0b11111},   {933,1,0b11011},   {934,1,0b11011},   {935,0,0b10101},   {936,1,0b11001},   {937,1,0b00111},   {938,1,0b01011},   {939,0,0b00101},   {940,1,0b00111},   {941,1,0b01001},   {942,1,0b11111},   {943,0,0b01101},   {944,1,0b11101},   {945,0,0b00001},   {946,1,0b11001},   {947,0,0b01001},   {948,1,0b01111},   {949,1,0b00001},   {950,0,0b00101},   {951,0,0b01111},   {952,0,0b11001},   {953,1,0b11001},   {954,0,0b10001},   {955,1,0b11011},   {956,0,0b01101},   {957,1,0b01101},   {958,1,0b01101},   {959,0,0b00111},   {960,0,0b01011},   {961,1,0b10011},   {962,0,0b00111},   {963,0,0b10001},   {964,0,0b10101},   {965,1,0b11001},   {966,1,0b11011},   {967,1,0b11101},   {968,1,0b01001},   {969,0,0b00101},   {970,0,0b00111},   {971,1,0b10111},   {972,1,0b00001},   {973,0,0b01101},   {974,1,0b10101},   {975,1,0b11111},   {976,1,0b01111},   {977,0,0b00101},   {978,0,0b10001},   {979,1,0b11111},   {980,1,0b10111},   {981,0,0b01011},   {982,0,0b01101},   {983,1,0b10101},   {984,1,0b11101},   {985,1,0b10011},   {986,0,0b10101},   {987,1,0b11111},   {988,0,0b10011},   {989,1,0b11111},   {990,0,0b00111},   {991,1,0b10111},   {992,0,0b10101},   {993,0,0b10101},   {994,1,0b11011},   {995,0,0b01001},   {996,1,0b10101},   {997,1,0b01011},   {998,1,0b10101},   {999,1,0b11001},   {1000,1,0b01111},   {1001,1,0b00011},   {1002,0,0b00001},   {1003,0,0b00011},   {1004,0,0b00101},   {1005,1,0b01011},   {1006,1,0b00101},   {1007,1,0b00101},   {1008,0,0b00111},   {1009,1,0b01011},   {1010,0,0b11001},   {1011,1,0b11111},   {1012,0,0b10101},   {1013,0,0b11101},   {1014,1,0b11111},   {1015,1,0b10101},   {1016,0,0b01011},   {1017,0,0b10111},   {1018,1,0b11111},   {1019,1,0b11011},   {1020,1,0b10101},   {1021,0,0b01001},   {1022,0,0b10001},   {1023,1,0b11111},   {1024,1,0b11001},   {1025,0,0b00101},   {1026,0,0b10001},   {1027,1,0b11001},   {1028,0,0b10111},   {1029,1,0b10111},   {1030,0,0b11011},   {1031,1,0b11111},   {1032,1,0b11111},   {1033,0,0b01011},   {1034,0,0b01011},   {1035,1,0b10011},   {1036,0,0b01101},   {1037,1,0b10011},   {1038,0,0b00001},   {1039,1,0b01111},   {1040,1,0b00011},   {1041,0,0b10101},   {1042,1,0b11101},   {1043,0,0b11001},   {1044,1,0b11011},   {1045,1,0b01101},   {1046,0,0b00101},   {1047,0,0b01011},   {1048,0,0b10101},   {1049,0,0b11101},   {1050,1,0b11101},   {1051,0,0b00001},   {1052,0,0b01101},   {1053,1,0b10001},   {1054,0,0b00101},   {1055,0,0b00111},   {1056,1,0b01001},   {1057,0,0b11101},   {1058,1,0b11101},   {1059,1,0b10001},   {1060,1,0b10101},   {1061,0,0b00101},   {1062,1,0b10101},   {1063,0,0b00011},   {1064,0,0b00101},   {1065,1,0b10111},   {1066,0,0b01011},   {1067,1,0b01011},   {1068,0,0b00101},   {1069,1,0b01011},   {1070,1,0b01001},   {1071,0,0b00011},   {1072,1,0b00101},   {1073,0,0b10001},   {1074,1,0b11101},   {1075,1,0b01011},   {1076,1,0b11001}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_int_iter_skip1(it);
                {
                    const size_t expected_boundary_length = 6;
                    const uint8_t expected_boundary[expected_boundary_length] = {0b01100010, 0b00101001, 0b11111000, 0b10110111, 0b10010000, 0b00101011};
                    const std::vector<uint32_t> occupieds_pos = {0, 2, 4, 6, 7, 8, 9, 10, 12, 13, 14, 15, 17, 18, 20, 21, 23, 26, 27, 28, 29, 30, 31, 32, 33, 34, 36, 37, 39, 40, 41, 42, 43, 44, 47, 49, 50, 52, 53, 54, 55, 56, 57, 58, 60, 63, 64, 65, 66, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 95, 96, 97, 98, 100, 101, 102, 104, 105, 107, 108, 109, 110, 111, 112, 113, 115, 116, 118, 119, 120, 121, 122, 123, 125, 126, 127, 128, 131, 134, 135, 136, 138, 139, 140, 141, 142, 143, 145, 146, 147, 148, 150, 152, 154, 155, 156, 157, 158, 159, 160, 163, 164, 165, 166, 170, 171, 172, 173, 175, 176, 177, 179, 180, 181, 185, 186, 187, 188, 189, 190, 193, 195, 196, 198, 199, 200, 201, 202, 203, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 219, 220, 222, 223, 224, 225, 226, 227, 229, 230, 231, 232, 233, 234, 235, 236, 237, 239, 240, 241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 254, 255, 256, 257, 258, 260, 261, 262, 264, 265, 266, 268, 269, 271, 273, 274, 275, 276, 277, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 291, 292, 297, 298, 300, 301, 302, 303, 304, 305, 308, 309, 310, 311, 312, 313, 314, 316, 317, 318, 321, 322, 323, 324, 326, 327, 328, 329, 331, 332, 335, 338, 340, 341, 343, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 358, 361, 362, 364, 365, 367, 368, 369, 370, 371, 372, 373, 374, 377, 378, 380, 382, 383, 384, 387, 388, 389, 391, 392, 393, 394, 395, 396, 398, 399, 400, 401, 403, 404, 406, 409, 410, 411, 413, 415, 417, 418, 420, 421, 424, 425, 429, 430, 431, 432, 433, 434, 435, 437, 438, 439, 440, 442, 443, 444, 445, 446, 447, 448, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 461, 462, 463, 464, 466, 468, 471, 472, 474, 475, 477, 478, 479, 480, 481, 482, 484, 485, 486, 487, 489, 490, 491, 492, 493, 494, 497, 498, 499, 502, 503, 505, 506, 507, 508, 509, 511, 512, 513, 516, 517, 521, 523, 525, 526, 527, 528, 530, 532, 537, 538, 539, 541, 542, 543, 544, 545, 549, 551, 552, 553, 554, 556, 557, 558, 559, 560, 562, 564, 567, 568, 570, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 589, 590, 591, 592, 593, 594, 595, 596, 597, 598, 601, 602, 603, 604, 606, 607, 608, 609, 610, 612, 613, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 629, 630, 631, 632, 634, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 647, 649, 650, 651, 654, 655, 656, 657, 658, 659, 661, 662, 664, 668, 669, 670, 671, 673, 675, 676, 677, 678, 680, 681, 682, 683, 684, 686, 687, 688, 689, 691, 692, 693, 694, 698, 700, 702, 703, 704, 705, 706, 708, 709, 710, 714, 715, 717, 718, 719, 720, 721, 722, 724, 725, 730, 731, 732, 733, 734, 735, 736, 737, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753, 756, 757, 758, 762, 763, 765, 767, 768, 770, 771, 772, 774, 776, 777, 778, 779, 781, 783, 784, 786, 787, 789, 790, 792, 793, 796, 797, 800, 801, 802, 803, 805, 809, 811};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b11101},   {  1,1,0b11111},   {  2,0,0b10101},   {  3,1,0b11011},   {  5,0,0b01001},   {  6,0,0b01011},   {  7,1,0b10011},   {  8,0,0b00101},   {  9,1,0b10001},   { 10,0,0b01101},   { 11,1,0b11001},   { 12,0,0b00001},   { 13,1,0b11111},   { 14,1,0b10001},   { 15,1,0b00001},   { 16,0,0b01001},   { 17,1,0b11111},   { 18,0,0b01101},   { 19,1,0b01111},   { 20,0,0b01111},   { 21,0,0b10111},   { 22,1,0b11111},   { 23,1,0b01111},   { 24,0,0b00001},   { 25,0,0b01001},   { 26,1,0b11001},   { 27,1,0b10001},   { 28,1,0b10111},   { 29,1,0b01011},   { 30,0,0b00011},   { 31,1,0b11001},   { 34,0,0b01001},   { 35,0,0b01111},   { 36,0,0b10101},   { 37,1,0b11001},   { 38,0,0b00011},   { 39,0,0b00101},   { 40,0,0b01101},   { 41,0,0b10011},   { 42,1,0b11011},   { 43,1,0b11001},   { 44,1,0b00011},   { 45,1,0b01111},   { 46,1,0b11011},   { 47,1,0b11111},   { 48,1,0b10011},   { 49,1,0b11111},   { 50,1,0b00001},   { 51,1,0b10001},   { 52,0,0b01101},   { 53,0,0b01111},   { 54,1,0b10101},   { 55,1,0b10101},   { 56,0,0b10111},   { 57,1,0b11101},   { 58,1,0b10101},   { 59,0,0b00001},   { 60,1,0b10001},   { 61,1,0b01111},   { 62,0,0b01101},   { 63,0,0b10111},   { 64,1,0b11001},   { 65,0,0b01001},   { 66,0,0b01011},   { 67,1,0b01111},   { 68,1,0b00111},   { 69,1,0b00111},   { 70,0,0b00111},   { 71,1,0b01101},   { 72,1,0b11101},   { 73,1,0b10101},   { 74,0,0b10001},   { 75,1,0b11111},   { 76,1,0b11111},   { 77,0,0b01001},   { 78,0,0b10111},   { 79,1,0b11001},   { 80,0,0b00111},   { 81,1,0b11111},   { 83,0,0b01011},   { 84,1,0b10101},   { 85,0,0b00001},   { 86,0,0b01101},   { 87,0,0b10011},   { 88,1,0b10101},   { 89,1,0b01011},   { 90,0,0b01101},   { 91,1,0b01111},   { 92,1,0b00101},   { 93,1,0b11101},   { 94,0,0b00111},   { 95,1,0b01001},   { 96,0,0b00101},   { 97,1,0b10001},   { 98,0,0b10001},   { 99,0,0b10001},   {100,1,0b11001},   {101,1,0b01101},   {102,1,0b01001},   {103,0,0b00101},   {104,0,0b00111},   {105,0,0b00111},   {106,1,0b10011},   {107,1,0b00001},   {108,0,0b01101},   {109,1,0b10101},   {110,0,0b10101},   {111,1,0b11001},   {112,0,0b01111},   {113,1,0b01111},   {114,1,0b00101},   {115,0,0b10001},   {116,0,0b10101},   {117,0,0b10111},   {118,1,0b11011},   {119,1,0b10111},   {120,1,0b10101},   {121,0,0b01111},   {122,0,0b01111},   {123,0,0b10101},   {124,1,0b10101},   {125,0,0b00111},   {126,0,0b01011},   {127,1,0b10001},   {128,0,0b01011},   {129,1,0b11101},   {130,0,0b01001},   {131,1,0b10111},   {132,0,0b10111},   {133,1,0b11001},   {134,0,0b01111},   {135,1,0b11001},   {136,0,0b00101},   {137,1,0b11101},   {138,1,0b00011},   {139,1,0b11011},   {140,1,0b00011},   {141,0,0b10001},   {142,1,0b10111},   {143,0,0b00001},   {144,0,0b01101},   {145,0,0b10001},   {146,1,0b11011},   {147,0,0b00101},   {148,0,0b10111},   {149,1,0b11011},   {150,0,0b01111},   {151,1,0b11001},   {152,0,0b10011},   {153,1,0b10101},   {154,1,0b10111},   {155,0,0b01011},   {156,1,0b01111},   {157,0,0b00111},   {158,0,0b10001},   {159,1,0b11011},   {160,1,0b00001},   {161,0,0b11111},   {162,1,0b11111},   {163,0,0b01001},   {164,0,0b10001},   {165,1,0b10011},   {166,1,0b01111},   {167,0,0b00111},   {168,0,0b01111},   {169,1,0b10101},   {170,0,0b01101},   {171,1,0b11001},   {172,1,0b11011},   {173,1,0b01011},   {174,1,0b10101},   {175,1,0b01101},   {176,1,0b01101},   {177,0,0b00111},   {178,1,0b11001},   {179,0,0b01011},   {180,1,0b11011},   {181,0,0b00011},   {182,1,0b01001},   {183,0,0b01101},   {184,0,0b11001},   {185,1,0b11101},   {186,1,0b10011},   {187,0,0b00011},   {188,1,0b00011},   {189,0,0b00011},   {190,0,0b00101},   {191,1,0b10101},   {192,1,0b00001},   {193,1,0b10001},   {194,1,0b11011},   {195,0,0b10001},   {196,0,0b10111},   {197,0,0b11101},   {198,1,0b11101},   {199,1,0b00101},   {200,0,0b01101},   {201,1,0b11111},   {202,1,0b01101},   {203,1,0b00101},   {204,1,0b01001},   {205,1,0b10001},   {206,0,0b01001},   {207,1,0b01011},   {208,0,0b10111},   {209,1,0b10111},   {210,1,0b01101},   {211,0,0b01011},   {212,1,0b10101},   {213,0,0b01011},   {214,1,0b10001},   {215,0,0b00001},   {216,0,0b11011},   {217,0,0b11011},   {218,1,0b11011},   {219,1,0b11101},   {220,0,0b00001},   {221,1,0b00101},   {222,0,0b00011},   {223,1,0b01001},   {224,1,0b10101},   {225,1,0b11001},   {226,1,0b11101},   {227,1,0b00001},   {228,1,0b01001},   {229,0,0b00111},   {230,0,0b01001},   {231,1,0b11011},   {232,1,0b01111},   {233,0,0b01001},   {234,0,0b01011},   {235,1,0b01111},   {236,0,0b00111},   {237,0,0b00111},   {238,1,0b11011},   {239,1,0b10101},   {240,0,0b00001},   {241,0,0b10001},   {242,1,0b11011},   {243,0,0b00101},   {244,1,0b10011},   {245,0,0b00111},   {246,1,0b10001},   {247,1,0b11001},   {248,1,0b11001},   {249,1,0b00111},   {250,1,0b10111},   {251,0,0b00001},   {252,1,0b10101},   {253,1,0b10011},   {254,1,0b00101},   {255,1,0b10101},   {256,1,0b01111},   {257,0,0b01001},   {258,1,0b10001},   {259,0,0b00011},   {260,0,0b10001},   {261,1,0b11111},   {262,0,0b00011},   {263,0,0b01011},   {264,1,0b10011},   {265,1,0b00011},   {266,0,0b01101},   {267,1,0b11001},   {268,0,0b00101},   {269,0,0b10111},   {270,1,0b11011},   {271,1,0b11101},   {272,1,0b10001},   {273,0,0b00001},   {274,1,0b11101},   {275,0,0b10111},   {276,1,0b11111},   {277,0,0b01101},   {278,0,0b10111},   {279,1,0b11111},   {280,0,0b00111},   {281,1,0b11101},   {282,0,0b01011},   {283,1,0b10101},   {284,0,0b01101},   {285,1,0b10001},   {286,0,0b00101},   {287,0,0b10001},   {288,0,0b11001},   {289,1,0b11101},   {290,1,0b00111},   {291,0,0b01011},   {292,1,0b11001},   {293,0,0b01011},   {294,1,0b11001},   {295,0,0b00001},   {296,0,0b00011},   {297,0,0b00101},   {298,1,0b11011},   {299,0,0b00011},   {300,0,0b00111},   {301,1,0b10001},   {302,1,0b11011},   {303,0,0b11001},   {304,1,0b11111},   {305,1,0b11101},   {306,0,0b11001},   {307,1,0b11111},   {308,0,0b01001},   {309,1,0b01101},   {310,1,0b11011},   {311,1,0b10101},   {312,0,0b00101},   {313,0,0b01101},   {314,1,0b10111},   {315,0,0b00111},   {316,0,0b11011},   {317,1,0b11011},   {318,1,0b00011},   {319,0,0b01111},   {320,0,0b11001},   {321,1,0b11111},   {322,1,0b10011},   {323,1,0b01101},   {324,1,0b11101},   {325,1,0b00001},   {326,1,0b01101},   {327,0,0b01001},   {328,1,0b11011},   {329,0,0b00111},   {330,1,0b10001},   {331,1,0b10011},   {332,0,0b01111},   {333,0,0b10101},   {334,1,0b11101},   {335,1,0b11001},   {336,0,0b01101},   {337,1,0b10111},   {338,1,0b00001},   {339,1,0b00011},   {340,1,0b01111},   {341,1,0b11011},   {342,0,0b10101},   {343,1,0b10111},   {344,1,0b10101},   {345,1,0b11011},   {346,0,0b01011},   {347,1,0b11001},   {348,1,0b01001},   {349,1,0b01101},   {350,0,0b11001},   {351,1,0b11111},   {352,1,0b01001},   {353,1,0b11011},   {354,1,0b00101},   {355,1,0b00111},   {356,0,0b01111},   {357,1,0b11001},   {358,0,0b10011},   {359,1,0b11011},   {360,0,0b01001},   {361,1,0b10001},   {362,1,0b10101},   {363,0,0b01101},   {364,1,0b11101},   {365,1,0b01111},   {366,1,0b01101},   {367,0,0b00111},   {368,0,0b01101},   {369,1,0b11001},   {370,0,0b10011},   {371,1,0b11101},   {372,0,0b01001},   {373,0,0b01011},   {374,1,0b01111},   {375,0,0b10101},   {376,1,0b11101},   {377,0,0b00101},   {378,0,0b01011},   {379,1,0b11001},   {380,1,0b10001},   {381,0,0b00101},   {382,1,0b11101},   {383,0,0b01111},   {384,1,0b11111},   {385,0,0b10111},   {386,1,0b11111},   {387,1,0b01101},   {388,0,0b10011},   {389,1,0b10101},   {390,0,0b00001},   {391,0,0b00011},   {392,1,0b10011},   {393,0,0b01001},   {394,0,0b10011},   {395,1,0b11001},   {396,0,0b01011},   {397,1,0b01111},   {398,0,0b00111},   {399,1,0b11011},   {400,1,0b10111},   {401,0,0b01111},   {402,1,0b01111},   {403,1,0b00101},   {404,1,0b11111},   {405,1,0b11101},   {406,1,0b10101},   {407,1,0b01101},   {408,0,0b01111},   {409,0,0b10001},   {410,1,0b10001},   {411,0,0b10101},   {412,1,0b11111},   {413,0,0b01111},   {414,1,0b11101},   {415,0,0b00111},   {416,0,0b01001},   {417,1,0b10001},   {418,0,0b10011},   {419,1,0b11101},   {420,0,0b00001},   {421,1,0b01111},   {422,0,0b11001},   {423,1,0b11101},   {424,1,0b01101},   {425,1,0b11111},   {426,1,0b01101},   {427,0,0b00011},   {428,0,0b00101},   {429,0,0b11101},   {430,1,0b11111},   {431,0,0b00011},   {432,0,0b01001},   {433,1,0b11011},   {434,0,0b01101},   {435,1,0b11111},   {436,1,0b11001},   {437,0,0b01111},   {438,1,0b11101},   {439,1,0b11101},   {440,1,0b00101},   {441,0,0b01011},   {442,1,0b11011},   {443,0,0b00011},   {444,1,0b00101},   {445,1,0b11011},   {446,0,0b11111},   {447,1,0b11111},   {448,1,0b10101},   {449,0,0b00011},   {450,0,0b01111},   {451,0,0b10001},   {452,1,0b11011},   {453,0,0b01101},   {454,0,0b10101},   {455,1,0b11111},   {456,1,0b10111},   {457,1,0b01111},   {458,0,0b01101},   {459,1,0b10101},   {460,0,0b10101},   {461,1,0b11101},   {462,1,0b00111},   {463,1,0b11101},   {464,1,0b01101},   {465,0,0b00001},   {466,0,0b10001},   {467,1,0b10101},   {468,1,0b10001},   {469,0,0b00011},   {470,1,0b01101},   {471,1,0b00001},   {472,0,0b00001},   {473,1,0b01101},   {474,1,0b10011},   {475,1,0b11101},   {476,0,0b00001},   {477,1,0b11011},   {478,1,0b00001},   {479,1,0b01101},   {480,1,0b10011},   {481,1,0b00111},   {482,1,0b01111},   {483,1,0b01001},   {484,0,0b00111},   {485,1,0b11101},   {486,0,0b01011},   {487,1,0b10101},   {488,1,0b00111},   {489,0,0b00111},   {490,1,0b01001},   {491,1,0b11101},   {492,0,0b10001},   {493,1,0b11101},   {494,1,0b01001},   {495,1,0b10001},   {496,1,0b10101},   {497,0,0b00011},   {498,1,0b00111},   {500,0,0b00011},   {501,1,0b01101},   {502,1,0b10001},   {504,0,0b01111},   {505,1,0b10111},   {507,1,0b00011},   {508,1,0b10001},   {509,1,0b10101},   {513,0,0b01101},   {514,1,0b10001},   {515,0,0b00101},   {516,0,0b00111},   {517,1,0b11101},   {518,1,0b11111},   {519,0,0b00011},   {520,1,0b00011},   {521,0,0b00101},   {522,1,0b01111},   {523,0,0b00001},   {524,1,0b10111},   {525,0,0b00001},   {526,1,0b00101},   {527,1,0b01001},   {528,0,0b00101},   {529,1,0b01111},   {530,1,0b10111},   {531,1,0b00011},   {532,0,0b10001},   {533,1,0b11101},   {534,1,0b01001},   {535,0,0b00111},   {536,0,0b10111},   {537,1,0b11111},   {538,1,0b11001},   {539,0,0b00001},   {540,1,0b01101},   {542,1,0b11001},   {544,1,0b00011},   {545,0,0b00111},   {546,1,0b10111},   {548,0,0b00101},   {549,0,0b10101},   {550,1,0b11011},   {551,1,0b01101},   {553,0,0b01011},   {554,0,0b10001},   {555,1,0b11011},   {556,0,0b01111},   {557,1,0b11111},   {558,0,0b00011},   {559,0,0b01101},   {560,0,0b10011},   {561,1,0b10011},   {562,0,0b00001},   {563,1,0b11111},   {564,1,0b11001},   {565,1,0b01001},   {569,1,0b10101},   {570,0,0b10011},   {571,1,0b11001},   {572,1,0b10011},   {573,1,0b01011},   {574,1,0b01111},   {576,0,0b00011},   {577,0,0b01101},   {578,1,0b11111},   {579,0,0b00111},   {580,1,0b01001},   {581,1,0b01001},   {582,1,0b11001},   {583,1,0b10001},   {584,1,0b10011},   {586,0,0b01001},   {587,1,0b10101},   {588,0,0b10111},   {589,1,0b11111},   {590,1,0b10111},   {591,0,0b00001},   {592,0,0b00001},   {593,1,0b01011},   {594,1,0b01001},   {595,0,0b00011},   {596,1,0b10001},   {597,0,0b11101},   {598,1,0b11111},   {599,1,0b11111},   {600,1,0b11101},   {601,1,0b10111},   {602,1,0b01011},   {603,0,0b11101},   {604,1,0b11111},   {605,1,0b01011},   {606,1,0b11011},   {607,1,0b01011},   {608,0,0b10111},   {609,1,0b11011},   {610,0,0b01101},   {611,1,0b01111},   {612,1,0b01001},   {613,0,0b01111},   {614,1,0b10111},   {615,0,0b10011},   {616,0,0b10111},   {617,1,0b11001},   {618,1,0b01111},   {619,0,0b10011},   {620,1,0b10011},   {621,0,0b00101},   {622,1,0b01111},   {625,1,0b00011},   {626,0,0b00001},   {627,1,0b11011},   {629,1,0b11111},   {630,1,0b00101},   {633,0,0b01011},   {634,1,0b01101},   {635,0,0b01111},   {636,1,0b10101},   {637,1,0b10111},   {638,0,0b10101},   {639,0,0b11011},   {640,1,0b11011},   {641,1,0b00111},   {642,0,0b01111},   {643,1,0b01111},   {644,0,0b00011},   {645,0,0b00011},   {646,0,0b01111},   {647,0,0b10011},   {648,1,0b11011},   {649,1,0b01111},   {650,1,0b01111},   {651,1,0b00111},   {652,0,0b00001},   {653,1,0b00011},   {654,1,0b10011},   {655,0,0b11011},   {656,1,0b11101},   {657,0,0b00001},   {658,1,0b00011},   {659,1,0b00101},   {660,1,0b01011},   {661,1,0b00101},   {662,0,0b00011},   {663,1,0b01001},   {664,0,0b01101},   {665,1,0b11011},   {666,0,0b10001},   {667,1,0b10011},   {668,1,0b11101},   {670,1,0b11111},   {671,1,0b11001},   {673,0,0b10001},   {674,1,0b11011},   {675,0,0b00111},   {676,1,0b11001},   {677,0,0b00101},   {678,0,0b00101},   {679,1,0b01101},   {680,1,0b00011},   {681,1,0b10001},   {682,0,0b01111},   {683,1,0b01111},   {684,1,0b00001},   {686,1,0b10001},   {691,0,0b10001},   {692,1,0b11001},   {694,1,0b11001},   {696,0,0b10001},   {697,0,0b10001},   {698,1,0b10111},   {699,1,0b11011},   {700,1,0b11111},   {701,1,0b11111},   {703,1,0b00001},   {706,0,0b00111},   {707,0,0b01111},   {708,1,0b10111},   {712,0,0b00101},   {713,1,0b01101},   {714,1,0b01101},   {715,1,0b00011},   {718,0,0b10011},   {719,0,0b11011},   {720,1,0b11111},   {721,1,0b00001},   {722,0,0b00001},   {723,0,0b10011},   {724,1,0b11011},   {725,1,0b01001},   {726,0,0b00001},   {727,1,0b00011},   {728,1,0b10111},   {731,1,0b00101},   {732,1,0b11101},   {734,1,0b01111},   {735,0,0b10011},   {736,1,0b10111},   {738,0,0b00101},   {739,0,0b01101},   {740,1,0b10011},   {741,0,0b01101},   {742,1,0b01111},   {743,0,0b00101},   {744,1,0b01111},   {745,0,0b10101},   {746,1,0b10111},   {747,0,0b00111},   {748,1,0b10011},   {749,0,0b00011},   {750,1,0b10001},   {751,0,0b00011},   {752,1,0b10001},   {753,1,0b01111},   {754,1,0b11001},   {756,1,0b01101},   {760,0,0b00001},   {761,0,0b01001},   {762,0,0b11111},   {763,1,0b11111},   {764,0,0b01101},   {765,1,0b10101},   {766,1,0b01011},   {767,0,0b00111},   {768,0,0b01101},   {769,1,0b11011},   {770,1,0b00001},   {771,0,0b01011},   {772,0,0b01111},   {773,1,0b11001},   {774,0,0b00111},   {775,0,0b01011},   {776,0,0b11011},   {777,1,0b11011},   {778,0,0b01011},   {779,1,0b01011},   {780,0,0b01101},   {781,0,0b10001},   {782,1,0b11001},   {783,1,0b00101},   {784,1,0b10011},   {785,0,0b01101},   {786,1,0b11111},   {787,0,0b00111},   {788,0,0b01011},   {789,1,0b11001},   {790,1,0b11111},   {791,1,0b10111},   {792,0,0b00011},   {793,1,0b11011},   {794,0,0b11001},   {795,1,0b11011},   {796,0,0b00101},   {797,0,0b00101},   {798,1,0b11111},   {799,0,0b01011},   {800,1,0b10111},   {801,1,0b01101},   {802,1,0b01001},   {803,0,0b01001},   {804,1,0b10011},   {805,1,0b11001},   {806,0,0b10111},   {807,1,0b11011},   {808,0,0b10101},   {809,1,0b11101},   {810,1,0b11101},   {811,0,0b00111},   {812,0,0b01011},   {813,1,0b10001},   {814,0,0b00001},   {815,0,0b00011},   {816,1,0b00011},   {817,1,0b10111},   {818,1,0b00111},   {819,0,0b01001},   {820,1,0b01011},   {821,0,0b00011},   {822,1,0b10011},   {823,0,0b01001},   {824,0,0b11001},   {825,1,0b11001},   {826,1,0b10111},   {827,1,0b01011},   {828,0,0b00011},   {829,1,0b10001},   {830,0,0b00101},   {831,1,0b11001},   {832,1,0b10011},   {833,1,0b10111},   {834,0,0b00001},   {835,0,0b01001},   {836,1,0b11011},   {837,0,0b00101},   {838,0,0b01011},   {839,1,0b10001},   {840,0,0b01111},   {841,1,0b11001},   {842,1,0b00101},   {843,1,0b10011},   {844,1,0b11001},   {845,1,0b10011},   {846,1,0b10011},   {847,1,0b01111},   {848,1,0b01101},   {849,1,0b10101},   {850,0,0b00011},   {851,0,0b00111},   {852,1,0b10001},   {853,1,0b00101},   {854,0,0b00001},   {855,0,0b01101},   {856,1,0b10111},   {857,1,0b11001},   {858,0,0b11001},   {859,1,0b11011},   {860,1,0b10111},   {861,1,0b10011},   {862,1,0b00001},   {863,0,0b00111},   {864,1,0b11001},   {865,0,0b00011},   {866,1,0b10111},   {867,0,0b01101},   {868,1,0b10001},   {869,1,0b01001},   {870,0,0b10111},   {871,1,0b10111},   {872,1,0b10001},   {873,1,0b00011},   {874,1,0b01011},   {875,1,0b01101},   {876,0,0b00011},   {877,1,0b11111},   {878,1,0b11101},   {879,1,0b10111},   {880,1,0b00101},   {881,0,0b01001},   {882,1,0b11101},   {883,1,0b00111},   {884,0,0b10011},   {885,1,0b11101},   {886,0,0b00111},   {887,1,0b10101},   {888,0,0b00011},   {889,1,0b01111},   {890,0,0b01001},   {891,1,0b01111},   {892,0,0b00011},   {893,1,0b10011},   {894,0,0b01011},   {895,1,0b11001},   {896,0,0b00001},   {897,1,0b00101},   {898,1,0b01011},   {899,0,0b00011},   {900,0,0b00011},   {901,1,0b01111},   {902,0,0b01011},   {903,1,0b01101},   {904,0,0b00001},   {905,1,0b10011},   {906,0,0b00001},   {907,0,0b00001},   {908,0,0b00111},   {909,0,0b10011},   {910,0,0b11001},   {911,1,0b11001},   {912,1,0b10001},   {913,1,0b10111},   {914,0,0b00001},   {915,0,0b00011},   {916,1,0b11011},   {917,1,0b11101},   {918,0,0b00011},   {919,0,0b10001},   {920,1,0b10111},   {921,0,0b01001},   {922,0,0b10011},   {923,1,0b10111},   {924,0,0b00111},   {925,1,0b11101},   {926,1,0b00111},   {927,0,0b00001},   {928,1,0b00011},   {929,0,0b00011},   {930,1,0b11011},   {931,0,0b00001},   {932,0,0b01011},   {933,1,0b01011},   {934,0,0b00001},   {935,0,0b00111},   {936,0,0b01011},   {937,0,0b11101},   {938,1,0b11111},   {939,1,0b11101},   {940,0,0b00101},   {941,1,0b00111},   {942,1,0b00111},   {943,0,0b11001},   {944,1,0b11111},   {945,0,0b00111},   {946,1,0b11101},   {947,1,0b10101},   {948,1,0b11011},   {949,0,0b00101},   {950,1,0b11011},   {951,1,0b01001},   {952,1,0b00001},   {953,1,0b11011},   {954,1,0b11101},   {955,0,0b00001},   {956,1,0b00001},   {957,0,0b00101},   {958,0,0b00101},   {959,1,0b01111},   {960,1,0b00111},   {961,1,0b11101},   {962,0,0b11011},   {963,1,0b11111},   {964,0,0b00001},   {965,1,0b01101},   {966,0,0b00101},   {967,0,0b00111},   {968,1,0b10101},   {969,1,0b11001},   {970,0,0b00001},   {971,1,0b01011},   {972,0,0b00011},   {973,0,0b01011},   {974,1,0b10001},   {975,0,0b01111},   {976,0,0b10011},   {977,1,0b11011},   {978,0,0b01101},   {979,1,0b11101},   {980,0,0b10001},   {981,1,0b11101},   {982,0,0b01111},   {983,1,0b01111},   {984,0,0b01001},   {985,0,0b01001},   {986,0,0b01101},   {987,1,0b11111},   {988,1,0b10111},   {989,1,0b00111},   {990,0,0b00101},   {991,1,0b01101},   {992,0,0b01001},   {993,0,0b10101},   {994,1,0b11001},   {995,1,0b00101},   {996,1,0b00101},   {997,0,0b01011},   {998,0,0b01011},   {999,1,0b01011},   {1000,1,0b10111},   {1001,0,0b01001},   {1002,1,0b01111},   {1003,0,0b00001},   {1004,0,0b01001},   {1005,0,0b01001},   {1006,0,0b01011},   {1007,1,0b11011},   {1008,1,0b10101},   {1009,1,0b11111},   {1010,0,0b01001},   {1011,1,0b01101},   {1012,0,0b00001},   {1013,1,0b11101},   {1014,1,0b00001},   {1015,1,0b10111},   {1016,1,0b00001},   {1017,1,0b11011},   {1018,1,0b10011},   {1019,0,0b00001},   {1020,1,0b11101},   {1021,0,0b01001},   {1022,1,0b10101},   {1023,0,0b11001},   {1024,1,0b11101},   {1025,0,0b00001},   {1026,1,0b01011},   {1027,0,0b01111},   {1028,1,0b10101},   {1029,1,0b01001},   {1030,1,0b01101},   {1031,0,0b01111},   {1032,0,0b11011},   {1033,0,0b11101},   {1034,1,0b11111},   {1035,0,0b00001},   {1036,0,0b00011},   {1037,1,0b01111},   {1038,0,0b11001},   {1039,1,0b11111},   {1040,1,0b00111},   {1041,0,0b00001},   {1042,1,0b00011},   {1043,0,0b01011},   {1044,0,0b10011},   {1045,1,0b11101},   {1046,1,0b11011},   {1047,1,0b01001},   {1048,1,0b00101},   {1049,1,0b11001},   {1050,1,0b01011},   {1051,1,0b01111},   {1052,1,0b10101},   {1056,1,0b11011},   {1057,0,0b01011},   {1058,0,0b01101},   {1059,1,0b11011},   {1061,0,0b00111},   {1062,0,0b11011},   {1063,1,0b11101},   {1064,0,0b00101},   {1065,1,0b00101},   {1066,0,0b00101},   {1067,1,0b11001},   {1068,0,0b00011},   {1069,1,0b01101},   {1070,0,0b01101},   {1071,1,0b11011},   {1073,0,0b00101},   {1074,0,0b10001},   {1075,1,0b11001},   {1076,1,0b00011}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_int_iter_skip1(it);
                {
                    const size_t expected_boundary_length = 6;
                    const uint8_t expected_boundary[expected_boundary_length] = {0b11000111, 0b10011111, 0b10110001, 0b01100101, 0b10001101, 0b10001110};
                    const std::vector<uint32_t> occupieds_pos = {1, 2, 5, 9, 11, 13, 14, 16, 17, 21, 23, 28, 30, 31, 32, 33, 34, 37, 39, 41, 45, 47, 50, 52, 54, 58, 65, 66, 69, 72, 75, 76, 79, 86, 90, 97, 104, 105, 108, 110, 111, 113, 123, 126, 128, 129, 130, 131, 133, 134, 137, 138, 139, 140, 141, 142, 144, 146, 147, 148, 149, 150, 151, 152, 153, 154, 158, 160, 161, 163, 165, 172, 173, 174, 176, 181, 182, 183, 184, 186, 187, 190, 192, 193, 194, 195, 196, 198, 201, 207, 213, 214, 216, 217, 218, 220, 222, 223, 225, 226, 227, 232, 234, 237, 238, 240, 243, 244, 246, 248, 249, 250, 251, 252, 253, 256, 257, 258, 260, 261, 262, 266, 268, 274, 275, 276, 277, 279, 280, 281, 283, 285, 288, 289, 290, 291, 294, 295, 297, 298, 300, 301, 303, 305, 306, 313, 316, 318, 319, 325, 326, 327, 331, 333, 335, 336, 337, 338, 339, 343, 344, 345, 346, 351, 352, 361, 365, 366, 367, 369, 370, 374, 375, 382, 384, 387, 388, 389, 391, 392, 393, 394, 399, 401, 402, 406, 410, 411, 413, 414, 415, 423, 425, 426, 430, 431, 439, 442, 445, 446, 448, 450, 451, 452, 453, 454, 456, 462, 463, 466, 467, 469, 470, 472, 473, 474, 475, 476, 477, 479, 480, 482, 483, 486, 487, 493, 494, 495, 497, 498, 501, 504, 505, 506, 508, 510, 516, 519, 521, 525, 526, 532, 536, 539, 542, 545, 547, 551, 552, 553, 557, 558, 559, 564, 567, 572, 575, 578, 579, 580, 581, 583, 584, 586, 587, 589, 590, 593, 595, 596, 597, 599, 602, 607, 608, 609, 611, 612, 613, 614, 619, 620, 621, 624, 626, 629, 632, 638, 639, 641, 642, 644, 649, 651, 652, 653, 654, 657, 658, 660, 661, 664, 666, 668, 670, 671, 673, 674, 676, 678, 683, 684, 688, 690, 691, 693, 695, 700, 701, 702, 703, 705, 706, 707, 712, 713, 714, 716, 717, 718, 719, 720, 721, 722, 723, 724, 725, 726, 728, 731, 732, 733, 738, 740, 743, 744, 749, 750, 751, 757, 760, 761, 763, 764, 766, 767, 769, 770, 773, 775, 780, 781, 783, 784, 785, 786, 787, 788, 794, 796, 799, 800, 802, 803, 805, 806, 807, 813, 814, 817, 818, 820, 823, 825, 826, 831, 834, 840, 841, 843, 846, 849, 851, 855, 858, 861, 862, 864, 867, 868, 874, 875, 876, 880, 887, 891, 894, 895, 896, 899};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,1,0b00101},   {  2,0,0b01111},   {  3,0,0b10101},   {  4,1,0b11011},   {  5,1,0b00101},   { 10,1,0b01001},   { 13,1,0b10111},   { 15,1,0b00101},   { 16,1,0b10111},   { 19,1,0b10001},   { 20,1,0b10001},   { 25,0,0b00011},   { 26,0,0b00101},   { 27,0,0b11011},   { 28,1,0b11101},   { 29,0,0b00011},   { 30,1,0b11001},   { 33,0,0b10101},   { 34,1,0b11101},   { 35,1,0b11111},   { 37,1,0b11011},   { 38,1,0b11011},   { 39,0,0b00001},   { 40,1,0b10011},   { 41,1,0b01111},   { 44,1,0b11101},   { 46,1,0b00101},   { 48,1,0b10101},   { 53,0,0b11001},   { 54,1,0b11011},   { 56,1,0b11111},   { 59,0,0b01011},   { 60,1,0b01111},   { 62,0,0b00001},   { 63,1,0b01001},   { 64,1,0b00111},   { 69,0,0b10001},   { 70,1,0b10001},   { 77,1,0b11001},   { 78,0,0b10011},   { 79,1,0b11001},   { 82,0,0b00101},   { 83,1,0b11011},   { 85,1,0b01101},   { 89,1,0b11001},   { 90,1,0b10111},   { 94,1,0b10011},   {102,1,0b10111},   {107,1,0b00111},   {115,1,0b01111},   {124,1,0b01011},   {125,1,0b10001},   {128,1,0b01111},   {131,1,0b11111},   {132,1,0b10011},   {134,1,0b01101},   {146,1,0b11101},   {150,1,0b00011},   {152,1,0b11011},   {153,1,0b11011},   {155,1,0b00001},   {156,1,0b11011},   {158,1,0b11011},   {159,1,0b01101},   {163,1,0b00001},   {164,1,0b10001},   {165,1,0b00001},   {167,1,0b01001},   {168,1,0b00111},   {169,1,0b11111},   {171,1,0b00011},   {174,0,0b01011},   {175,1,0b10011},   {176,1,0b10111},   {177,1,0b10011},   {178,1,0b01111},   {179,1,0b00001},   {180,1,0b01111},   {181,1,0b00101},   {182,1,0b10111},   {183,1,0b10011},   {188,0,0b10001},   {189,1,0b10111},   {190,0,0b10111},   {191,1,0b11001},   {192,1,0b10001},   {194,0,0b11011},   {195,1,0b11111},   {196,0,0b00011},   {197,1,0b10111},   {205,1,0b10101},   {206,1,0b10001},   {207,1,0b10011},   {210,1,0b01001},   {216,1,0b10001},   {217,1,0b11101},   {218,1,0b10011},   {219,0,0b01011},   {220,1,0b10111},   {222,1,0b00001},   {223,1,0b01101},   {226,1,0b10101},   {229,1,0b11001},   {230,1,0b00111},   {231,1,0b11001},   {232,1,0b10111},   {233,1,0b00011},   {236,1,0b01011},   {239,1,0b10001},   {247,0,0b01101},   {248,0,0b10001},   {249,1,0b11101},   {254,1,0b01011},   {255,1,0b11101},   {257,0,0b01111},   {258,1,0b10001},   {259,0,0b00001},   {260,1,0b00011},   {261,1,0b10011},   {262,1,0b01001},   {264,1,0b10111},   {266,1,0b00101},   {268,1,0b00001},   {269,0,0b01011},   {270,1,0b10111},   {271,1,0b10111},   {276,1,0b00111},   {279,1,0b11111},   {282,1,0b11111},   {284,0,0b01001},   {285,1,0b11011},   {286,1,0b01101},   {290,1,0b00101},   {291,1,0b10111},   {293,1,0b00101},   {296,1,0b01101},   {297,1,0b10101},   {298,1,0b11011},   {299,0,0b01011},   {300,0,0b01101},   {301,1,0b01101},   {302,0,0b01011},   {303,1,0b11011},   {304,0,0b00011},   {305,1,0b10101},   {306,1,0b11101},   {307,0,0b01001},   {308,1,0b01101},   {309,0,0b01111},   {310,1,0b10111},   {311,0,0b10101},   {312,0,0b11011},   {313,1,0b11101},   {314,1,0b00111},   {315,0,0b00101},   {316,1,0b11111},   {317,1,0b10111},   {319,1,0b10111},   {327,1,0b10111},   {328,1,0b11111},   {329,0,0b00011},   {330,1,0b11101},   {331,0,0b10001},   {332,0,0b10111},   {333,1,0b11001},   {334,0,0b10111},   {335,1,0b11111},   {336,1,0b11011},   {337,1,0b00011},   {338,0,0b00011},   {339,0,0b00111},   {340,1,0b10101},   {341,0,0b10011},   {342,1,0b10101},   {343,1,0b01001},   {344,1,0b10011},   {346,0,0b01001},   {347,1,0b11111},   {348,1,0b11011},   {350,1,0b00111},   {352,1,0b10001},   {354,1,0b11111},   {355,1,0b10101},   {358,1,0b00001},   {359,1,0b01011},   {361,0,0b00001},   {362,1,0b01001},   {364,1,0b11001},   {365,0,0b01001},   {366,1,0b01011},   {373,1,0b00111},   {377,0,0b00101},   {378,1,0b01101},   {379,0,0b00001},   {380,1,0b11001},   {381,1,0b10101},   {387,1,0b11101},   {389,1,0b01101},   {390,1,0b01101},   {395,1,0b00001},   {397,1,0b01001},   {399,1,0b11101},   {401,1,0b01011},   {402,1,0b11101},   {403,1,0b11111},   {404,1,0b00111},   {409,1,0b00111},   {410,1,0b11011},   {411,1,0b11111},   {412,0,0b01001},   {413,1,0b10101},   {418,1,0b10111},   {420,1,0b11011},   {430,0,0b00101},   {431,1,0b01101},   {435,0,0b10001},   {436,1,0b10111},   {437,1,0b11101},   {438,1,0b11111},   {440,1,0b01011},   {441,0,0b01001},   {442,0,0b10101},   {443,1,0b10111},   {446,1,0b01111},   {447,0,0b00101},   {448,0,0b01001},   {449,0,0b11011},   {450,1,0b11101},   {455,1,0b10011},   {458,1,0b00101},   {461,1,0b10111},   {463,1,0b11011},   {464,0,0b01111},   {465,1,0b01111},   {466,0,0b00101},   {467,0,0b11001},   {468,1,0b11111},   {469,1,0b10001},   {470,1,0b10001},   {471,1,0b01101},   {476,1,0b01111},   {478,0,0b00011},   {479,1,0b01001},   {480,1,0b01111},   {484,1,0b01101},   {489,1,0b01101},   {490,1,0b10101},   {492,1,0b00011},   {494,0,0b10111},   {495,1,0b11111},   {496,1,0b10101},   {504,1,0b10001},   {507,0,0b10101},   {508,1,0b11001},   {509,1,0b01111},   {513,1,0b10111},   {514,0,0b00001},   {515,1,0b11001},   {524,0,0b00001},   {525,0,0b10101},   {526,0,0b11011},   {527,1,0b11101},   {528,1,0b11111},   {531,1,0b10001},   {532,1,0b11001},   {534,1,0b10111},   {537,0,0b01101},   {538,1,0b10101},   {539,0,0b10101},   {540,1,0b11101},   {541,1,0b00001},   {542,1,0b01001},   {543,1,0b11111},   {544,1,0b11101},   {551,0,0b01111},   {552,0,0b10111},   {553,1,0b11111},   {554,1,0b00011},   {556,0,0b01011},   {557,0,0b10001},   {558,1,0b11101},   {559,1,0b00111},   {560,1,0b10001},   {561,1,0b10011},   {563,1,0b00011},   {564,0,0b00101},   {565,1,0b01011},   {566,1,0b10111},   {567,1,0b01001},   {568,1,0b11111},   {569,1,0b10001},   {571,1,0b00101},   {572,0,0b00101},   {573,0,0b11111},   {574,1,0b11111},   {575,1,0b11101},   {576,1,0b11011},   {580,1,0b01101},   {581,1,0b00111},   {588,1,0b01001},   {589,1,0b11001},   {590,1,0b01101},   {593,1,0b11111},   {594,0,0b01101},   {595,1,0b01111},   {598,1,0b01001},   {601,1,0b11001},   {602,1,0b00001},   {603,1,0b01011},   {606,1,0b00101},   {608,0,0b00011},   {609,1,0b11111},   {615,0,0b00101},   {616,1,0b10101},   {619,1,0b01001},   {621,1,0b10011},   {626,1,0b10011},   {627,0,0b10001},   {628,1,0b11111},   {635,1,0b00101},   {639,1,0b00101},   {643,1,0b11111},   {646,1,0b11111},   {650,1,0b10011},   {652,1,0b11001},   {657,0,0b10011},   {658,1,0b11011},   {659,1,0b10001},   {660,0,0b00001},   {661,1,0b01011},   {664,1,0b11101},   {666,1,0b10011},   {667,1,0b11111},   {673,1,0b11011},   {676,1,0b11001},   {682,0,0b10101},   {683,1,0b11111},   {686,1,0b01111},   {689,1,0b10011},   {691,1,0b10001},   {692,0,0b00111},   {693,1,0b10001},   {694,1,0b10101},   {695,1,0b01011},   {697,1,0b11111},   {699,0,0b11001},   {700,1,0b11011},   {701,1,0b10001},   {703,0,0b10101},   {704,1,0b11111},   {705,1,0b01111},   {707,1,0b11101},   {710,0,0b00101},   {711,1,0b00111},   {712,1,0b00101},   {713,0,0b00011},   {714,1,0b01111},   {715,1,0b01011},   {718,0,0b00001},   {719,1,0b01101},   {724,1,0b10101},   {725,1,0b11111},   {726,1,0b00001},   {729,0,0b10001},   {730,1,0b10101},   {731,1,0b10001},   {732,1,0b10001},   {733,1,0b11101},   {738,1,0b11011},   {740,1,0b10011},   {741,0,0b00111},   {742,1,0b01101},   {744,0,0b00001},   {745,1,0b00111},   {747,1,0b01011},   {750,1,0b00001},   {754,1,0b11001},   {761,0,0b10101},   {762,1,0b10101},   {763,1,0b00111},   {765,1,0b01111},   {766,1,0b10001},   {768,0,0b00001},   {769,1,0b00101},   {774,0,0b10111},   {775,1,0b10111},   {777,0,0b00101},   {778,0,0b01001},   {779,1,0b11111},   {780,1,0b01011},   {781,0,0b10011},   {782,0,0b10011},   {783,1,0b11111},   {784,1,0b11101},   {785,0,0b00001},   {786,1,0b11111},   {787,1,0b01101},   {788,1,0b01001},   {789,1,0b01011},   {792,0,0b01001},   {793,0,0b01011},   {794,1,0b11101},   {795,0,0b01001},   {796,1,0b10001},   {797,1,0b10011},   {799,1,0b10111},   {800,0,0b01101},   {801,1,0b11111},   {803,1,0b01001},   {804,1,0b10111},   {806,0,0b00111},   {807,1,0b10001},   {809,0,0b01001},   {810,0,0b01011},   {811,1,0b10111},   {815,1,0b10001},   {816,0,0b01001},   {817,1,0b10101},   {821,1,0b00011},   {823,1,0b11011},   {824,1,0b11101},   {827,1,0b00001},   {829,0,0b11011},   {830,1,0b11111},   {835,1,0b01101},   {836,1,0b00101},   {837,1,0b01101},   {839,1,0b10011},   {841,1,0b00011},   {842,1,0b10101},   {843,1,0b01011},   {849,1,0b00001},   {851,1,0b10001},   {852,1,0b10001},   {854,1,0b01101},   {855,1,0b00111},   {857,1,0b10111},   {858,1,0b00011},   {859,0,0b00101},   {860,0,0b10101},   {861,1,0b11011},   {862,1,0b01101},   {863,1,0b00001},   {864,1,0b00001},   {865,1,0b00001},   {866,0,0b01011},   {867,1,0b10001},   {868,0,0b00011},   {869,0,0b00111},   {870,1,0b11001},   {871,1,0b00101},   {872,0,0b00111},   {873,1,0b11111},   {874,0,0b10011},   {875,0,0b10011},   {876,1,0b11101},   {877,0,0b10101},   {878,1,0b10101},   {880,1,0b11011},   {883,1,0b01111},   {886,0,0b00101},   {887,1,0b01001},   {888,1,0b00111},   {894,0,0b00111},   {895,1,0b10101},   {896,1,0b11111},   {897,1,0b11101},   {903,1,0b10001},   {907,1,0b00001},   {908,0,0b01111},   {909,1,0b01111},   {910,1,0b10111},   {911,0,0b10001},   {912,1,0b10011},   {914,0,0b00101},   {915,1,0b11001},   {916,1,0b10101},   {917,0,0b11011},   {918,1,0b11111},   {919,1,0b11111},   {922,0,0b00101},   {923,1,0b10001},   {925,1,0b10101},   {931,1,0b11001},   {932,1,0b01101},   {934,1,0b11001},   {935,0,0b10101},   {936,1,0b11111},   {937,1,0b00001},   {938,1,0b10011},   {939,1,0b01001},   {940,1,0b01101},   {947,1,0b00101},   {950,1,0b10111},   {953,0,0b00011},   {954,1,0b01111},   {955,1,0b10011},   {957,0,0b11111},   {958,1,0b11111},   {959,1,0b10011},   {960,1,0b10001},   {962,0,0b00111},   {963,0,0b01111},   {964,1,0b10011},   {965,1,0b11111},   {970,1,0b01001},   {971,1,0b10011},   {975,0,0b00111},   {976,1,0b01111},   {977,1,0b01111},   {978,1,0b01011},   {982,1,0b11101},   {984,0,0b10001},   {985,1,0b10011},   {986,1,0b00011},   {991,0,0b11011},   {992,1,0b11101},   {995,1,0b00011},   {1002,1,0b00101},   {1003,0,0b01011},   {1004,1,0b10011},   {1006,1,0b10011},   {1009,1,0b11001},   {1013,1,0b10011},   {1015,1,0b01101},   {1020,1,0b01001},   {1024,0,0b00101},   {1025,1,0b10111},   {1027,1,0b00101},   {1028,1,0b00001},   {1031,0,0b00001},   {1032,0,0b10101},   {1033,1,0b11001},   {1034,0,0b01011},   {1035,0,0b01011},   {1036,1,0b11001},   {1037,1,0b10001},   {1043,0,0b00101},   {1044,0,0b10011},   {1045,1,0b10011},   {1046,1,0b00111},   {1047,1,0b11011},   {1050,1,0b01111},   {1058,1,0b11111},   {1063,1,0b01101},   {1067,0,0b01101},   {1068,1,0b11101},   {1069,1,0b11111},   {1070,0,0b00111},   {1071,1,0b01011},   {1073,1,0b01101}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);
                    REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_int_iter_skip1(it);
                {
                    const size_t expected_boundary_length = 8;
                    const uint8_t expected_boundary[expected_boundary_length] =
                    {0b11111111, 0b11111111, 0b11111111, 0b11111111,
                        0b11111111, 0b11111111, 0b11111111, 0b11111111};
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_int_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                         reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }
                wh_int_iter_destroy(it);
            }
            else {
                wormhole_iter *it = wh_iter_create(s.better_tree_);
                wh_iter_seek(it, nullptr, 0);
                {
                    const size_t expected_boundary_length = 8;
                    const uint8_t expected_boundary[expected_boundary_length] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000};
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_iter_skip1(it);
                {
                    const size_t expected_boundary_length = 6;
                    const uint8_t expected_boundary[expected_boundary_length] = {0b00000000, 0b00010000, 0b00001100, 0b00111101, 0b11110111, 0b11101011};
                    const std::vector<uint32_t> occupieds_pos = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 20, 22, 23, 25, 27, 28, 29, 32, 34, 35, 36, 37, 38, 39, 40, 41, 42, 44, 45, 46, 47, 49, 51, 52, 54, 55, 56, 57, 59, 60, 61, 63, 64, 70, 71, 72, 73, 74, 75, 78, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 92, 94, 95, 98, 99, 100, 101, 102, 103, 104, 107, 108, 111, 113, 114, 115, 116, 117, 118, 120, 121, 122, 123, 125, 126, 127, 131, 132, 133, 134, 135, 137, 138, 139, 141, 144, 145, 146, 147, 148, 149, 151, 152, 153, 154, 155, 157, 159, 160, 162, 164, 165, 167, 168, 170, 174, 175, 176, 177, 179, 180, 181, 182, 184, 185, 186, 187, 189, 190, 192, 196, 197, 199, 201, 202, 203, 204, 205, 206, 207, 208, 210, 212, 213, 214, 216, 218, 220, 221, 222, 223, 225, 226, 228, 229, 230, 231, 234, 235, 237, 238, 239, 241, 242, 243, 244, 246, 248, 249, 250, 251, 253, 254, 255, 256, 257, 259, 261, 262, 263, 264, 265, 267, 268, 270, 271, 272, 274, 276, 277, 278, 280, 281, 282, 283, 285, 287, 288, 289, 291, 292, 293, 294, 296, 297, 298, 301, 302, 303, 304, 305, 308, 309, 310, 311, 313, 314, 315, 320, 321, 322, 323, 325, 326, 327, 328, 331, 332, 334, 335, 336, 338, 339, 340, 341, 342, 344, 345, 346, 348, 349, 351, 354, 355, 356, 357, 358, 359, 360, 363, 364, 365, 366, 367, 369, 371, 372, 373, 374, 375, 376, 378, 379, 381, 382, 383, 384, 385, 386, 388, 389, 390, 391, 394, 395, 396, 397, 398, 399, 400, 402, 403, 404, 406, 408, 409, 410, 411, 414, 415, 416, 417, 419, 420, 421, 422, 424, 428, 429, 430, 432, 433, 435, 437, 438, 439, 440, 441, 443, 444, 445, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 459, 460, 461, 462, 463, 464, 466, 467, 468, 469, 470, 471, 472, 473, 474, 476, 477, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 492, 494, 495, 496, 498, 499, 500, 501, 502, 503, 504, 506, 508, 509, 510, 511, 512, 513, 514, 516, 517, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 531, 532, 535, 537, 538, 540, 541, 542, 546, 548, 549, 550, 551, 552, 553, 554, 555, 557, 558, 559, 561, 562, 564, 565, 566, 568, 570, 572, 573, 574, 575, 578, 579, 580, 584, 585, 586, 587, 588, 589, 590, 591, 594, 595, 596, 597, 598, 599, 601, 602, 603, 604, 607, 608, 612, 613, 614, 616, 617, 618, 619, 620, 621, 623, 624, 625, 628, 629, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650, 651, 653, 655, 657, 658, 659, 660, 662, 664, 665, 666, 668, 669, 670, 672, 673, 674, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 689, 690, 691, 692, 693, 694, 697, 698, 700, 701, 702, 703, 705, 706, 707, 708, 709, 710, 711, 713, 714, 715, 718, 720, 721, 722, 724, 725, 727, 728, 730, 731, 732, 733, 736, 737, 738, 744, 745, 746, 747, 748, 750, 753, 754, 755, 756, 758, 759, 761, 762, 763, 765, 767, 768, 769, 770, 771, 772, 773, 774, 775, 777, 778, 779, 780, 781, 783, 784};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,0,0b00001},   {  2,0,0b01101},   {  3,0,0b10011},   {  4,0,0b10101},   {  5,1,0b11111},   {  6,1,0b11011},   {  7,0,0b10101},   {  8,1,0b10111},   {  9,1,0b01101},   { 10,0,0b00011},   { 11,0,0b10101},   { 12,1,0b11101},   { 13,1,0b00011},   { 14,0,0b00001},   { 15,0,0b00001},   { 16,1,0b01101},   { 17,1,0b10011},   { 18,0,0b10101},   { 19,1,0b11011},   { 20,1,0b00011},   { 21,1,0b01011},   { 22,0,0b00111},   { 23,0,0b01011},   { 24,0,0b11011},   { 25,1,0b11111},   { 26,0,0b00101},   { 27,1,0b11101},   { 28,1,0b01001},   { 29,1,0b10001},   { 30,1,0b10011},   { 31,1,0b11011},   { 32,0,0b00001},   { 33,0,0b00111},   { 34,0,0b11011},   { 35,0,0b11011},   { 36,1,0b11101},   { 37,1,0b00101},   { 38,1,0b11111},   { 39,0,0b01001},   { 40,0,0b10011},   { 41,0,0b10011},   { 42,1,0b11111},   { 43,1,0b11101},   { 44,0,0b01101},   { 45,0,0b01101},   { 46,1,0b11101},   { 47,1,0b01101},   { 48,0,0b00011},   { 49,0,0b11001},   { 50,1,0b11011},   { 51,1,0b01001},   { 52,1,0b00111},   { 53,0,0b00011},   { 54,1,0b00111},   { 55,1,0b10011},   { 56,0,0b00001},   { 57,1,0b11001},   { 58,0,0b00001},   { 59,0,0b00001},   { 60,1,0b01001},   { 61,0,0b00101},   { 62,0,0b10011},   { 63,1,0b10111},   { 64,1,0b00011},   { 65,1,0b00111},   { 66,0,0b10001},   { 67,1,0b10001},   { 68,1,0b01101},   { 69,0,0b00001},   { 70,0,0b01101},   { 71,1,0b11011},   { 72,0,0b01101},   { 73,0,0b10111},   { 74,0,0b11001},   { 75,1,0b11101},   { 76,0,0b00111},   { 77,0,0b00111},   { 78,0,0b01011},   { 79,1,0b11011},   { 80,0,0b01001},   { 81,0,0b01101},   { 82,0,0b01101},   { 83,1,0b11111},   { 84,1,0b11111},   { 85,0,0b10011},   { 86,0,0b11001},   { 87,1,0b11001},   { 88,1,0b00101},   { 89,0,0b00001},   { 90,1,0b00111},   { 91,1,0b01011},   { 92,0,0b01011},   { 93,0,0b11011},   { 94,1,0b11011},   { 95,1,0b10111},   { 96,0,0b01001},   { 97,1,0b10111},   { 98,0,0b10011},   { 99,1,0b11011},   {100,1,0b11001},   {101,1,0b00101},   {102,1,0b11001},   {103,1,0b01101},   {104,0,0b10111},   {105,1,0b11111},   {106,1,0b11001},   {109,0,0b00101},   {110,1,0b01111},   {111,1,0b01001},   {112,0,0b11101},   {113,1,0b11101},   {114,0,0b01111},   {115,0,0b10001},   {116,1,0b11101},   {117,0,0b00101},   {118,1,0b11001},   {119,1,0b11011},   {120,1,0b00111},   {121,0,0b01011},   {122,0,0b01011},   {123,0,0b01101},   {124,1,0b11111},   {125,1,0b10101},   {126,0,0b10011},   {127,1,0b10101},   {128,1,0b11001},   {129,0,0b00011},   {130,1,0b10111},   {131,1,0b01111},   {134,1,0b01011},   {135,1,0b00111},   {137,1,0b10111},   {138,0,0b01001},   {139,1,0b01111},   {140,1,0b00101},   {141,0,0b01001},   {142,1,0b01011},   {143,0,0b01111},   {144,1,0b10011},   {146,1,0b01011},   {148,0,0b00001},   {149,0,0b10011},   {150,1,0b11001},   {152,1,0b01011},   {154,0,0b10001},   {155,0,0b10101},   {156,1,0b10101},   {157,1,0b10011},   {158,1,0b11001},   {159,0,0b10001},   {160,0,0b10101},   {161,1,0b11111},   {162,0,0b00001},   {163,0,0b00111},   {164,1,0b01011},   {165,1,0b01011},   {166,1,0b11101},   {167,0,0b00111},   {168,1,0b01011},   {169,0,0b00101},   {170,1,0b10101},   {171,1,0b11101},   {172,0,0b00101},   {173,1,0b11001},   {174,0,0b00111},   {175,1,0b11101},   {176,0,0b00111},   {177,1,0b11001},   {179,0,0b01001},   {180,1,0b11011},   {181,0,0b00011},   {182,1,0b11111},   {183,1,0b11111},   {184,0,0b10111},   {185,1,0b10111},   {186,0,0b00101},   {187,1,0b11101},   {188,0,0b10001},   {189,0,0b10101},   {190,1,0b11111},   {191,0,0b01111},   {192,0,0b10111},   {193,1,0b11111},   {194,1,0b10111},   {195,1,0b01011},   {197,0,0b01011},   {198,1,0b10101},   {199,1,0b10111},   {200,0,0b10101},   {201,1,0b11001},   {202,1,0b00101},   {203,1,0b11011},   {204,1,0b01011},   {207,0,0b00001},   {208,0,0b01011},   {209,0,0b01011},   {210,1,0b01111},   {211,1,0b11111},   {212,1,0b10111},   {213,1,0b01111},   {214,1,0b11011},   {215,0,0b01011},   {216,1,0b01101},   {218,1,0b01001},   {219,0,0b11011},   {220,0,0b11011},   {221,1,0b11111},   {222,1,0b10101},   {224,1,0b01011},   {226,1,0b11111},   {229,1,0b11001},   {230,1,0b01011},   {233,0,0b01011},   {234,1,0b10111},   {238,1,0b01111},   {239,0,0b11101},   {240,0,0b11101},   {241,0,0b11111},   {242,1,0b11111},   {243,0,0b00001},   {244,0,0b01001},   {245,1,0b10101},   {246,1,0b01111},   {247,0,0b00011},   {248,1,0b11001},   {249,0,0b00011},   {250,0,0b00101},   {251,1,0b11011},   {252,0,0b00101},   {253,1,0b01111},   {254,0,0b00001},   {255,0,0b11001},   {256,1,0b11111},   {257,0,0b00001},   {258,0,0b01011},   {259,1,0b11101},   {260,1,0b01011},   {261,0,0b01101},   {262,1,0b01111},   {263,0,0b11011},   {264,1,0b11101},   {265,1,0b00011},   {266,0,0b00011},   {267,1,0b11011},   {268,0,0b00101},   {269,1,0b10001},   {270,1,0b11101},   {271,1,0b11001},   {272,0,0b01101},   {273,1,0b11001},   {275,1,0b10101},   {277,0,0b11001},   {278,1,0b11001},   {279,1,0b10101},   {280,0,0b00011},   {281,1,0b00101},   {282,1,0b00101},   {283,0,0b10001},   {284,1,0b11111},   {285,1,0b10101},   {286,1,0b10111},   {287,0,0b00101},   {288,1,0b01101},   {290,1,0b01011},   {292,1,0b01111},   {293,1,0b00111},   {296,0,0b01011},   {297,1,0b01101},   {298,1,0b11001},   {301,0,0b00001},   {302,1,0b10101},   {303,0,0b00001},   {304,1,0b01011},   {305,1,0b11001},   {306,1,0b01011},   {308,0,0b01001},   {309,1,0b10001},   {310,1,0b01001},   {312,0,0b01011},   {313,1,0b01011},   {314,0,0b01011},   {315,0,0b01011},   {316,1,0b01111},   {317,0,0b00011},   {318,1,0b11011},   {319,1,0b10011},   {320,0,0b10001},   {321,1,0b11001},   {322,1,0b00001},   {325,0,0b01101},   {326,0,0b01101},   {327,1,0b11001},   {328,0,0b01101},   {329,1,0b10111},   {330,0,0b00001},   {331,1,0b10011},   {332,0,0b00011},   {333,1,0b10111},   {334,1,0b11101},   {335,0,0b01111},   {336,0,0b10111},   {337,1,0b11001},   {338,1,0b10011},   {339,1,0b01101},   {340,1,0b01011},   {341,1,0b11111},   {342,0,0b01001},   {343,1,0b11001},   {344,1,0b00001},   {346,0,0b10001},   {347,0,0b10111},   {348,1,0b11101},   {349,0,0b00111},   {350,1,0b10001},   {351,1,0b10001},   {352,0,0b01011},   {353,1,0b11001},   {354,1,0b00011},   {355,1,0b00111},   {357,0,0b10011},   {358,1,0b11101},   {359,1,0b00011},   {360,0,0b01111},   {361,1,0b10001},   {362,1,0b01111},   {363,1,0b10101},   {366,1,0b11111},   {367,1,0b00111},   {370,1,0b11001},   {371,1,0b11101},   {373,1,0b00011},   {375,1,0b00101},   {378,1,0b00111},   {379,1,0b11111},   {381,0,0b01001},   {382,0,0b10011},   {383,0,0b11001},   {384,1,0b11011},   {385,0,0b00111},   {386,1,0b01101},   {387,0,0b00011},   {388,0,0b00101},   {389,0,0b01101},   {390,0,0b10111},   {391,0,0b11001},   {392,1,0b11111},   {393,1,0b10111},   {394,1,0b00001},   {395,0,0b01101},   {396,1,0b01101},   {397,0,0b00001},   {398,0,0b01011},   {399,1,0b10111},   {400,1,0b11111},   {401,0,0b00101},   {402,1,0b01001},   {403,1,0b01101},   {404,0,0b10001},   {405,0,0b10011},   {406,1,0b10101},   {407,1,0b01011},   {408,1,0b10001},   {409,0,0b01011},   {410,1,0b01111},   {411,1,0b11011},   {412,0,0b00101},   {413,1,0b10001},   {414,0,0b00111},   {415,0,0b11001},   {416,0,0b11001},   {417,1,0b11101},   {418,1,0b01001},   {419,0,0b00111},   {420,1,0b11011},   {421,1,0b11101},   {422,1,0b01111},   {423,1,0b11001},   {424,0,0b00011},   {425,1,0b10111},   {426,0,0b00101},   {427,1,0b11001},   {428,0,0b10011},   {429,1,0b10111},   {430,1,0b10011},   {431,1,0b01011},   {432,0,0b01111},   {433,1,0b11111},   {438,1,0b10111},   {440,1,0b00011},   {441,1,0b10011},   {442,1,0b01111},   {445,1,0b11111},   {446,1,0b01001},   {447,0,0b00001},   {448,0,0b00001},   {449,0,0b01101},   {450,0,0b01111},   {451,0,0b10101},   {452,0,0b11101},   {453,1,0b11111},   {454,0,0b10001},   {455,1,0b11111},   {456,0,0b10001},   {457,1,0b11101},   {458,0,0b00101},   {459,1,0b01001},   {460,0,0b00101},   {461,0,0b10011},   {462,0,0b11001},   {463,0,0b11101},   {464,1,0b11101},   {465,1,0b01111},   {466,0,0b00011},   {467,1,0b10111},   {468,1,0b00001},   {469,1,0b01111},   {470,1,0b00111},   {471,1,0b10011},   {472,0,0b11001},   {473,0,0b11001},   {474,1,0b11101},   {475,1,0b10111},   {476,1,0b11111},   {477,0,0b00011},   {478,1,0b01001},   {479,0,0b00001},   {480,0,0b10101},   {481,1,0b11101},   {482,0,0b10001},   {483,1,0b10011},   {484,1,0b00001},   {485,1,0b00001},   {486,0,0b01111},   {487,1,0b01111},   {488,1,0b00111},   {489,0,0b00011},   {490,0,0b00101},   {491,0,0b01101},   {492,1,0b10001},   {493,0,0b00011},   {494,1,0b10111},   {495,0,0b01001},   {496,1,0b10101},   {497,0,0b00111},   {498,1,0b01011},   {499,0,0b00001},   {500,1,0b10001},   {501,0,0b01011},   {502,0,0b10011},   {503,1,0b11111},   {504,1,0b11011},   {505,1,0b10011},   {506,1,0b10001},   {507,0,0b00111},   {508,1,0b10001},   {509,1,0b01101},   {510,0,0b10111},   {511,1,0b11101},   {512,1,0b11101},   {513,0,0b00111},   {514,0,0b11001},   {515,1,0b11101},   {516,1,0b01011},   {517,0,0b10001},   {518,1,0b10111},   {519,0,0b00011},   {520,1,0b00111},   {521,1,0b00101},   {522,0,0b10101},   {523,1,0b11001},   {524,0,0b00101},   {525,1,0b10101},   {526,1,0b10001},   {527,0,0b01001},   {528,1,0b11111},   {529,0,0b01101},   {530,1,0b01111},   {531,1,0b11001},   {532,0,0b00101},   {533,1,0b00111},   {534,1,0b01011},   {535,0,0b00111},   {536,0,0b10001},   {537,0,0b10101},   {538,1,0b10111},   {539,0,0b00011},   {540,0,0b01001},   {541,1,0b10011},   {542,1,0b11101},   {543,0,0b01001},   {544,0,0b01001},   {545,0,0b01011},   {546,1,0b11011},   {547,0,0b01101},   {548,0,0b10101},   {549,1,0b10101},   {550,0,0b00001},   {551,0,0b00101},   {552,1,0b01111},   {553,1,0b01011},   {554,0,0b10101},   {555,1,0b11101},   {556,1,0b10011},   {557,0,0b01001},   {558,0,0b10001},   {559,1,0b10101},   {560,0,0b00101},   {561,0,0b01111},   {562,0,0b10101},   {563,0,0b10111},   {564,1,0b11101},   {565,1,0b01111},   {566,1,0b00101},   {567,0,0b00101},   {568,0,0b00111},   {569,1,0b01001},   {570,1,0b01011},   {571,1,0b01111},   {572,1,0b00101},   {573,1,0b00111},   {574,1,0b11011},   {575,0,0b00111},   {576,1,0b10101},   {577,1,0b01111},   {578,1,0b11001},   {579,1,0b00001},   {580,1,0b00001},   {581,1,0b00001},   {582,0,0b01011},   {583,1,0b11011},   {584,0,0b00011},   {585,0,0b01001},   {586,0,0b10011},   {587,1,0b11011},   {588,0,0b10001},   {589,1,0b11001},   {590,1,0b00101},   {591,1,0b10111},   {592,0,0b10011},   {593,0,0b10101},   {594,1,0b11001},   {595,0,0b10011},   {596,1,0b10101},   {597,1,0b10011},   {598,1,0b11001},   {599,0,0b00111},   {600,0,0b10101},   {601,0,0b10101},   {602,1,0b11101},   {603,0,0b10011},   {604,1,0b10011},   {605,1,0b01001},   {606,1,0b01011},   {607,1,0b00111},   {608,0,0b00001},   {609,0,0b00011},   {610,0,0b10001},   {611,1,0b11001},   {612,0,0b00001},   {613,0,0b01011},   {614,1,0b11001},   {615,1,0b00111},   {616,1,0b11001},   {617,0,0b00111},   {618,0,0b01101},   {619,0,0b10001},   {620,1,0b10001},   {621,1,0b11011},   {622,0,0b01101},   {623,1,0b11101},   {624,0,0b00011},   {625,1,0b01111},   {626,1,0b11101},   {627,0,0b00111},   {628,1,0b01001},   {629,0,0b00001},   {630,1,0b11011},   {631,0,0b00101},   {632,1,0b10111},   {633,0,0b00111},   {634,1,0b11011},   {635,1,0b11101},   {636,1,0b00011},   {637,1,0b01111},   {638,0,0b00011},   {639,0,0b01101},   {640,1,0b11011},   {641,1,0b00011},   {642,0,0b00111},   {643,0,0b01001},   {644,0,0b01011},   {645,0,0b10111},   {646,0,0b11001},   {647,1,0b11101},   {648,1,0b01011},   {649,1,0b10111},   {650,1,0b00011},   {651,0,0b00101},   {652,0,0b10111},   {653,0,0b10111},   {654,1,0b11011},   {655,0,0b01011},   {656,1,0b11011},   {657,0,0b00111},   {658,0,0b01101},   {659,0,0b10011},   {660,1,0b11111},   {661,1,0b01011},   {662,0,0b10101},   {663,1,0b11011},   {664,0,0b01011},   {665,0,0b01111},   {666,1,0b11001},   {667,1,0b11011},   {668,1,0b00111},   {669,1,0b00101},   {670,1,0b01001},   {671,1,0b01011},   {672,1,0b11101},   {673,1,0b10011},   {674,0,0b01001},   {675,1,0b11001},   {676,0,0b00101},   {677,1,0b01111},   {678,0,0b00001},   {679,1,0b10001},   {680,0,0b00101},   {681,1,0b11011},   {682,1,0b10101},   {683,0,0b01111},   {684,0,0b10111},   {685,1,0b11111},   {686,0,0b00101},   {687,0,0b01011},   {688,0,0b10011},   {689,1,0b11011},   {690,1,0b01001},   {691,0,0b00101},   {692,1,0b11001},   {693,0,0b10011},   {694,1,0b10101},   {695,1,0b10001},   {696,1,0b10111},   {697,1,0b00111},   {698,1,0b11001},   {699,1,0b00001},   {700,1,0b01101},   {701,1,0b11111},   {702,1,0b00001},   {703,0,0b00111},   {704,1,0b11001},   {705,1,0b10001},   {706,0,0b11101},   {707,1,0b11101},   {708,0,0b10111},   {709,1,0b11111},   {710,0,0b00001},   {711,0,0b00101},   {712,1,0b01011},   {713,0,0b00011},   {714,1,0b10111},   {715,0,0b00011},   {716,1,0b00101},   {717,0,0b01001},   {718,0,0b10101},   {719,1,0b11111},   {720,0,0b01011},   {721,1,0b10111},   {722,1,0b01111},   {723,0,0b01011},   {724,1,0b10101},   {725,0,0b01111},   {726,1,0b11011},   {727,0,0b00111},   {728,0,0b10011},   {729,1,0b10111},   {730,0,0b00111},   {731,0,0b10001},   {732,1,0b10011},   {733,1,0b00001},   {734,0,0b01111},   {735,0,0b01111},   {736,1,0b10001},   {737,1,0b01001},   {738,0,0b00001},   {739,1,0b00111},   {740,1,0b11011},   {741,0,0b10001},   {742,0,0b10001},   {743,0,0b10001},   {744,1,0b10111},   {745,1,0b10001},   {746,1,0b10001},   {747,0,0b01001},   {748,0,0b01101},   {749,0,0b11001},   {750,1,0b11011},   {751,1,0b10001},   {752,0,0b00001},   {753,0,0b00011},   {754,1,0b00101},   {755,1,0b00111},   {756,0,0b00011},   {757,1,0b01001},   {758,1,0b00111},   {759,0,0b00001},   {760,1,0b11101},   {761,1,0b01111},   {762,1,0b00111},   {763,0,0b01001},   {764,1,0b10001},   {765,1,0b00111},   {766,1,0b01011},   {767,1,0b10011},   {768,0,0b01101},   {769,1,0b10001},   {770,1,0b10001},   {771,0,0b00011},   {772,0,0b00101},   {773,1,0b11101},   {774,1,0b10111},   {775,1,0b10101},   {776,1,0b10001},   {777,1,0b11101},   {778,0,0b11001},   {779,1,0b11101},   {780,0,0b01011},   {781,1,0b11001},   {782,0,0b10001},   {783,1,0b10101},   {784,1,0b00001},   {785,0,0b10001},   {786,1,0b11101},   {787,1,0b11111},   {788,0,0b00011},   {789,1,0b10001},   {790,1,0b01011},   {791,1,0b10101},   {792,0,0b11001},   {793,1,0b11101},   {794,0,0b10001},   {795,1,0b10101},   {796,1,0b01001},   {797,1,0b11011},   {798,0,0b00111},   {799,0,0b01001},   {800,0,0b10111},   {801,1,0b11111},   {802,1,0b11101},   {803,1,0b10101},   {804,0,0b00011},   {805,1,0b10001},   {806,1,0b11111},   {807,1,0b10111},   {808,1,0b01111},   {809,0,0b01011},   {810,1,0b11011},   {811,1,0b10101},   {812,0,0b00011},   {813,1,0b01101},   {814,0,0b00101},   {815,0,0b01001},   {816,1,0b10001},   {817,0,0b01101},   {818,1,0b11111},   {819,1,0b01001},   {820,0,0b10101},   {821,0,0b10111},   {822,1,0b11111},   {823,0,0b00001},   {824,1,0b10011},   {825,1,0b00011},   {826,1,0b01101},   {827,1,0b11011},   {828,1,0b00001},   {829,1,0b11011},   {830,1,0b10011},   {831,0,0b01101},   {832,1,0b10001},   {833,0,0b00011},   {834,1,0b10011},   {835,0,0b00101},   {836,1,0b01001},   {837,0,0b01111},   {838,1,0b10011},   {839,0,0b10011},   {840,1,0b11111},   {841,1,0b01001},   {842,1,0b01101},   {843,0,0b01111},   {844,1,0b10101},   {845,0,0b00011},   {846,1,0b11111},   {847,1,0b00111},   {848,0,0b00001},   {849,0,0b00111},   {850,0,0b10101},   {851,0,0b10111},   {852,1,0b11101},   {853,0,0b01111},   {854,1,0b11001},   {855,1,0b10001},   {856,1,0b01111},   {857,0,0b10111},   {858,0,0b11001},   {859,1,0b11001},   {860,0,0b10101},   {861,1,0b11111},   {862,1,0b10011},   {863,0,0b01011},   {864,1,0b11011},   {865,1,0b11011},   {866,0,0b10011},   {867,1,0b10101},   {868,0,0b00101},   {869,1,0b00101},   {870,0,0b00011},   {871,0,0b10111},   {872,1,0b11011},   {873,0,0b01001},   {874,0,0b01001},   {875,1,0b11101},   {876,0,0b01001},   {877,0,0b10101},   {878,1,0b11101},   {879,0,0b11001},   {880,0,0b11111},   {881,1,0b11111},   {882,0,0b01011},   {883,1,0b11101},   {884,0,0b00111},   {885,1,0b10111},   {886,0,0b00101},   {887,1,0b01011},   {888,0,0b00001},   {889,1,0b00011},   {890,0,0b00111},   {891,0,0b01001},   {892,1,0b01101},   {893,0,0b00011},   {894,1,0b10011},   {895,1,0b01111},   {896,1,0b10111},   {897,0,0b01001},   {898,0,0b01111},   {899,0,0b10011},   {900,1,0b11111},   {901,1,0b11001},   {902,0,0b00011},   {903,1,0b10001},   {904,1,0b01011},   {905,0,0b00101},   {906,1,0b01001},   {907,0,0b10011},   {908,0,0b10111},   {909,0,0b11011},   {910,1,0b11111},   {911,1,0b11111},   {912,1,0b00111},   {913,1,0b01101},   {914,1,0b11101},   {915,0,0b10101},   {916,0,0b11011},   {917,1,0b11011},   {918,0,0b01111},   {919,0,0b10001},   {920,1,0b11101},   {921,0,0b01011},   {922,1,0b10111},   {923,0,0b00001},   {924,0,0b01011},   {925,1,0b10001},   {926,0,0b00011},   {927,1,0b01101},   {928,1,0b11001},   {929,0,0b00111},   {930,0,0b01101},   {931,0,0b11011},   {932,1,0b11111},   {933,1,0b11011},   {934,1,0b11011},   {935,0,0b10101},   {936,1,0b11001},   {937,1,0b00111},   {938,1,0b01011},   {939,0,0b00101},   {940,1,0b00111},   {941,1,0b01001},   {942,1,0b11111},   {943,0,0b01101},   {944,1,0b11101},   {945,0,0b00001},   {946,1,0b11001},   {947,0,0b01001},   {948,1,0b01111},   {949,1,0b00001},   {950,0,0b00101},   {951,0,0b01111},   {952,0,0b11001},   {953,1,0b11001},   {954,0,0b10001},   {955,1,0b11011},   {956,0,0b01101},   {957,1,0b01101},   {958,1,0b01101},   {959,0,0b00111},   {960,0,0b01011},   {961,1,0b10011},   {962,0,0b00111},   {963,0,0b10001},   {964,0,0b10101},   {965,1,0b11001},   {966,1,0b11011},   {967,1,0b11101},   {968,1,0b01001},   {969,0,0b00101},   {970,0,0b00111},   {971,1,0b10111},   {972,1,0b00001},   {973,0,0b01101},   {974,1,0b10101},   {975,1,0b11111},   {976,1,0b01111},   {977,0,0b00101},   {978,0,0b10001},   {979,1,0b11111},   {980,1,0b10111},   {981,0,0b01011},   {982,0,0b01101},   {983,1,0b10101},   {984,1,0b11101},   {985,1,0b10011},   {986,0,0b10101},   {987,1,0b11111},   {988,0,0b10011},   {989,1,0b11111},   {990,0,0b00111},   {991,1,0b10111},   {992,0,0b10101},   {993,0,0b10101},   {994,1,0b11011},   {995,0,0b01001},   {996,1,0b10101},   {997,1,0b01011},   {998,1,0b10101},   {999,1,0b11001},   {1000,1,0b01111},   {1001,1,0b00011},   {1002,0,0b00001},   {1003,0,0b00011},   {1004,0,0b00101},   {1005,1,0b01011},   {1006,1,0b00101},   {1007,1,0b00101},   {1008,0,0b00111},   {1009,1,0b01011},   {1010,0,0b11001},   {1011,1,0b11111},   {1012,0,0b10101},   {1013,0,0b11101},   {1014,1,0b11111},   {1015,1,0b10101},   {1016,0,0b01011},   {1017,0,0b10111},   {1018,1,0b11111},   {1019,1,0b11011},   {1020,1,0b10101},   {1021,0,0b01001},   {1022,0,0b10001},   {1023,1,0b11111},   {1024,1,0b11001},   {1025,0,0b00101},   {1026,0,0b10001},   {1027,1,0b11001},   {1028,0,0b10111},   {1029,1,0b10111},   {1030,0,0b11011},   {1031,1,0b11111},   {1032,1,0b11111},   {1033,0,0b01011},   {1034,0,0b01011},   {1035,1,0b10011},   {1036,0,0b01101},   {1037,1,0b10011},   {1038,0,0b00001},   {1039,1,0b01111},   {1040,1,0b00011},   {1041,0,0b10101},   {1042,1,0b11101},   {1043,0,0b11001},   {1044,1,0b11011},   {1045,1,0b01101},   {1046,0,0b00101},   {1047,0,0b01011},   {1048,0,0b10101},   {1049,0,0b11101},   {1050,1,0b11101},   {1051,0,0b00001},   {1052,0,0b01101},   {1053,1,0b10001},   {1054,0,0b00101},   {1055,0,0b00111},   {1056,1,0b01001},   {1057,0,0b11101},   {1058,1,0b11101},   {1059,1,0b10001},   {1060,1,0b10101},   {1061,0,0b00101},   {1062,1,0b10101},   {1063,0,0b00011},   {1064,0,0b00101},   {1065,1,0b10111},   {1066,0,0b01011},   {1067,1,0b01011},   {1068,0,0b00101},   {1069,1,0b01011},   {1070,1,0b01001},   {1071,0,0b00011},   {1072,1,0b00101},   {1073,0,0b10001},   {1074,1,0b11101},   {1075,1,0b01011},   {1076,1,0b11001}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_iter_skip1(it);
                {
                    const size_t expected_boundary_length = 6;
                    const uint8_t expected_boundary[expected_boundary_length] = {0b01100010, 0b00101001, 0b11111000, 0b10110111, 0b10010000, 0b00101011};
                    const std::vector<uint32_t> occupieds_pos = {0, 2, 4, 6, 7, 8, 9, 10, 12, 13, 14, 15, 17, 18, 20, 21, 23, 26, 27, 28, 29, 30, 31, 32, 33, 34, 36, 37, 39, 40, 41, 42, 43, 44, 47, 49, 50, 52, 53, 54, 55, 56, 57, 58, 60, 63, 64, 65, 66, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 95, 96, 97, 98, 100, 101, 102, 104, 105, 107, 108, 109, 110, 111, 112, 113, 115, 116, 118, 119, 120, 121, 122, 123, 125, 126, 127, 128, 131, 134, 135, 136, 138, 139, 140, 141, 142, 143, 145, 146, 147, 148, 150, 152, 154, 155, 156, 157, 158, 159, 160, 163, 164, 165, 166, 170, 171, 172, 173, 175, 176, 177, 179, 180, 181, 185, 186, 187, 188, 189, 190, 193, 195, 196, 198, 199, 200, 201, 202, 203, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 219, 220, 222, 223, 224, 225, 226, 227, 229, 230, 231, 232, 233, 234, 235, 236, 237, 239, 240, 241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 254, 255, 256, 257, 258, 260, 261, 262, 264, 265, 266, 268, 269, 271, 273, 274, 275, 276, 277, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 291, 292, 297, 298, 300, 301, 302, 303, 304, 305, 308, 309, 310, 311, 312, 313, 314, 316, 317, 318, 321, 322, 323, 324, 326, 327, 328, 329, 331, 332, 335, 338, 340, 341, 343, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 358, 361, 362, 364, 365, 367, 368, 369, 370, 371, 372, 373, 374, 377, 378, 380, 382, 383, 384, 387, 388, 389, 391, 392, 393, 394, 395, 396, 398, 399, 400, 401, 403, 404, 406, 409, 410, 411, 413, 415, 417, 418, 420, 421, 424, 425, 429, 430, 431, 432, 433, 434, 435, 437, 438, 439, 440, 442, 443, 444, 445, 446, 447, 448, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 461, 462, 463, 464, 466, 468, 471, 472, 474, 475, 477, 478, 479, 480, 481, 482, 484, 485, 486, 487, 489, 490, 491, 492, 493, 494, 497, 498, 499, 502, 503, 505, 506, 507, 508, 509, 511, 512, 513, 516, 517, 521, 523, 525, 526, 527, 528, 530, 532, 537, 538, 539, 541, 542, 543, 544, 545, 549, 551, 552, 553, 554, 556, 557, 558, 559, 560, 562, 564, 567, 568, 570, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 589, 590, 591, 592, 593, 594, 595, 596, 597, 598, 601, 602, 603, 604, 606, 607, 608, 609, 610, 612, 613, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 629, 630, 631, 632, 634, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 647, 649, 650, 651, 654, 655, 656, 657, 658, 659, 661, 662, 664, 668, 669, 670, 671, 673, 675, 676, 677, 678, 680, 681, 682, 683, 684, 686, 687, 688, 689, 691, 692, 693, 694, 698, 700, 702, 703, 704, 705, 706, 708, 709, 710, 714, 715, 717, 718, 719, 720, 721, 722, 724, 725, 730, 731, 732, 733, 734, 735, 736, 737, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753, 756, 757, 758, 762, 763, 765, 767, 768, 770, 771, 772, 774, 776, 777, 778, 779, 781, 783, 784, 786, 787, 789, 790, 792, 793, 796, 797, 800, 801, 802, 803, 805, 809, 811};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  0,0,0b11101},   {  1,1,0b11111},   {  2,0,0b10101},   {  3,1,0b11011},   {  5,0,0b01001},   {  6,0,0b01011},   {  7,1,0b10011},   {  8,0,0b00101},   {  9,1,0b10001},   { 10,0,0b01101},   { 11,1,0b11001},   { 12,0,0b00001},   { 13,1,0b11111},   { 14,1,0b10001},   { 15,1,0b00001},   { 16,0,0b01001},   { 17,1,0b11111},   { 18,0,0b01101},   { 19,1,0b01111},   { 20,0,0b01111},   { 21,0,0b10111},   { 22,1,0b11111},   { 23,1,0b01111},   { 24,0,0b00001},   { 25,0,0b01001},   { 26,1,0b11001},   { 27,1,0b10001},   { 28,1,0b10111},   { 29,1,0b01011},   { 30,0,0b00011},   { 31,1,0b11001},   { 34,0,0b01001},   { 35,0,0b01111},   { 36,0,0b10101},   { 37,1,0b11001},   { 38,0,0b00011},   { 39,0,0b00101},   { 40,0,0b01101},   { 41,0,0b10011},   { 42,1,0b11011},   { 43,1,0b11001},   { 44,1,0b00011},   { 45,1,0b01111},   { 46,1,0b11011},   { 47,1,0b11111},   { 48,1,0b10011},   { 49,1,0b11111},   { 50,1,0b00001},   { 51,1,0b10001},   { 52,0,0b01101},   { 53,0,0b01111},   { 54,1,0b10101},   { 55,1,0b10101},   { 56,0,0b10111},   { 57,1,0b11101},   { 58,1,0b10101},   { 59,0,0b00001},   { 60,1,0b10001},   { 61,1,0b01111},   { 62,0,0b01101},   { 63,0,0b10111},   { 64,1,0b11001},   { 65,0,0b01001},   { 66,0,0b01011},   { 67,1,0b01111},   { 68,1,0b00111},   { 69,1,0b00111},   { 70,0,0b00111},   { 71,1,0b01101},   { 72,1,0b11101},   { 73,1,0b10101},   { 74,0,0b10001},   { 75,1,0b11111},   { 76,1,0b11111},   { 77,0,0b01001},   { 78,0,0b10111},   { 79,1,0b11001},   { 80,0,0b00111},   { 81,1,0b11111},   { 83,0,0b01011},   { 84,1,0b10101},   { 85,0,0b00001},   { 86,0,0b01101},   { 87,0,0b10011},   { 88,1,0b10101},   { 89,1,0b01011},   { 90,0,0b01101},   { 91,1,0b01111},   { 92,1,0b00101},   { 93,1,0b11101},   { 94,0,0b00111},   { 95,1,0b01001},   { 96,0,0b00101},   { 97,1,0b10001},   { 98,0,0b10001},   { 99,0,0b10001},   {100,1,0b11001},   {101,1,0b01101},   {102,1,0b01001},   {103,0,0b00101},   {104,0,0b00111},   {105,0,0b00111},   {106,1,0b10011},   {107,1,0b00001},   {108,0,0b01101},   {109,1,0b10101},   {110,0,0b10101},   {111,1,0b11001},   {112,0,0b01111},   {113,1,0b01111},   {114,1,0b00101},   {115,0,0b10001},   {116,0,0b10101},   {117,0,0b10111},   {118,1,0b11011},   {119,1,0b10111},   {120,1,0b10101},   {121,0,0b01111},   {122,0,0b01111},   {123,0,0b10101},   {124,1,0b10101},   {125,0,0b00111},   {126,0,0b01011},   {127,1,0b10001},   {128,0,0b01011},   {129,1,0b11101},   {130,0,0b01001},   {131,1,0b10111},   {132,0,0b10111},   {133,1,0b11001},   {134,0,0b01111},   {135,1,0b11001},   {136,0,0b00101},   {137,1,0b11101},   {138,1,0b00011},   {139,1,0b11011},   {140,1,0b00011},   {141,0,0b10001},   {142,1,0b10111},   {143,0,0b00001},   {144,0,0b01101},   {145,0,0b10001},   {146,1,0b11011},   {147,0,0b00101},   {148,0,0b10111},   {149,1,0b11011},   {150,0,0b01111},   {151,1,0b11001},   {152,0,0b10011},   {153,1,0b10101},   {154,1,0b10111},   {155,0,0b01011},   {156,1,0b01111},   {157,0,0b00111},   {158,0,0b10001},   {159,1,0b11011},   {160,1,0b00001},   {161,0,0b11111},   {162,1,0b11111},   {163,0,0b01001},   {164,0,0b10001},   {165,1,0b10011},   {166,1,0b01111},   {167,0,0b00111},   {168,0,0b01111},   {169,1,0b10101},   {170,0,0b01101},   {171,1,0b11001},   {172,1,0b11011},   {173,1,0b01011},   {174,1,0b10101},   {175,1,0b01101},   {176,1,0b01101},   {177,0,0b00111},   {178,1,0b11001},   {179,0,0b01011},   {180,1,0b11011},   {181,0,0b00011},   {182,1,0b01001},   {183,0,0b01101},   {184,0,0b11001},   {185,1,0b11101},   {186,1,0b10011},   {187,0,0b00011},   {188,1,0b00011},   {189,0,0b00011},   {190,0,0b00101},   {191,1,0b10101},   {192,1,0b00001},   {193,1,0b10001},   {194,1,0b11011},   {195,0,0b10001},   {196,0,0b10111},   {197,0,0b11101},   {198,1,0b11101},   {199,1,0b00101},   {200,0,0b01101},   {201,1,0b11111},   {202,1,0b01101},   {203,1,0b00101},   {204,1,0b01001},   {205,1,0b10001},   {206,0,0b01001},   {207,1,0b01011},   {208,0,0b10111},   {209,1,0b10111},   {210,1,0b01101},   {211,0,0b01011},   {212,1,0b10101},   {213,0,0b01011},   {214,1,0b10001},   {215,0,0b00001},   {216,0,0b11011},   {217,0,0b11011},   {218,1,0b11011},   {219,1,0b11101},   {220,0,0b00001},   {221,1,0b00101},   {222,0,0b00011},   {223,1,0b01001},   {224,1,0b10101},   {225,1,0b11001},   {226,1,0b11101},   {227,1,0b00001},   {228,1,0b01001},   {229,0,0b00111},   {230,0,0b01001},   {231,1,0b11011},   {232,1,0b01111},   {233,0,0b01001},   {234,0,0b01011},   {235,1,0b01111},   {236,0,0b00111},   {237,0,0b00111},   {238,1,0b11011},   {239,1,0b10101},   {240,0,0b00001},   {241,0,0b10001},   {242,1,0b11011},   {243,0,0b00101},   {244,1,0b10011},   {245,0,0b00111},   {246,1,0b10001},   {247,1,0b11001},   {248,1,0b11001},   {249,1,0b00111},   {250,1,0b10111},   {251,0,0b00001},   {252,1,0b10101},   {253,1,0b10011},   {254,1,0b00101},   {255,1,0b10101},   {256,1,0b01111},   {257,0,0b01001},   {258,1,0b10001},   {259,0,0b00011},   {260,0,0b10001},   {261,1,0b11111},   {262,0,0b00011},   {263,0,0b01011},   {264,1,0b10011},   {265,1,0b00011},   {266,0,0b01101},   {267,1,0b11001},   {268,0,0b00101},   {269,0,0b10111},   {270,1,0b11011},   {271,1,0b11101},   {272,1,0b10001},   {273,0,0b00001},   {274,1,0b11101},   {275,0,0b10111},   {276,1,0b11111},   {277,0,0b01101},   {278,0,0b10111},   {279,1,0b11111},   {280,0,0b00111},   {281,1,0b11101},   {282,0,0b01011},   {283,1,0b10101},   {284,0,0b01101},   {285,1,0b10001},   {286,0,0b00101},   {287,0,0b10001},   {288,0,0b11001},   {289,1,0b11101},   {290,1,0b00111},   {291,0,0b01011},   {292,1,0b11001},   {293,0,0b01011},   {294,1,0b11001},   {295,0,0b00001},   {296,0,0b00011},   {297,0,0b00101},   {298,1,0b11011},   {299,0,0b00011},   {300,0,0b00111},   {301,1,0b10001},   {302,1,0b11011},   {303,0,0b11001},   {304,1,0b11111},   {305,1,0b11101},   {306,0,0b11001},   {307,1,0b11111},   {308,0,0b01001},   {309,1,0b01101},   {310,1,0b11011},   {311,1,0b10101},   {312,0,0b00101},   {313,0,0b01101},   {314,1,0b10111},   {315,0,0b00111},   {316,0,0b11011},   {317,1,0b11011},   {318,1,0b00011},   {319,0,0b01111},   {320,0,0b11001},   {321,1,0b11111},   {322,1,0b10011},   {323,1,0b01101},   {324,1,0b11101},   {325,1,0b00001},   {326,1,0b01101},   {327,0,0b01001},   {328,1,0b11011},   {329,0,0b00111},   {330,1,0b10001},   {331,1,0b10011},   {332,0,0b01111},   {333,0,0b10101},   {334,1,0b11101},   {335,1,0b11001},   {336,0,0b01101},   {337,1,0b10111},   {338,1,0b00001},   {339,1,0b00011},   {340,1,0b01111},   {341,1,0b11011},   {342,0,0b10101},   {343,1,0b10111},   {344,1,0b10101},   {345,1,0b11011},   {346,0,0b01011},   {347,1,0b11001},   {348,1,0b01001},   {349,1,0b01101},   {350,0,0b11001},   {351,1,0b11111},   {352,1,0b01001},   {353,1,0b11011},   {354,1,0b00101},   {355,1,0b00111},   {356,0,0b01111},   {357,1,0b11001},   {358,0,0b10011},   {359,1,0b11011},   {360,0,0b01001},   {361,1,0b10001},   {362,1,0b10101},   {363,0,0b01101},   {364,1,0b11101},   {365,1,0b01111},   {366,1,0b01101},   {367,0,0b00111},   {368,0,0b01101},   {369,1,0b11001},   {370,0,0b10011},   {371,1,0b11101},   {372,0,0b01001},   {373,0,0b01011},   {374,1,0b01111},   {375,0,0b10101},   {376,1,0b11101},   {377,0,0b00101},   {378,0,0b01011},   {379,1,0b11001},   {380,1,0b10001},   {381,0,0b00101},   {382,1,0b11101},   {383,0,0b01111},   {384,1,0b11111},   {385,0,0b10111},   {386,1,0b11111},   {387,1,0b01101},   {388,0,0b10011},   {389,1,0b10101},   {390,0,0b00001},   {391,0,0b00011},   {392,1,0b10011},   {393,0,0b01001},   {394,0,0b10011},   {395,1,0b11001},   {396,0,0b01011},   {397,1,0b01111},   {398,0,0b00111},   {399,1,0b11011},   {400,1,0b10111},   {401,0,0b01111},   {402,1,0b01111},   {403,1,0b00101},   {404,1,0b11111},   {405,1,0b11101},   {406,1,0b10101},   {407,1,0b01101},   {408,0,0b01111},   {409,0,0b10001},   {410,1,0b10001},   {411,0,0b10101},   {412,1,0b11111},   {413,0,0b01111},   {414,1,0b11101},   {415,0,0b00111},   {416,0,0b01001},   {417,1,0b10001},   {418,0,0b10011},   {419,1,0b11101},   {420,0,0b00001},   {421,1,0b01111},   {422,0,0b11001},   {423,1,0b11101},   {424,1,0b01101},   {425,1,0b11111},   {426,1,0b01101},   {427,0,0b00011},   {428,0,0b00101},   {429,0,0b11101},   {430,1,0b11111},   {431,0,0b00011},   {432,0,0b01001},   {433,1,0b11011},   {434,0,0b01101},   {435,1,0b11111},   {436,1,0b11001},   {437,0,0b01111},   {438,1,0b11101},   {439,1,0b11101},   {440,1,0b00101},   {441,0,0b01011},   {442,1,0b11011},   {443,0,0b00011},   {444,1,0b00101},   {445,1,0b11011},   {446,0,0b11111},   {447,1,0b11111},   {448,1,0b10101},   {449,0,0b00011},   {450,0,0b01111},   {451,0,0b10001},   {452,1,0b11011},   {453,0,0b01101},   {454,0,0b10101},   {455,1,0b11111},   {456,1,0b10111},   {457,1,0b01111},   {458,0,0b01101},   {459,1,0b10101},   {460,0,0b10101},   {461,1,0b11101},   {462,1,0b00111},   {463,1,0b11101},   {464,1,0b01101},   {465,0,0b00001},   {466,0,0b10001},   {467,1,0b10101},   {468,1,0b10001},   {469,0,0b00011},   {470,1,0b01101},   {471,1,0b00001},   {472,0,0b00001},   {473,1,0b01101},   {474,1,0b10011},   {475,1,0b11101},   {476,0,0b00001},   {477,1,0b11011},   {478,1,0b00001},   {479,1,0b01101},   {480,1,0b10011},   {481,1,0b00111},   {482,1,0b01111},   {483,1,0b01001},   {484,0,0b00111},   {485,1,0b11101},   {486,0,0b01011},   {487,1,0b10101},   {488,1,0b00111},   {489,0,0b00111},   {490,1,0b01001},   {491,1,0b11101},   {492,0,0b10001},   {493,1,0b11101},   {494,1,0b01001},   {495,1,0b10001},   {496,1,0b10101},   {497,0,0b00011},   {498,1,0b00111},   {500,0,0b00011},   {501,1,0b01101},   {502,1,0b10001},   {504,0,0b01111},   {505,1,0b10111},   {507,1,0b00011},   {508,1,0b10001},   {509,1,0b10101},   {513,0,0b01101},   {514,1,0b10001},   {515,0,0b00101},   {516,0,0b00111},   {517,1,0b11101},   {518,1,0b11111},   {519,0,0b00011},   {520,1,0b00011},   {521,0,0b00101},   {522,1,0b01111},   {523,0,0b00001},   {524,1,0b10111},   {525,0,0b00001},   {526,1,0b00101},   {527,1,0b01001},   {528,0,0b00101},   {529,1,0b01111},   {530,1,0b10111},   {531,1,0b00011},   {532,0,0b10001},   {533,1,0b11101},   {534,1,0b01001},   {535,0,0b00111},   {536,0,0b10111},   {537,1,0b11111},   {538,1,0b11001},   {539,0,0b00001},   {540,1,0b01101},   {542,1,0b11001},   {544,1,0b00011},   {545,0,0b00111},   {546,1,0b10111},   {548,0,0b00101},   {549,0,0b10101},   {550,1,0b11011},   {551,1,0b01101},   {553,0,0b01011},   {554,0,0b10001},   {555,1,0b11011},   {556,0,0b01111},   {557,1,0b11111},   {558,0,0b00011},   {559,0,0b01101},   {560,0,0b10011},   {561,1,0b10011},   {562,0,0b00001},   {563,1,0b11111},   {564,1,0b11001},   {565,1,0b01001},   {569,1,0b10101},   {570,0,0b10011},   {571,1,0b11001},   {572,1,0b10011},   {573,1,0b01011},   {574,1,0b01111},   {576,0,0b00011},   {577,0,0b01101},   {578,1,0b11111},   {579,0,0b00111},   {580,1,0b01001},   {581,1,0b01001},   {582,1,0b11001},   {583,1,0b10001},   {584,1,0b10011},   {586,0,0b01001},   {587,1,0b10101},   {588,0,0b10111},   {589,1,0b11111},   {590,1,0b10111},   {591,0,0b00001},   {592,0,0b00001},   {593,1,0b01011},   {594,1,0b01001},   {595,0,0b00011},   {596,1,0b10001},   {597,0,0b11101},   {598,1,0b11111},   {599,1,0b11111},   {600,1,0b11101},   {601,1,0b10111},   {602,1,0b01011},   {603,0,0b11101},   {604,1,0b11111},   {605,1,0b01011},   {606,1,0b11011},   {607,1,0b01011},   {608,0,0b10111},   {609,1,0b11011},   {610,0,0b01101},   {611,1,0b01111},   {612,1,0b01001},   {613,0,0b01111},   {614,1,0b10111},   {615,0,0b10011},   {616,0,0b10111},   {617,1,0b11001},   {618,1,0b01111},   {619,0,0b10011},   {620,1,0b10011},   {621,0,0b00101},   {622,1,0b01111},   {625,1,0b00011},   {626,0,0b00001},   {627,1,0b11011},   {629,1,0b11111},   {630,1,0b00101},   {633,0,0b01011},   {634,1,0b01101},   {635,0,0b01111},   {636,1,0b10101},   {637,1,0b10111},   {638,0,0b10101},   {639,0,0b11011},   {640,1,0b11011},   {641,1,0b00111},   {642,0,0b01111},   {643,1,0b01111},   {644,0,0b00011},   {645,0,0b00011},   {646,0,0b01111},   {647,0,0b10011},   {648,1,0b11011},   {649,1,0b01111},   {650,1,0b01111},   {651,1,0b00111},   {652,0,0b00001},   {653,1,0b00011},   {654,1,0b10011},   {655,0,0b11011},   {656,1,0b11101},   {657,0,0b00001},   {658,1,0b00011},   {659,1,0b00101},   {660,1,0b01011},   {661,1,0b00101},   {662,0,0b00011},   {663,1,0b01001},   {664,0,0b01101},   {665,1,0b11011},   {666,0,0b10001},   {667,1,0b10011},   {668,1,0b11101},   {670,1,0b11111},   {671,1,0b11001},   {673,0,0b10001},   {674,1,0b11011},   {675,0,0b00111},   {676,1,0b11001},   {677,0,0b00101},   {678,0,0b00101},   {679,1,0b01101},   {680,1,0b00011},   {681,1,0b10001},   {682,0,0b01111},   {683,1,0b01111},   {684,1,0b00001},   {686,1,0b10001},   {691,0,0b10001},   {692,1,0b11001},   {694,1,0b11001},   {696,0,0b10001},   {697,0,0b10001},   {698,1,0b10111},   {699,1,0b11011},   {700,1,0b11111},   {701,1,0b11111},   {703,1,0b00001},   {706,0,0b00111},   {707,0,0b01111},   {708,1,0b10111},   {712,0,0b00101},   {713,1,0b01101},   {714,1,0b01101},   {715,1,0b00011},   {718,0,0b10011},   {719,0,0b11011},   {720,1,0b11111},   {721,1,0b00001},   {722,0,0b00001},   {723,0,0b10011},   {724,1,0b11011},   {725,1,0b01001},   {726,0,0b00001},   {727,1,0b00011},   {728,1,0b10111},   {731,1,0b00101},   {732,1,0b11101},   {734,1,0b01111},   {735,0,0b10011},   {736,1,0b10111},   {738,0,0b00101},   {739,0,0b01101},   {740,1,0b10011},   {741,0,0b01101},   {742,1,0b01111},   {743,0,0b00101},   {744,1,0b01111},   {745,0,0b10101},   {746,1,0b10111},   {747,0,0b00111},   {748,1,0b10011},   {749,0,0b00011},   {750,1,0b10001},   {751,0,0b00011},   {752,1,0b10001},   {753,1,0b01111},   {754,1,0b11001},   {756,1,0b01101},   {760,0,0b00001},   {761,0,0b01001},   {762,0,0b11111},   {763,1,0b11111},   {764,0,0b01101},   {765,1,0b10101},   {766,1,0b01011},   {767,0,0b00111},   {768,0,0b01101},   {769,1,0b11011},   {770,1,0b00001},   {771,0,0b01011},   {772,0,0b01111},   {773,1,0b11001},   {774,0,0b00111},   {775,0,0b01011},   {776,0,0b11011},   {777,1,0b11011},   {778,0,0b01011},   {779,1,0b01011},   {780,0,0b01101},   {781,0,0b10001},   {782,1,0b11001},   {783,1,0b00101},   {784,1,0b10011},   {785,0,0b01101},   {786,1,0b11111},   {787,0,0b00111},   {788,0,0b01011},   {789,1,0b11001},   {790,1,0b11111},   {791,1,0b10111},   {792,0,0b00011},   {793,1,0b11011},   {794,0,0b11001},   {795,1,0b11011},   {796,0,0b00101},   {797,0,0b00101},   {798,1,0b11111},   {799,0,0b01011},   {800,1,0b10111},   {801,1,0b01101},   {802,1,0b01001},   {803,0,0b01001},   {804,1,0b10011},   {805,1,0b11001},   {806,0,0b10111},   {807,1,0b11011},   {808,0,0b10101},   {809,1,0b11101},   {810,1,0b11101},   {811,0,0b00111},   {812,0,0b01011},   {813,1,0b10001},   {814,0,0b00001},   {815,0,0b00011},   {816,1,0b00011},   {817,1,0b10111},   {818,1,0b00111},   {819,0,0b01001},   {820,1,0b01011},   {821,0,0b00011},   {822,1,0b10011},   {823,0,0b01001},   {824,0,0b11001},   {825,1,0b11001},   {826,1,0b10111},   {827,1,0b01011},   {828,0,0b00011},   {829,1,0b10001},   {830,0,0b00101},   {831,1,0b11001},   {832,1,0b10011},   {833,1,0b10111},   {834,0,0b00001},   {835,0,0b01001},   {836,1,0b11011},   {837,0,0b00101},   {838,0,0b01011},   {839,1,0b10001},   {840,0,0b01111},   {841,1,0b11001},   {842,1,0b00101},   {843,1,0b10011},   {844,1,0b11001},   {845,1,0b10011},   {846,1,0b10011},   {847,1,0b01111},   {848,1,0b01101},   {849,1,0b10101},   {850,0,0b00011},   {851,0,0b00111},   {852,1,0b10001},   {853,1,0b00101},   {854,0,0b00001},   {855,0,0b01101},   {856,1,0b10111},   {857,1,0b11001},   {858,0,0b11001},   {859,1,0b11011},   {860,1,0b10111},   {861,1,0b10011},   {862,1,0b00001},   {863,0,0b00111},   {864,1,0b11001},   {865,0,0b00011},   {866,1,0b10111},   {867,0,0b01101},   {868,1,0b10001},   {869,1,0b01001},   {870,0,0b10111},   {871,1,0b10111},   {872,1,0b10001},   {873,1,0b00011},   {874,1,0b01011},   {875,1,0b01101},   {876,0,0b00011},   {877,1,0b11111},   {878,1,0b11101},   {879,1,0b10111},   {880,1,0b00101},   {881,0,0b01001},   {882,1,0b11101},   {883,1,0b00111},   {884,0,0b10011},   {885,1,0b11101},   {886,0,0b00111},   {887,1,0b10101},   {888,0,0b00011},   {889,1,0b01111},   {890,0,0b01001},   {891,1,0b01111},   {892,0,0b00011},   {893,1,0b10011},   {894,0,0b01011},   {895,1,0b11001},   {896,0,0b00001},   {897,1,0b00101},   {898,1,0b01011},   {899,0,0b00011},   {900,0,0b00011},   {901,1,0b01111},   {902,0,0b01011},   {903,1,0b01101},   {904,0,0b00001},   {905,1,0b10011},   {906,0,0b00001},   {907,0,0b00001},   {908,0,0b00111},   {909,0,0b10011},   {910,0,0b11001},   {911,1,0b11001},   {912,1,0b10001},   {913,1,0b10111},   {914,0,0b00001},   {915,0,0b00011},   {916,1,0b11011},   {917,1,0b11101},   {918,0,0b00011},   {919,0,0b10001},   {920,1,0b10111},   {921,0,0b01001},   {922,0,0b10011},   {923,1,0b10111},   {924,0,0b00111},   {925,1,0b11101},   {926,1,0b00111},   {927,0,0b00001},   {928,1,0b00011},   {929,0,0b00011},   {930,1,0b11011},   {931,0,0b00001},   {932,0,0b01011},   {933,1,0b01011},   {934,0,0b00001},   {935,0,0b00111},   {936,0,0b01011},   {937,0,0b11101},   {938,1,0b11111},   {939,1,0b11101},   {940,0,0b00101},   {941,1,0b00111},   {942,1,0b00111},   {943,0,0b11001},   {944,1,0b11111},   {945,0,0b00111},   {946,1,0b11101},   {947,1,0b10101},   {948,1,0b11011},   {949,0,0b00101},   {950,1,0b11011},   {951,1,0b01001},   {952,1,0b00001},   {953,1,0b11011},   {954,1,0b11101},   {955,0,0b00001},   {956,1,0b00001},   {957,0,0b00101},   {958,0,0b00101},   {959,1,0b01111},   {960,1,0b00111},   {961,1,0b11101},   {962,0,0b11011},   {963,1,0b11111},   {964,0,0b00001},   {965,1,0b01101},   {966,0,0b00101},   {967,0,0b00111},   {968,1,0b10101},   {969,1,0b11001},   {970,0,0b00001},   {971,1,0b01011},   {972,0,0b00011},   {973,0,0b01011},   {974,1,0b10001},   {975,0,0b01111},   {976,0,0b10011},   {977,1,0b11011},   {978,0,0b01101},   {979,1,0b11101},   {980,0,0b10001},   {981,1,0b11101},   {982,0,0b01111},   {983,1,0b01111},   {984,0,0b01001},   {985,0,0b01001},   {986,0,0b01101},   {987,1,0b11111},   {988,1,0b10111},   {989,1,0b00111},   {990,0,0b00101},   {991,1,0b01101},   {992,0,0b01001},   {993,0,0b10101},   {994,1,0b11001},   {995,1,0b00101},   {996,1,0b00101},   {997,0,0b01011},   {998,0,0b01011},   {999,1,0b01011},   {1000,1,0b10111},   {1001,0,0b01001},   {1002,1,0b01111},   {1003,0,0b00001},   {1004,0,0b01001},   {1005,0,0b01001},   {1006,0,0b01011},   {1007,1,0b11011},   {1008,1,0b10101},   {1009,1,0b11111},   {1010,0,0b01001},   {1011,1,0b01101},   {1012,0,0b00001},   {1013,1,0b11101},   {1014,1,0b00001},   {1015,1,0b10111},   {1016,1,0b00001},   {1017,1,0b11011},   {1018,1,0b10011},   {1019,0,0b00001},   {1020,1,0b11101},   {1021,0,0b01001},   {1022,1,0b10101},   {1023,0,0b11001},   {1024,1,0b11101},   {1025,0,0b00001},   {1026,1,0b01011},   {1027,0,0b01111},   {1028,1,0b10101},   {1029,1,0b01001},   {1030,1,0b01101},   {1031,0,0b01111},   {1032,0,0b11011},   {1033,0,0b11101},   {1034,1,0b11111},   {1035,0,0b00001},   {1036,0,0b00011},   {1037,1,0b01111},   {1038,0,0b11001},   {1039,1,0b11111},   {1040,1,0b00111},   {1041,0,0b00001},   {1042,1,0b00011},   {1043,0,0b01011},   {1044,0,0b10011},   {1045,1,0b11101},   {1046,1,0b11011},   {1047,1,0b01001},   {1048,1,0b00101},   {1049,1,0b11001},   {1050,1,0b01011},   {1051,1,0b01111},   {1052,1,0b10101},   {1056,1,0b11011},   {1057,0,0b01011},   {1058,0,0b01101},   {1059,1,0b11011},   {1061,0,0b00111},   {1062,0,0b11011},   {1063,1,0b11101},   {1064,0,0b00101},   {1065,1,0b00101},   {1066,0,0b00101},   {1067,1,0b11001},   {1068,0,0b00011},   {1069,1,0b01101},   {1070,0,0b01101},   {1071,1,0b11011},   {1073,0,0b00101},   {1074,0,0b10001},   {1075,1,0b11001},   {1076,1,0b00011}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
                    wh_iter_peek_ref(it, reinterpret_cast<const void **>(&res_key), &res_size,
                                     reinterpret_cast<void **>(&store), &dummy);

                    REQUIRE_EQ(memcmp(expected_boundary, res_key, expected_boundary_length), 0);
                    REQUIRE_FALSE(store->IsPartialKey());
                    REQUIRE_EQ(store->GetInvalidBits(), 0);
                    AssertStoreContents(s, *store, occupieds_pos, checks);
                }

                wh_iter_skip1(it);
                {
                    const size_t expected_boundary_length = 6;
                    const uint8_t expected_boundary[expected_boundary_length] = {0b11000111, 0b10011111, 0b10110001, 0b01100101, 0b10001101, 0b10001110};
                    const std::vector<uint32_t> occupieds_pos = {1, 2, 5, 9, 11, 13, 14, 16, 17, 21, 23, 28, 30, 31, 32, 33, 34, 37, 39, 41, 45, 47, 50, 52, 54, 58, 65, 66, 69, 72, 75, 76, 79, 86, 90, 97, 104, 105, 108, 110, 111, 113, 123, 126, 128, 129, 130, 131, 133, 134, 137, 138, 139, 140, 141, 142, 144, 146, 147, 148, 149, 150, 151, 152, 153, 154, 158, 160, 161, 163, 165, 172, 173, 174, 176, 181, 182, 183, 184, 186, 187, 190, 192, 193, 194, 195, 196, 198, 201, 207, 213, 214, 216, 217, 218, 220, 222, 223, 225, 226, 227, 232, 234, 237, 238, 240, 243, 244, 246, 248, 249, 250, 251, 252, 253, 256, 257, 258, 260, 261, 262, 266, 268, 274, 275, 276, 277, 279, 280, 281, 283, 285, 288, 289, 290, 291, 294, 295, 297, 298, 300, 301, 303, 305, 306, 313, 316, 318, 319, 325, 326, 327, 331, 333, 335, 336, 337, 338, 339, 343, 344, 345, 346, 351, 352, 361, 365, 366, 367, 369, 370, 374, 375, 382, 384, 387, 388, 389, 391, 392, 393, 394, 399, 401, 402, 406, 410, 411, 413, 414, 415, 423, 425, 426, 430, 431, 439, 442, 445, 446, 448, 450, 451, 452, 453, 454, 456, 462, 463, 466, 467, 469, 470, 472, 473, 474, 475, 476, 477, 479, 480, 482, 483, 486, 487, 493, 494, 495, 497, 498, 501, 504, 505, 506, 508, 510, 516, 519, 521, 525, 526, 532, 536, 539, 542, 545, 547, 551, 552, 553, 557, 558, 559, 564, 567, 572, 575, 578, 579, 580, 581, 583, 584, 586, 587, 589, 590, 593, 595, 596, 597, 599, 602, 607, 608, 609, 611, 612, 613, 614, 619, 620, 621, 624, 626, 629, 632, 638, 639, 641, 642, 644, 649, 651, 652, 653, 654, 657, 658, 660, 661, 664, 666, 668, 670, 671, 673, 674, 676, 678, 683, 684, 688, 690, 691, 693, 695, 700, 701, 702, 703, 705, 706, 707, 712, 713, 714, 716, 717, 718, 719, 720, 721, 722, 723, 724, 725, 726, 728, 731, 732, 733, 738, 740, 743, 744, 749, 750, 751, 757, 760, 761, 763, 764, 766, 767, 769, 770, 773, 775, 780, 781, 783, 784, 785, 786, 787, 788, 794, 796, 799, 800, 802, 803, 805, 806, 807, 813, 814, 817, 818, 820, 823, 825, 826, 831, 834, 840, 841, 843, 846, 849, 851, 855, 858, 861, 862, 864, 867, 868, 874, 875, 876, 880, 887, 891, 894, 895, 896, 899};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {{  1,1,0b00101},   {  2,0,0b01111},   {  3,0,0b10101},   {  4,1,0b11011},   {  5,1,0b00101},   { 10,1,0b01001},   { 13,1,0b10111},   { 15,1,0b00101},   { 16,1,0b10111},   { 19,1,0b10001},   { 20,1,0b10001},   { 25,0,0b00011},   { 26,0,0b00101},   { 27,0,0b11011},   { 28,1,0b11101},   { 29,0,0b00011},   { 30,1,0b11001},   { 33,0,0b10101},   { 34,1,0b11101},   { 35,1,0b11111},   { 37,1,0b11011},   { 38,1,0b11011},   { 39,0,0b00001},   { 40,1,0b10011},   { 41,1,0b01111},   { 44,1,0b11101},   { 46,1,0b00101},   { 48,1,0b10101},   { 53,0,0b11001},   { 54,1,0b11011},   { 56,1,0b11111},   { 59,0,0b01011},   { 60,1,0b01111},   { 62,0,0b00001},   { 63,1,0b01001},   { 64,1,0b00111},   { 69,0,0b10001},   { 70,1,0b10001},   { 77,1,0b11001},   { 78,0,0b10011},   { 79,1,0b11001},   { 82,0,0b00101},   { 83,1,0b11011},   { 85,1,0b01101},   { 89,1,0b11001},   { 90,1,0b10111},   { 94,1,0b10011},   {102,1,0b10111},   {107,1,0b00111},   {115,1,0b01111},   {124,1,0b01011},   {125,1,0b10001},   {128,1,0b01111},   {131,1,0b11111},   {132,1,0b10011},   {134,1,0b01101},   {146,1,0b11101},   {150,1,0b00011},   {152,1,0b11011},   {153,1,0b11011},   {155,1,0b00001},   {156,1,0b11011},   {158,1,0b11011},   {159,1,0b01101},   {163,1,0b00001},   {164,1,0b10001},   {165,1,0b00001},   {167,1,0b01001},   {168,1,0b00111},   {169,1,0b11111},   {171,1,0b00011},   {174,0,0b01011},   {175,1,0b10011},   {176,1,0b10111},   {177,1,0b10011},   {178,1,0b01111},   {179,1,0b00001},   {180,1,0b01111},   {181,1,0b00101},   {182,1,0b10111},   {183,1,0b10011},   {188,0,0b10001},   {189,1,0b10111},   {190,0,0b10111},   {191,1,0b11001},   {192,1,0b10001},   {194,0,0b11011},   {195,1,0b11111},   {196,0,0b00011},   {197,1,0b10111},   {205,1,0b10101},   {206,1,0b10001},   {207,1,0b10011},   {210,1,0b01001},   {216,1,0b10001},   {217,1,0b11101},   {218,1,0b10011},   {219,0,0b01011},   {220,1,0b10111},   {222,1,0b00001},   {223,1,0b01101},   {226,1,0b10101},   {229,1,0b11001},   {230,1,0b00111},   {231,1,0b11001},   {232,1,0b10111},   {233,1,0b00011},   {236,1,0b01011},   {239,1,0b10001},   {247,0,0b01101},   {248,0,0b10001},   {249,1,0b11101},   {254,1,0b01011},   {255,1,0b11101},   {257,0,0b01111},   {258,1,0b10001},   {259,0,0b00001},   {260,1,0b00011},   {261,1,0b10011},   {262,1,0b01001},   {264,1,0b10111},   {266,1,0b00101},   {268,1,0b00001},   {269,0,0b01011},   {270,1,0b10111},   {271,1,0b10111},   {276,1,0b00111},   {279,1,0b11111},   {282,1,0b11111},   {284,0,0b01001},   {285,1,0b11011},   {286,1,0b01101},   {290,1,0b00101},   {291,1,0b10111},   {293,1,0b00101},   {296,1,0b01101},   {297,1,0b10101},   {298,1,0b11011},   {299,0,0b01011},   {300,0,0b01101},   {301,1,0b01101},   {302,0,0b01011},   {303,1,0b11011},   {304,0,0b00011},   {305,1,0b10101},   {306,1,0b11101},   {307,0,0b01001},   {308,1,0b01101},   {309,0,0b01111},   {310,1,0b10111},   {311,0,0b10101},   {312,0,0b11011},   {313,1,0b11101},   {314,1,0b00111},   {315,0,0b00101},   {316,1,0b11111},   {317,1,0b10111},   {319,1,0b10111},   {327,1,0b10111},   {328,1,0b11111},   {329,0,0b00011},   {330,1,0b11101},   {331,0,0b10001},   {332,0,0b10111},   {333,1,0b11001},   {334,0,0b10111},   {335,1,0b11111},   {336,1,0b11011},   {337,1,0b00011},   {338,0,0b00011},   {339,0,0b00111},   {340,1,0b10101},   {341,0,0b10011},   {342,1,0b10101},   {343,1,0b01001},   {344,1,0b10011},   {346,0,0b01001},   {347,1,0b11111},   {348,1,0b11011},   {350,1,0b00111},   {352,1,0b10001},   {354,1,0b11111},   {355,1,0b10101},   {358,1,0b00001},   {359,1,0b01011},   {361,0,0b00001},   {362,1,0b01001},   {364,1,0b11001},   {365,0,0b01001},   {366,1,0b01011},   {373,1,0b00111},   {377,0,0b00101},   {378,1,0b01101},   {379,0,0b00001},   {380,1,0b11001},   {381,1,0b10101},   {387,1,0b11101},   {389,1,0b01101},   {390,1,0b01101},   {395,1,0b00001},   {397,1,0b01001},   {399,1,0b11101},   {401,1,0b01011},   {402,1,0b11101},   {403,1,0b11111},   {404,1,0b00111},   {409,1,0b00111},   {410,1,0b11011},   {411,1,0b11111},   {412,0,0b01001},   {413,1,0b10101},   {418,1,0b10111},   {420,1,0b11011},   {430,0,0b00101},   {431,1,0b01101},   {435,0,0b10001},   {436,1,0b10111},   {437,1,0b11101},   {438,1,0b11111},   {440,1,0b01011},   {441,0,0b01001},   {442,0,0b10101},   {443,1,0b10111},   {446,1,0b01111},   {447,0,0b00101},   {448,0,0b01001},   {449,0,0b11011},   {450,1,0b11101},   {455,1,0b10011},   {458,1,0b00101},   {461,1,0b10111},   {463,1,0b11011},   {464,0,0b01111},   {465,1,0b01111},   {466,0,0b00101},   {467,0,0b11001},   {468,1,0b11111},   {469,1,0b10001},   {470,1,0b10001},   {471,1,0b01101},   {476,1,0b01111},   {478,0,0b00011},   {479,1,0b01001},   {480,1,0b01111},   {484,1,0b01101},   {489,1,0b01101},   {490,1,0b10101},   {492,1,0b00011},   {494,0,0b10111},   {495,1,0b11111},   {496,1,0b10101},   {504,1,0b10001},   {507,0,0b10101},   {508,1,0b11001},   {509,1,0b01111},   {513,1,0b10111},   {514,0,0b00001},   {515,1,0b11001},   {524,0,0b00001},   {525,0,0b10101},   {526,0,0b11011},   {527,1,0b11101},   {528,1,0b11111},   {531,1,0b10001},   {532,1,0b11001},   {534,1,0b10111},   {537,0,0b01101},   {538,1,0b10101},   {539,0,0b10101},   {540,1,0b11101},   {541,1,0b00001},   {542,1,0b01001},   {543,1,0b11111},   {544,1,0b11101},   {551,0,0b01111},   {552,0,0b10111},   {553,1,0b11111},   {554,1,0b00011},   {556,0,0b01011},   {557,0,0b10001},   {558,1,0b11101},   {559,1,0b00111},   {560,1,0b10001},   {561,1,0b10011},   {563,1,0b00011},   {564,0,0b00101},   {565,1,0b01011},   {566,1,0b10111},   {567,1,0b01001},   {568,1,0b11111},   {569,1,0b10001},   {571,1,0b00101},   {572,0,0b00101},   {573,0,0b11111},   {574,1,0b11111},   {575,1,0b11101},   {576,1,0b11011},   {580,1,0b01101},   {581,1,0b00111},   {588,1,0b01001},   {589,1,0b11001},   {590,1,0b01101},   {593,1,0b11111},   {594,0,0b01101},   {595,1,0b01111},   {598,1,0b01001},   {601,1,0b11001},   {602,1,0b00001},   {603,1,0b01011},   {606,1,0b00101},   {608,0,0b00011},   {609,1,0b11111},   {615,0,0b00101},   {616,1,0b10101},   {619,1,0b01001},   {621,1,0b10011},   {626,1,0b10011},   {627,0,0b10001},   {628,1,0b11111},   {635,1,0b00101},   {639,1,0b00101},   {643,1,0b11111},   {646,1,0b11111},   {650,1,0b10011},   {652,1,0b11001},   {657,0,0b10011},   {658,1,0b11011},   {659,1,0b10001},   {660,0,0b00001},   {661,1,0b01011},   {664,1,0b11101},   {666,1,0b10011},   {667,1,0b11111},   {673,1,0b11011},   {676,1,0b11001},   {682,0,0b10101},   {683,1,0b11111},   {686,1,0b01111},   {689,1,0b10011},   {691,1,0b10001},   {692,0,0b00111},   {693,1,0b10001},   {694,1,0b10101},   {695,1,0b01011},   {697,1,0b11111},   {699,0,0b11001},   {700,1,0b11011},   {701,1,0b10001},   {703,0,0b10101},   {704,1,0b11111},   {705,1,0b01111},   {707,1,0b11101},   {710,0,0b00101},   {711,1,0b00111},   {712,1,0b00101},   {713,0,0b00011},   {714,1,0b01111},   {715,1,0b01011},   {718,0,0b00001},   {719,1,0b01101},   {724,1,0b10101},   {725,1,0b11111},   {726,1,0b00001},   {729,0,0b10001},   {730,1,0b10101},   {731,1,0b10001},   {732,1,0b10001},   {733,1,0b11101},   {738,1,0b11011},   {740,1,0b10011},   {741,0,0b00111},   {742,1,0b01101},   {744,0,0b00001},   {745,1,0b00111},   {747,1,0b01011},   {750,1,0b00001},   {754,1,0b11001},   {761,0,0b10101},   {762,1,0b10101},   {763,1,0b00111},   {765,1,0b01111},   {766,1,0b10001},   {768,0,0b00001},   {769,1,0b00101},   {774,0,0b10111},   {775,1,0b10111},   {777,0,0b00101},   {778,0,0b01001},   {779,1,0b11111},   {780,1,0b01011},   {781,0,0b10011},   {782,0,0b10011},   {783,1,0b11111},   {784,1,0b11101},   {785,0,0b00001},   {786,1,0b11111},   {787,1,0b01101},   {788,1,0b01001},   {789,1,0b01011},   {792,0,0b01001},   {793,0,0b01011},   {794,1,0b11101},   {795,0,0b01001},   {796,1,0b10001},   {797,1,0b10011},   {799,1,0b10111},   {800,0,0b01101},   {801,1,0b11111},   {803,1,0b01001},   {804,1,0b10111},   {806,0,0b00111},   {807,1,0b10001},   {809,0,0b01001},   {810,0,0b01011},   {811,1,0b10111},   {815,1,0b10001},   {816,0,0b01001},   {817,1,0b10101},   {821,1,0b00011},   {823,1,0b11011},   {824,1,0b11101},   {827,1,0b00001},   {829,0,0b11011},   {830,1,0b11111},   {835,1,0b01101},   {836,1,0b00101},   {837,1,0b01101},   {839,1,0b10011},   {841,1,0b00011},   {842,1,0b10101},   {843,1,0b01011},   {849,1,0b00001},   {851,1,0b10001},   {852,1,0b10001},   {854,1,0b01101},   {855,1,0b00111},   {857,1,0b10111},   {858,1,0b00011},   {859,0,0b00101},   {860,0,0b10101},   {861,1,0b11011},   {862,1,0b01101},   {863,1,0b00001},   {864,1,0b00001},   {865,1,0b00001},   {866,0,0b01011},   {867,1,0b10001},   {868,0,0b00011},   {869,0,0b00111},   {870,1,0b11001},   {871,1,0b00101},   {872,0,0b00111},   {873,1,0b11111},   {874,0,0b10011},   {875,0,0b10011},   {876,1,0b11101},   {877,0,0b10101},   {878,1,0b10101},   {880,1,0b11011},   {883,1,0b01111},   {886,0,0b00101},   {887,1,0b01001},   {888,1,0b00111},   {894,0,0b00111},   {895,1,0b10101},   {896,1,0b11111},   {897,1,0b11101},   {903,1,0b10001},   {907,1,0b00001},   {908,0,0b01111},   {909,1,0b01111},   {910,1,0b10111},   {911,0,0b10001},   {912,1,0b10011},   {914,0,0b00101},   {915,1,0b11001},   {916,1,0b10101},   {917,0,0b11011},   {918,1,0b11111},   {919,1,0b11111},   {922,0,0b00101},   {923,1,0b10001},   {925,1,0b10101},   {931,1,0b11001},   {932,1,0b01101},   {934,1,0b11001},   {935,0,0b10101},   {936,1,0b11111},   {937,1,0b00001},   {938,1,0b10011},   {939,1,0b01001},   {940,1,0b01101},   {947,1,0b00101},   {950,1,0b10111},   {953,0,0b00011},   {954,1,0b01111},   {955,1,0b10011},   {957,0,0b11111},   {958,1,0b11111},   {959,1,0b10011},   {960,1,0b10001},   {962,0,0b00111},   {963,0,0b01111},   {964,1,0b10011},   {965,1,0b11111},   {970,1,0b01001},   {971,1,0b10011},   {975,0,0b00111},   {976,1,0b01111},   {977,1,0b01111},   {978,1,0b01011},   {982,1,0b11101},   {984,0,0b10001},   {985,1,0b10011},   {986,1,0b00011},   {991,0,0b11011},   {992,1,0b11101},   {995,1,0b00011},   {1002,1,0b00101},   {1003,0,0b01011},   {1004,1,0b10011},   {1006,1,0b10011},   {1009,1,0b11001},   {1013,1,0b10011},   {1015,1,0b01101},   {1020,1,0b01001},   {1024,0,0b00101},   {1025,1,0b10111},   {1027,1,0b00101},   {1028,1,0b00001},   {1031,0,0b00001},   {1032,0,0b10101},   {1033,1,0b11001},   {1034,0,0b01011},   {1035,0,0b01011},   {1036,1,0b11001},   {1037,1,0b10001},   {1043,0,0b00101},   {1044,0,0b10011},   {1045,1,0b10011},   {1046,1,0b00111},   {1047,1,0b11011},   {1050,1,0b01111},   {1058,1,0b11111},   {1063,1,0b01101},   {1067,0,0b01101},   {1068,1,0b11101},   {1069,1,0b11111},   {1070,0,0b00111},   {1071,1,0b01011},   {1073,1,0b01101}};
                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
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
                    const uint8_t expected_boundary[expected_boundary_length] = {0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111, 0b11111111};
                    const std::vector<uint32_t> occupieds_pos = {};
                    const std::vector<std::tuple<uint32_t, bool, uint64_t>> checks = {};

                    const uint8_t *res_key;
                    uint32_t res_size, dummy;
                    typename Diva<O>::InfixStore *store;
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
    }


    template <bool O>
    static void SerializeDeserialize() {
        const uint32_t infix_size = 5;
        const uint32_t seed = 1;
        const float load_factor = 0.95;
        const uint32_t n_keys = 40000;

        const uint32_t rng_seed = 2;
        std::mt19937_64 rng(rng_seed);

        std::vector<uint64_t> keys;
        for (int32_t i = 0; i < n_keys; i++)
            keys.push_back(rng());
        std::sort(keys.begin(), keys.end());
        std::vector<std::string> string_keys;
        for (int32_t i = 0; i < n_keys; i++) {
            size_t str_length;
            if constexpr (O)
                str_length = 8;
            else
                str_length = 6 + rng() % 3;
            const uint64_t value = to_big_endian_order(keys[i]);
            string_keys.emplace_back(reinterpret_cast<const char *>(&value), str_length);
        }

        Diva<O> s(infix_size, string_keys.begin(), string_keys.end(), seed, load_factor);
        char *buf = new char[20000000];
        s.Serialize(buf);
        Diva<O> reconstructed_s(buf);
        AssertDivas(s, reconstructed_s);
    }

private:
    template <bool O>
    static void AssertStoreContents(const Diva<O>& s, const typename Diva<O>::InfixStore& store,
                                    const std::vector<uint32_t>& occupieds_pos,
                                    const std::vector<std::tuple<uint32_t, bool, uint64_t>>& checks) {
        REQUIRE_NE(store.ptr, nullptr);
        REQUIRE_EQ(store.GetElemCount(), checks.size());
        const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + 1;
        const uint64_t *runends = store.ptr + 1 + Diva<O>::infix_store_target_size / 64;
        uint32_t ind = 0;
        for (uint32_t i = 0; i < Diva<O>::infix_store_target_size; i++) {
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

        uint32_t check_popcnts[2] = {};
        for (int32_t i = 0; i < Diva<O>::infix_store_target_size / 128; i++) {
            check_popcnts[0] += __builtin_popcountll(occupieds[i]);
            if (static_cast<int32_t>(s.scaled_sizes_[store.GetSizeGrade()]) - i * 64 > 0) {
                const uint64_t mask = BITMASK(std::min(64UL, s.scaled_sizes_[store.GetSizeGrade()] - i * 64));
                check_popcnts[1] += __builtin_popcountll(runends[i] & mask);
            }
        }

        REQUIRE_EQ(popcnts[0], check_popcnts[0]);
        REQUIRE_EQ(popcnts[1], check_popcnts[1]);
    }


    template <bool O>
    static void AssertDivas(const Diva<O>& a, const Diva<O>& b) {
        REQUIRE_EQ(a.infix_store_target_size, b.infix_store_target_size);
        REQUIRE_EQ(a.base_implicit_size, b.base_implicit_size);
        REQUIRE_EQ(a.scale_shift, b.scale_shift);
        REQUIRE_EQ(a.scale_implicit_shift, b.scale_implicit_shift);
        REQUIRE_EQ(a.size_scalar_count, b.size_scalar_count);
        REQUIRE_EQ(a.size_scalar_shrink_grow_sep, b.size_scalar_shrink_grow_sep);
        REQUIRE_EQ(a.load_factor_, b.load_factor_);
        REQUIRE_EQ(a.load_factor_alt_, b.load_factor_alt_);

        REQUIRE_EQ(a.infix_size_, b.infix_size_);
        REQUIRE_EQ(a.rng_seed_, b.rng_seed_);

        for (int32_t i = 0; i < a.size_scalar_count; i++) {
            REQUIRE_EQ(a.size_scalars_[i], b.size_scalars_[i]);
            REQUIRE_EQ(a.scaled_sizes_[i], b.scaled_sizes_[i]);
            REQUIRE_EQ(a.implicit_scalars_[i], b.implicit_scalars_[i]);
        }

        const uint8_t *tree_key_a, *tree_key_b;
        uint32_t tree_key_a_len, tree_key_b_len, dummy;
        typename Diva<O>::InfixStore *store_a, *store_b;
        if constexpr (O) {
            wormhole_int_iter it_a, it_b;
            it_a.ref = a.better_tree_int_;
            it_a.map = a.better_tree_int_->map;
            it_a.leaf = nullptr;
            it_a.is = 0;
            it_b.ref = b.better_tree_int_;
            it_b.map = b.better_tree_int_->map;
            it_b.leaf = nullptr;
            it_b.is = 0;
            wh_int_iter_seek(&it_a, nullptr, 0);
            wh_int_iter_seek(&it_b, nullptr, 0);
            while (wh_int_iter_valid(&it_a) && wh_int_iter_valid(&it_b)) {
                wh_int_iter_peek_ref(&it_a, reinterpret_cast<const void **>(&tree_key_a), &tree_key_a_len, 
                                            reinterpret_cast<void **>(&store_a), &dummy);
                wh_int_iter_peek_ref(&it_b, reinterpret_cast<const void **>(&tree_key_b), &tree_key_b_len, 
                                            reinterpret_cast<void **>(&store_b), &dummy);
                REQUIRE_EQ(tree_key_a_len, tree_key_b_len);
                REQUIRE_EQ(memcmp(tree_key_a, tree_key_b, tree_key_a_len), 0);
                REQUIRE_EQ(store_a->status, store_b->status);
                const uint32_t slot_count = a.scaled_sizes_[store_a->GetSizeGrade()];
                const uint32_t word_count = store_a->GetPtrWordCount(slot_count, a.infix_size_);
                REQUIRE_EQ(memcmp(store_a->ptr, store_b->ptr, word_count * sizeof(uint64_t)), 0);
                wh_int_iter_skip1(&it_a);
                wh_int_iter_skip1(&it_b);
            }
            REQUIRE_EQ(wh_int_iter_valid(&it_a), wh_int_iter_valid(&it_b));
            if (it_a.leaf)
                wormleaf_int_unlock_read(it_a.leaf);
            if (it_b.leaf)
                wormleaf_int_unlock_read(it_b.leaf);
        }
        else {
            wormhole_iter it_a, it_b;
            it_a.ref = a.better_tree_;
            it_a.map = a.better_tree_->map;
            it_a.leaf = nullptr;
            it_a.is = 0;
            it_b.ref = b.better_tree_;
            it_b.map = b.better_tree_->map;
            it_b.leaf = nullptr;
            it_b.is = 0;
            wh_iter_seek(&it_a, nullptr, 0);
            wh_iter_seek(&it_b, nullptr, 0);
            while (wh_iter_valid(&it_a) && wh_iter_valid(&it_b)) {
                wh_iter_peek_ref(&it_a, reinterpret_cast<const void **>(&tree_key_a), &tree_key_a_len, 
                                        reinterpret_cast<void **>(&store_a), &dummy);
                wh_iter_peek_ref(&it_b, reinterpret_cast<const void **>(&tree_key_b), &tree_key_b_len, 
                                        reinterpret_cast<void **>(&store_b), &dummy);
                REQUIRE_EQ(tree_key_a_len, tree_key_b_len);
                REQUIRE_EQ(memcmp(tree_key_a, tree_key_b, tree_key_a_len), 0);
                REQUIRE_EQ(store_a->status, store_b->status);
                const uint32_t slot_count = a.scaled_sizes_[store_a->GetSizeGrade()];
                const uint32_t word_count = store_a->GetPtrWordCount(slot_count, a.infix_size_);
                REQUIRE_EQ(memcmp(store_a->ptr, store_b->ptr, word_count * sizeof(uint64_t)), 0);
                wh_iter_skip1(&it_a);
                wh_iter_skip1(&it_b);
            }
            REQUIRE_EQ(wh_iter_valid(&it_a), wh_iter_valid(&it_b));
            if (it_a.leaf)
                wormleaf_unlock_read(it_a.leaf);
            if (it_b.leaf)
                wormleaf_unlock_read(it_b.leaf);
        }
    }


    template <bool O>
    static void PrintStore(const Diva<O>& s, const typename Diva<O>::InfixStore& store) {
        const uint32_t size_grade = store.GetSizeGrade();
        const uint32_t *popcnts = reinterpret_cast<const uint32_t *>(store.ptr);
        const uint64_t *occupieds = store.ptr + 1;
        const uint64_t *runends = store.ptr + 1 + Diva<O>::infix_store_target_size / 64;

        std::cerr << "is_partial=" << store.IsPartialKey() << " invalid_bits=" << store.GetInvalidBits();
        std::cerr << " size_grade=" << size_grade << " elem_count=" << store.GetElemCount();
        std::cerr << " --- ptr=" << store.ptr << std::endl;
        std::cerr << "popcnts=[" << popcnts[0] << ", " << popcnts[1] << ']' << std::endl;
        std::cerr << "occupieds: ";
        for (int32_t i = 0; i < Diva<O>::infix_store_target_size; i++) {
            if ((occupieds[i / 64] >> (i % 64)) & 1ULL)
                std::cerr << i << ", ";
        }
        std::cerr << std::endl << "runends + slots:" << std::endl;;
        int32_t cnt = 0;
        for (int32_t i = 0; i < s.scaled_sizes_[size_grade]; i++) {
            const uint64_t value = s.GetSlot(store, i);
            if (value == 0)
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
        std::cerr << std::endl;
    }
};


TEST_SUITE("diva") {
    TEST_CASE("insert") {
        DivaTests::Insert<false>();
    }

    TEST_CASE("random inserts") {
        DivaTests::RandomInsert<false>();
    }

    TEST_CASE("point query") {
        DivaTests::PointQuery<false>();
    }

    TEST_CASE("range query") {
        DivaTests::RangeQuery<false>();
    }

    TEST_CASE("delete") {
        DivaTests::Delete<false>();
    }

    TEST_CASE("shrink infix size") {
        DivaTests::ShrinkInfixSize<false>();
    }

    TEST_CASE("bulk load") {
        DivaTests::BulkLoad<false>();
    }

    TEST_CASE("serialize and deserialize") {
        DivaTests::SerializeDeserialize<false>();
    }
}

TEST_SUITE("diva (int optimized)") {
    TEST_CASE("insert") {
        DivaTests::Insert<true>();
    }

    TEST_CASE("random inserts") {
        DivaTests::RandomInsert<true>();
    }

    TEST_CASE("point query") {
        DivaTests::PointQuery<true>();
    }

    TEST_CASE("range query") {
        DivaTests::RangeQuery<true>();
    }

    TEST_CASE("delete") {
        DivaTests::Delete<true>();
    }

    TEST_CASE("shrink infix size") {
        DivaTests::ShrinkInfixSize<true>();
    }

    TEST_CASE("bulk load") {
        DivaTests::BulkLoad<true>();
    }

    TEST_CASE("serialize and deserialize") {
        DivaTests::SerializeDeserialize<true>();
    }
}

