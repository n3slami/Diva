#pragma once

#include "wormhole/lib.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <immintrin.h>

#define MAX_VALUE(nbits) ((1ULL << (nbits)) - 1)
#define BITMASK(nbits)                                    \
  ((nbits) == 64 ? 0xffffffffffffffff : MAX_VALUE(nbits))


/**
 * Returns the position of the k-th 1 in the 64-bit word x.
 * k is 0-based, so k=0 returns the position of the first 1.
 *
 * Uses the broadword selection algorithm by Vigna [1], improved by Gog
 * and Petri [2] and Vigna [3].
 *
 * [1] Sebastiano Vigna. Broadword Implementation of Rank/Select
 *    Queries. WEA, 2008
 *
 * [2] Simon Gog, Matthias Petri. Optimized succinct data
 * structures for massive data. Softw. Pract. Exper., 2014
 *
 * [3] Sebastiano Vigna. MG4J 5.2.1. http://mg4j.di.unimi.it/
 * The following code is taken from
 * https://github.com/facebook/folly/blob/b28186247104f8b90cfbe094d289c91f9e413317/folly/experimental/Select64.h
 */
const uint8_t kSelectInByte[2048] = {
	8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0,
	1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0,
	2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0,
	1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0,
	3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7, 0,
	1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0,
	2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0,
	1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0,
	1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 8, 8, 8, 1,
	8, 2, 2, 1, 8, 3, 3, 1, 3, 2, 2, 1, 8, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2,
	2, 1, 8, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1,
	4, 3, 3, 1, 3, 2, 2, 1, 8, 6, 6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1, 6, 4,
	4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1,
	3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 8, 7, 7, 1, 7, 2,
	2, 1, 7, 3, 3, 1, 3, 2, 2, 1, 7, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
	7, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3,
	3, 1, 3, 2, 2, 1, 7, 6, 6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1, 6, 4, 4, 1,
	4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2,
	2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 8, 8, 8, 8, 8, 8, 8, 2,
	8, 8, 8, 3, 8, 3, 3, 2, 8, 8, 8, 4, 8, 4, 4, 2, 8, 4, 4, 3, 4, 3, 3, 2, 8, 8,
	8, 5, 8, 5, 5, 2, 8, 5, 5, 3, 5, 3, 3, 2, 8, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3,
	4, 3, 3, 2, 8, 8, 8, 6, 8, 6, 6, 2, 8, 6, 6, 3, 6, 3, 3, 2, 8, 6, 6, 4, 6, 4,
	4, 2, 6, 4, 4, 3, 4, 3, 3, 2, 8, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2,
	6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2, 8, 8, 8, 7, 8, 7, 7, 2, 8, 7,
	7, 3, 7, 3, 3, 2, 8, 7, 7, 4, 7, 4, 4, 2, 7, 4, 4, 3, 4, 3, 3, 2, 8, 7, 7, 5,
	7, 5, 5, 2, 7, 5, 5, 3, 5, 3, 3, 2, 7, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3,
	3, 2, 8, 7, 7, 6, 7, 6, 6, 2, 7, 6, 6, 3, 6, 3, 3, 2, 7, 6, 6, 4, 6, 4, 4, 2,
	6, 4, 4, 3, 4, 3, 3, 2, 7, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2, 6, 5,
	5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 3, 8, 8, 8, 8, 8, 8, 8, 4, 8, 8, 8, 4, 8, 4, 4, 3, 8, 8, 8, 8, 8, 8,
	8, 5, 8, 8, 8, 5, 8, 5, 5, 3, 8, 8, 8, 5, 8, 5, 5, 4, 8, 5, 5, 4, 5, 4, 4, 3,
	8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 6, 8, 6, 6, 3, 8, 8, 8, 6, 8, 6, 6, 4, 8, 6,
	6, 4, 6, 4, 4, 3, 8, 8, 8, 6, 8, 6, 6, 5, 8, 6, 6, 5, 6, 5, 5, 3, 8, 6, 6, 5,
	6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7,
	7, 3, 8, 8, 8, 7, 8, 7, 7, 4, 8, 7, 7, 4, 7, 4, 4, 3, 8, 8, 8, 7, 8, 7, 7, 5,
	8, 7, 7, 5, 7, 5, 5, 3, 8, 7, 7, 5, 7, 5, 5, 4, 7, 5, 5, 4, 5, 4, 4, 3, 8, 8,
	8, 7, 8, 7, 7, 6, 8, 7, 7, 6, 7, 6, 6, 3, 8, 7, 7, 6, 7, 6, 6, 4, 7, 6, 6, 4,
	6, 4, 4, 3, 8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 3, 7, 6, 6, 5, 6, 5,
	5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 5, 8, 8, 8, 8, 8, 8, 8, 5, 8, 8, 8, 5, 8, 5, 5, 4, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 6, 8, 6,
	6, 4, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 6, 8, 6, 6, 5, 8, 8, 8, 6, 8, 6, 6, 5,
	8, 6, 6, 5, 6, 5, 5, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8,
	8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 4, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7,
	8, 7, 7, 5, 8, 8, 8, 7, 8, 7, 7, 5, 8, 7, 7, 5, 7, 5, 5, 4, 8, 8, 8, 8, 8, 8,
	8, 7, 8, 8, 8, 7, 8, 7, 7, 6, 8, 8, 8, 7, 8, 7, 7, 6, 8, 7, 7, 6, 7, 6, 6, 4,
	8, 8, 8, 7, 8, 7, 7, 6, 8, 7, 7, 6, 7, 6, 6, 5, 8, 7, 7, 6, 7, 6, 6, 5, 7, 6,
	6, 5, 6, 5, 5, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 5, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 6,
	8, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7,
	8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 6, 8, 8, 8, 8,
	8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 6, 8, 8, 8, 7, 8, 7, 7, 6, 8, 7, 7, 6, 7, 6,
	6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 6, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7
};

static inline uint64_t _select64(uint64_t x, int k)
{
	if (k >= __builtin_popcountll(x)) { return 64; }

	const uint64_t kOnesStep4  = 0x1111111111111111ULL;
	const uint64_t kOnesStep8  = 0x0101010101010101ULL;
	const uint64_t kMSBsStep8  = 0x80ULL * kOnesStep8;

	uint64_t s = x;
	s = s - ((s & 0xA * kOnesStep4) >> 1);
	s = (s & 0x3 * kOnesStep4) + ((s >> 2) & 0x3 * kOnesStep4);
	s = (s + (s >> 4)) & 0xF * kOnesStep8;
	uint64_t byteSums = s * kOnesStep8;

	uint64_t kStep8 = k * kOnesStep8;
	uint64_t geqKStep8 = (((kStep8 | kMSBsStep8) - byteSums) & kMSBsStep8);
	uint64_t place = __builtin_popcountll(geqKStep8) * 8;
	uint64_t byteRank = k - (((byteSums << 8) >> place) & (uint64_t)(0xFF));
	return place + kSelectInByte[((x >> place) & 0xFF) | (byteRank << 8)];
}

// Returns the position of the rank'th 1.  (rank = 0 returns the first 1)
// Returns 64 if there are fewer than rank+1 1s.
__attribute__((always_inline))
static inline uint32_t bit_select(uint64_t val, uint32_t i) {
#ifdef __BMI2__
    uint64_t tmp = 1ULL << i;
    tmp = _pdep_u64(tmp, val);
    return __builtin_ia32_tzcnt_u64(tmp);
#endif
    return _select64(val, i);
}


__attribute__((always_inline))
static inline uint32_t bit_rank(uint64_t val, uint32_t i) {
    return __builtin_popcountll(val & BITMASK(i));
}


__attribute__((always_inline))
static inline int32_t lowbit_pos(uint64_t val) {
    return __builtin_ia32_tzcnt_u64(val);
}


__attribute__((always_inline))
static inline int32_t highbit_pos(uint64_t val) {
    return 8 * sizeof(val) - __builtin_ia32_lzcnt_u64(val) - 1;
}


__attribute__((always_inline))
static inline void set_bitmap_bit(uint64_t *bitmap, const uint32_t pos) {
    bitmap[pos / 64] |= 1ULL << (pos % 64);
}


__attribute__((always_inline))
static inline void reset_bitmap_bit(uint64_t *bitmap, const uint32_t pos) {
    bitmap[pos / 64] &= ~(1ULL << (pos % 64));
}


__attribute__((always_inline))
static inline uint64_t get_bitmap_bit(const uint64_t *bitmap, const uint32_t pos) {
    return (bitmap[pos / 64] >> (pos % 64)) & 1ULL;
}


__attribute__((always_inline))
inline auto to_big_endian_order(uint64_t const &key) {
	return __builtin_bswap64(key);
};

__attribute__((always_inline))
inline auto to_big_endian_order(uint32_t const &key) {
	return __builtin_bswap32(key);
};


__attribute__((always_inline))
inline auto to_big_endian_order(uint16_t const &key) {
	return __builtin_bswap16(key);
};


// Inclusive range
//__attribute__((always_inline))
inline void zero_out_bitmap(uint64_t *ptr, int32_t l, int32_t r) {
    while (l <= r) {
        const int32_t offset = l % 64;
        const uint32_t erase_amount = std::min(r - l, 63 - offset) + 1;
        ptr[l / 64] &= ~(BITMASK(erase_amount) << offset);
        l += erase_amount;
    }
}


//__attribute__((always_inline))
inline void zero_out_bitmap_unaligned(uint64_t *ptr, int32_t l, int32_t r) {
    while (l <= r) {
        const int32_t offset = l % 64;
        const uint32_t erase_amount = std::min(r - l, 63 - offset) + 1;
        uint64_t val;
        memcpy(&val, ptr + l / 64, sizeof(val));
        val &= ~(BITMASK(erase_amount) << offset);
        memcpy(ptr + l / 64, &val, sizeof(val));
        l += erase_amount;
    }
}


//__attribute__((always_inline))
inline void move_bitmap_right(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt) {
    const int32_t l_src_bit_pos = l;
    int32_t r_src_bit_pos = r;
    int32_t dst_bit_pos = r_src_bit_pos + shamt;
    while (r_src_bit_pos >= l_src_bit_pos) {
        const int32_t src_offset = r_src_bit_pos % 64;
        const int32_t dst_offset = dst_bit_pos % 64;
        const int32_t move_amount = std::min(r_src_bit_pos - l_src_bit_pos,
                                             std::min(src_offset, dst_offset)) + 1;
        const uint64_t move_mask = BITMASK(move_amount);
        const uint64_t payload = (ptr[r_src_bit_pos / 64] >> (src_offset - move_amount + 1)) & move_mask;

        ptr[dst_bit_pos / 64] &= ~(move_mask << (dst_offset - move_amount + 1));
        ptr[dst_bit_pos / 64] |= payload << (dst_offset - move_amount + 1);

        r_src_bit_pos -= move_amount;
        dst_bit_pos -= move_amount;
    }
}


//__attribute__((always_inline))
inline void move_bitmap_right_unaligned(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt) {
    const int32_t l_src_bit_pos = l;
    int32_t r_src_bit_pos = r;
    int32_t dst_bit_pos = r_src_bit_pos + shamt;
    while (r_src_bit_pos >= l_src_bit_pos) {
        const int32_t src_offset = r_src_bit_pos % 64;
        const int32_t dst_offset = dst_bit_pos % 64;
        const int32_t move_amount = std::min(r_src_bit_pos - l_src_bit_pos,
                                             std::min(src_offset, dst_offset)) + 1;
        const uint64_t move_mask = BITMASK(move_amount);
        const uint64_t payload = (ptr[r_src_bit_pos / 64] >> (src_offset - move_amount + 1)) & move_mask;

        uint64_t val;
        memcpy(&val, ptr + dst_bit_pos / 64, sizeof(dst_bit_pos));
        val &= ~(move_mask << (dst_offset - move_amount + 1));
        val |= payload << (dst_offset - move_amount + 1);
        memcpy(ptr + dst_bit_pos / 64, &val, sizeof(dst_bit_pos));

        r_src_bit_pos -= move_amount;
        dst_bit_pos -= move_amount;
    }
}


//__attribute__((always_inline))
inline void shift_bitmap_right(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt) {
    move_bitmap_right(ptr, l, r, shamt);
    zero_out_bitmap(ptr, l, l + shamt - 1);
}


//__attribute__((always_inline))
inline void shift_bitmap_right_unaligned(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt) {
    move_bitmap_right_unaligned(ptr, l, r, shamt);
    zero_out_bitmap_unaligned(ptr, l, l + shamt - 1);
}


//__attribute__((always_inline))
inline void move_bitmap_left(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt) {
    int32_t l_src_bit_pos = l;
    const int32_t r_src_bit_pos = r;
    int32_t dst_bit_pos = l_src_bit_pos - shamt;
    while (l_src_bit_pos <= r_src_bit_pos) {
        const int32_t src_offset = l_src_bit_pos % 64;
        const int32_t dst_offset = dst_bit_pos % 64;
        const uint32_t move_amount = std::min(r_src_bit_pos - l_src_bit_pos,
                                              63 - std::max(src_offset, dst_offset)) + 1;
        const uint64_t move_mask = BITMASK(move_amount);
        const uint64_t payload = (ptr[l_src_bit_pos / 64] >> src_offset) & move_mask;

        ptr[dst_bit_pos / 64] &= ~(move_mask << dst_offset);
        ptr[dst_bit_pos / 64] |= payload << dst_offset;

        l_src_bit_pos += move_amount;
        dst_bit_pos += move_amount;
    }
}


//__attribute__((always_inline))
inline void move_bitmap_left_unaligned(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt) {
    int32_t l_src_bit_pos = l;
    const int32_t r_src_bit_pos = r;
    int32_t dst_bit_pos = l_src_bit_pos - shamt;
    while (l_src_bit_pos <= r_src_bit_pos) {
        const int32_t src_offset = l_src_bit_pos % 64;
        const int32_t dst_offset = dst_bit_pos % 64;
        const uint32_t move_amount = std::min(r_src_bit_pos - l_src_bit_pos,
                                              63 - std::max(src_offset, dst_offset)) + 1;
        const uint64_t move_mask = BITMASK(move_amount);
        const uint64_t payload = (ptr[l_src_bit_pos / 64] >> src_offset) & move_mask;

        uint64_t val;
        memcpy(&val, ptr + dst_bit_pos / 64, sizeof(val));
        val &= ~(move_mask << dst_offset);
        val |= payload << dst_offset;
        memcpy(ptr + dst_bit_pos / 64, &val, sizeof(val));

        l_src_bit_pos += move_amount;
        dst_bit_pos += move_amount;
    }
}


//__attribute__((always_inline))
inline void shift_bitmap_left(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt) {
    move_bitmap_left(ptr, l, r, shamt);
    zero_out_bitmap(ptr, r - shamt + 1, r);
}


//__attribute__((always_inline))
inline void shift_bitmap_left_unaligned(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt) {
    move_bitmap_left_unaligned(ptr, l, r, shamt);
    zero_out_bitmap_unaligned(ptr, r - shamt + 1, r);
}


// Assumes word-aligned buffers
__attribute__((always_inline))
inline void copy_bitmap_to_bitmap(const void *in_, uint32_t pos_in,
                                        void *out_, uint32_t pos_out,
                                  const uint32_t num_bits_to_copy) {
    const uint64_t *in = reinterpret_cast<const uint64_t *>(in_);
    uint64_t *out = reinterpret_cast<uint64_t *>(out_);
    const uint32_t buf_size = 8 * sizeof(uint64_t);
    const uint32_t pos_in_end = pos_in + num_bits_to_copy;
    while (pos_in < pos_in_end) {
        const uint32_t amount_to_move = std::min(pos_in_end - pos_in, std::min(buf_size - pos_in % buf_size,
                                                                               buf_size - pos_out % buf_size));
        const uint64_t mask_in = BITMASK(amount_to_move) << (pos_in % buf_size);
        const uint64_t mask_out = BITMASK(amount_to_move) << (pos_out % buf_size);
        out[pos_out / buf_size] &= ~mask_out;
        out[pos_out / buf_size] |= ((in[pos_in / buf_size] & mask_in) >> (pos_in % buf_size)) 
                                    << (pos_out % buf_size);
        pos_in += amount_to_move;
        pos_out += amount_to_move;
    }
}


// Assumes word-aligned buffers
//__attribute__((always_inline))
inline bool compare_bitmap_to_bitmap(const uint64_t *a, uint32_t pos_a,
                                     const uint64_t *b, uint32_t pos_b,
                                     const uint32_t num_bits_to_compare) {
    const uint32_t buf_size = 8 * sizeof(uint64_t);
    const uint32_t pos_a_end = pos_a + num_bits_to_compare;
    while (pos_a < pos_a_end) {
        const uint32_t amount_to_compare = std::min(pos_a_end - pos_a, std::min(buf_size - pos_a % buf_size,
                                                                                buf_size - pos_b % buf_size));
        const uint64_t val_a = a[pos_a / buf_size] >> (pos_a % buf_size);
        const uint64_t val_b = b[pos_b / buf_size] >> (pos_b % buf_size);
        if ((val_a & BITMASK(amount_to_compare)) != (val_b & BITMASK(amount_to_compare)))
            return false;
        pos_a += amount_to_compare;
        pos_b += amount_to_compare;
    }
    return true;
}


// Assumes word-aligned buffers
__attribute__((always_inline))
inline uint64_t read_data_from_bitmap(const void *bitmap, uint32_t& bit_pos,
                                      uint64_t& buffer_word, uint32_t& buffer_word_filled_bits,
                                      uint32_t num_bits_to_read) {
    const uint64_t *bitmap_words = reinterpret_cast<const uint64_t *>(bitmap);
    const bool fetch_next_word = num_bits_to_read > buffer_word_filled_bits;
    uint32_t bits_to_move = fetch_next_word ? buffer_word_filled_bits : num_bits_to_read;

    uint64_t res = buffer_word & BITMASK(bits_to_move);
    uint32_t res_filled_bits = bits_to_move;
    buffer_word >>= bits_to_move;
    buffer_word_filled_bits -= bits_to_move;
    if (fetch_next_word) {
        num_bits_to_read -= bits_to_move;
        while (num_bits_to_read > 0) {
            buffer_word_filled_bits = 64 - bit_pos % 64;
            buffer_word = bitmap_words[bit_pos / 64];
            buffer_word >>= bit_pos % 64;
            bit_pos += buffer_word_filled_bits;

            bits_to_move = std::min(num_bits_to_read, buffer_word_filled_bits);
            res |= (buffer_word & BITMASK(bits_to_move)) << res_filled_bits;
            res_filled_bits += bits_to_move;
            buffer_word >>= bits_to_move;
            buffer_word_filled_bits -= bits_to_move;
            num_bits_to_read -= bits_to_move;
        }
    }
    return res;
}


// Assumes word-aligned buffers
__attribute__((always_inline))
inline void write_bits_to_bitmap(void *bitmap, uint32_t bitmap_pos,
                                 uint64_t bits,
                                 uint32_t num_bits_to_copy) {
    bits &= BITMASK(num_bits_to_copy);
    uint64_t *bitmap_words = reinterpret_cast<uint64_t *>(bitmap);
    bitmap_words[bitmap_pos / 64] &= ~(BITMASK(num_bits_to_copy) << (bitmap_pos % 64));
    bitmap_words[bitmap_pos / 64] |= bits << (bitmap_pos % 64);
    const int32_t next_word_bit_count = std::max(static_cast<int32_t>(bitmap_pos) % 64 
                                            + static_cast<int32_t>(num_bits_to_copy) - 64, 0);
    if (next_word_bit_count > 0) {
        bitmap_words[bitmap_pos / 64 + 1] &= ~BITMASK(next_word_bit_count);
        bitmap_words[bitmap_pos / 64 + 1] |= bits >> (num_bits_to_copy - next_word_bit_count);
    }
}


// Assumes word-aligned buffers
__attribute__((always_inline))
inline void write_bits_from_string_to_bitmap(void *bitmap, uint32_t bitmap_pos,
                                             const void *str, uint32_t str_pos, 
                                             uint32_t num_bits_to_copy) {
    uint64_t *bitmap_words = reinterpret_cast<uint64_t *>(bitmap);
    const uint64_t *str_words = reinterpret_cast<const uint64_t *>(str);
    const uint32_t bitmap_pos_start = bitmap_pos;
    bitmap_pos += num_bits_to_copy;
    while (bitmap_pos > bitmap_pos_start) {
        int32_t bit_count_to_write = std::min<int32_t>(64 - str_pos % 64,
                                                       bitmap_pos - bitmap_pos_start);
        bitmap_pos -= bit_count_to_write;
        uint64_t data = 0;
        if (str) {
            memcpy(&data, str_words + str_pos / 64, std::min(bit_count_to_write + str_pos % 64 + 7, 64U) / 8);
            data = __builtin_bswap64(data) >> (8 * sizeof(data) - bit_count_to_write - str_pos % 64);
            data &= BITMASK(bit_count_to_write);
        }
        write_bits_to_bitmap(bitmap_words, bitmap_pos, data, bit_count_to_write);
        str_pos += bit_count_to_write;
    }
}


// Assumes word-aligned buffers
__attribute__((always_inline))
inline void write_bits_from_bitmap_to_string(void *str, uint32_t str_pos,
                                             const void *bitmap, uint32_t bitmap_pos, 
                                             uint32_t num_bits_to_copy) {
    uint64_t *str_words = reinterpret_cast<uint64_t *>(str);
    const uint32_t bitmap_pos_start = bitmap_pos;
    bitmap_pos += num_bits_to_copy;
    while (bitmap_pos > bitmap_pos_start) {
        const uint32_t bit_count_to_write = std::min(64 - str_pos % 64, num_bits_to_copy);
        bitmap_pos -= bit_count_to_write;
        uint64_t data = 0;
        copy_bitmap_to_bitmap(bitmap, bitmap_pos, &data, 0, bit_count_to_write);
        data = __builtin_bswap64(data << (64 - bit_count_to_write - str_pos % 64));
        str_words[str_pos / 64] |= data;
        str_pos += bit_count_to_write;
        num_bits_to_copy -= bit_count_to_write;
    }
}

// Assumes word-aligned buffers
// Returns the position of the first differring bit +1, multiplied by the usual
// comparison sign.
__attribute__((always_inline))
inline int64_t compare_bits_from_string_to_bitmap(const void *bitmap, uint32_t bitmap_pos,
                                                  const void *str, uint32_t str_pos,
                                                  uint32_t num_bits_to_compare) {
    int64_t match_len = 0;
    const uint64_t *str_words = reinterpret_cast<const uint64_t *>(str);
    const uint32_t bitmap_pos_start = bitmap_pos;
    bitmap_pos += num_bits_to_compare;
    while (bitmap_pos > bitmap_pos_start) {
        const uint32_t bit_count_to_compare = std::min(bitmap_pos - bitmap_pos_start,
                                                       64 - str_pos % 64);
        bitmap_pos -= bit_count_to_compare;

        uint64_t data_bitmap_buf = 0;
        uint32_t data_bitmap_buf_filled_bits = 0, read_bitmap_pos = bitmap_pos;
        uint64_t data_bitmap = read_data_from_bitmap(bitmap, read_bitmap_pos,
                                                     data_bitmap_buf, data_bitmap_buf_filled_bits,
                                                     bit_count_to_compare);

        uint64_t data_str = str_words[str_pos / 64];
        data_str = __builtin_bswap64(data_str) 
                    >> (8 * sizeof(data_str) - bit_count_to_compare - str_pos % 64);
        data_str &= BITMASK(bit_count_to_compare);

        const uint64_t data_diff = data_bitmap ^ data_str;
        const uint32_t first_diff_bit_pos = highbit_pos(data_diff);
        if (first_diff_bit_pos < 64)
            return (match_len + bit_count_to_compare - first_diff_bit_pos)
                    * (((data_str >> first_diff_bit_pos) & 1) ? 1 : -1);
        match_len += bit_count_to_compare;

        str_pos += bit_count_to_compare;
    }
    return 0;
}


// Assumes word-aligned buffers
__attribute__((always_inline))
inline void write_varlen_counter_to_bitmap(uint64_t *bitmap, int32_t bitmap_pos, uint64_t counter, const uint32_t counter_fragment_len) {
    const uint64_t varlen_counter_encoding_base = 1UL << counter_fragment_len;
    uint32_t digit_count = 1;
    for (uint64_t pw = varlen_counter_encoding_base; pw <= counter; pw *= varlen_counter_encoding_base)
        digit_count++;
    uint32_t bit_pos = digit_count + 1;
    uint64_t counter_encoding = 1ULL << digit_count;
    while (counter) {
        counter_encoding |= (counter % varlen_counter_encoding_base) << bit_pos;
        counter /= varlen_counter_encoding_base;
        bit_pos += counter_fragment_len;
    }
    write_bits_to_bitmap(bitmap, bitmap_pos, counter_encoding, digit_count * (counter_fragment_len + 1) + 1);
}


// Synchronization and Locking Primitives
typedef uint8_t lock_t;         // Should change the dang wormhole to allow for longer locks
static constexpr lock_t rwlock_no_access = 0;
static constexpr lock_t rwlock_write_bit = 1ULL << (8 * sizeof(lock_t) - 1);


__attribute__((always_inline))
inline void cpu_pause() {
#if defined(__x86_64__)
    _mm_pause();
#elif defined(__aarch64__)
    // nop
#endif
}


__attribute__((always_inline))
inline bool rwlock_trylock_read(std::atomic<lock_t>& lock) {
    if ((lock.fetch_add(1, std::memory_order_acquire) & rwlock_write_bit) == 0)
        return true;
    else {
        lock.fetch_sub(1, std::memory_order_relaxed);
        return false;
    }
}

__attribute__((always_inline))
inline void rwlock_lock_read(std::atomic<lock_t>& lock) {
#pragma nounroll
    do {
        if (rwlock_trylock_read(lock))
            return;
#pragma nounroll
        do {
            cpu_pause();
        } while (lock.load(std::memory_order_acquire) & rwlock_write_bit);
    } while (true);
}

__attribute__((always_inline))
inline void rwlock_unlock_read(std::atomic<lock_t>& lock) {
    lock.fetch_sub(1, std::memory_order_release);
}

__attribute__((always_inline))
inline bool rwlock_trylock_write(std::atomic<lock_t>& lock) {
    lock_t v0 = lock.load(std::memory_order_acquire);
    if (v0 == rwlock_no_access && lock.compare_exchange_weak(v0, rwlock_write_bit,
                                                             std::memory_order_acquire,
                                                             std::memory_order_relaxed)) {
        return true;
    }
    else {
        return false;
    }
}

__attribute__((always_inline))
inline void rwlock_lock_write(std::atomic<lock_t>& lock) {
#pragma nounroll
    do {
        if (rwlock_trylock_write(lock))
            return;
#pragma nounroll
        do {
            cpu_pause();
        } while (lock.load(std::memory_order_acquire));
    } while (true);
}

__attribute__((always_inline))
inline void rwlock_unlock_write(std::atomic<lock_t>& lock) {
    lock.fetch_sub(rwlock_write_bit, std::memory_order_release);
}


