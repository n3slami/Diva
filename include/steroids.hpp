#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <endian.h>
#include <iomanip>
#include <iostream>
#include <random>
#include <string_view>
#include <tuple>
#include <x86intrin.h>

#include "wormhole/wh.h"
#include "util.hpp"
#include "wormhole/wh_int.h"

// TODO: We have a pointer that goes out of the arrays bounds when calling
// `GetSharedIgnoreImplicitLengths`. Okay, maybe?

static void print_key(const uint8_t *key, const uint32_t key_len, const bool binary=true) {
    for (int32_t i = 0; i < key_len; i++) {
        if (binary) {
            for (int32_t j = 7; j >= 0; j--)
                std::cerr << +((key[i] >> j) & 1);
            std::cerr << ' ';
        }
        else
            std::cerr << std::setfill(' ') << std::setw(3) << +key[i] << ' ';
    }
    std::cerr << std::endl;
}


static void print_key(const char *key, const uint32_t key_len, const bool binary=true) {
    print_key(reinterpret_cast<const uint8_t *>(key), key_len, binary);
}


template <bool int_optimized>
class Steroids {
    friend class SteroidsTests;
    friend class InfixStoreTests;

public:
    Steroids(const uint32_t infix_size, const uint32_t rng_seed, const float load_factor);

    template <class t_itr>
    Steroids(const uint32_t infix_size, t_itr begin, t_itr end, const uint32_t key_len,
             const uint32_t rng_seed, const float load_factor);

    template <class t_itr>
    Steroids(const uint32_t infix_size, t_itr begin, t_itr end, 
             const uint32_t rng_seed, const float load_factor);

    void Insert(std::string_view key);
    void Insert(const uint8_t *key, const uint32_t key_len);
    void Delete(std::string_view input_key);
    void Delete(const uint8_t *input_key, const uint32_t input_key_len);
    bool RangeQuery(std::string_view input_l, std::string_view input_r) const;
    bool RangeQuery(const uint8_t *input_l, const uint32_t input_l_len,
                    const uint8_t *input_r, const uint32_t input_r_len) const;
    bool PointQuery(std::string_view key) const;
    bool PointQuery(const uint8_t *key, const uint32_t key_len) const;
    void ShrinkInfixSize(const uint32_t new_infix_size);
    uint32_t Size() const;

private:
    static constexpr uint32_t infix_store_target_size = 512;
    static_assert(infix_store_target_size % 64 == 0);
    static constexpr uint32_t base_implicit_size = 9;
    static constexpr uint32_t scale_shift = 15;
    static constexpr uint32_t scale_implicit_shift = 15;
    static constexpr uint32_t size_scalar_count = 100;

    struct InfiniteByteString {
        const uint8_t *str;
        uint32_t length;

        InfiniteByteString& operator=(const InfiniteByteString& other) {
            str = other.str;
            length = other.length;
            return *this;
        }

        __attribute__((always_inline))
        uint64_t WordAt(const uint32_t byte_pos) const {
            if (byte_pos >= length)
                return 0;
            uint64_t res = 0;
            memcpy(&res, str + byte_pos, std::min<uint32_t>(sizeof(res), length - byte_pos));
            return __bswap_64(res);
        };

        __attribute__((always_inline))
        uint64_t BitsAt(const uint32_t bit_pos, const uint32_t res_width) const {
            if (bit_pos / 8 >= length)
                return 0;
            uint64_t res = 0;
            memcpy(&res, str + bit_pos / 8, std::min<uint32_t>(sizeof(res), length - bit_pos / 8));
            res = __bswap_64(res) >> (8 * sizeof(res) - res_width - bit_pos % 8);
            return res & BITMASK(res_width);
        };

        __attribute__((always_inline))
        uint32_t GetBit(const uint32_t pos) const {
            return (pos / 8 < length ? (str[pos / 8] >> (7 - pos % 8)) & 1 : 0);
        };

        __attribute__((always_inline))
        bool IsPrefixOf(const InfiniteByteString& other, const uint32_t bits_to_ignore=0) const {
            if (length <= other.length && memcmp(str, other.str, length - 1) == 0)
                return (str[length - 1] | BITMASK(bits_to_ignore)) == (other.str[length - 1] | BITMASK(bits_to_ignore));
            return false;
        }

        bool operator<(const InfiniteByteString& rhs) const {
            int32_t cmp_result = memcmp(str, rhs.str, std::min(length, rhs.length));
            return cmp_result < 0 || (cmp_result == 0 && length < rhs.length);
        }

        bool operator<=(const InfiniteByteString& rhs) const {
            int32_t cmp_result = memcmp(str, rhs.str, std::min(length, rhs.length));
            return cmp_result < 0 || (cmp_result == 0 && length <= rhs.length);
        }

        bool operator==(const InfiniteByteString& rhs) const {
            return length == rhs.length && memcmp(str, rhs.str, std::min(length, rhs.length)) == 0;
        }
    };

    struct InfixStore {
        static const uint32_t size_grade_bit_count = 8;
        static const uint32_t elem_count_bit_count = 20;

        uint32_t status = 0;
        uint64_t *ptr = nullptr;

        InfixStore(const uint32_t slot_count, const uint32_t slot_size, const uint32_t size_grade=0) {
            SetSizeGrade(size_grade);
            const uint32_t word_count = GetPtrWordCount(slot_count, slot_size);
            ptr = new uint64_t[word_count];
            memset(ptr, 0, sizeof(uint64_t) * word_count);
        }
        InfixStore(uint64_t *ptr): status(0), ptr(ptr) {};
        InfixStore() = default;
        InfixStore(const InfixStore &other) = default;
        InfixStore(InfixStore &&other) = default;
        InfixStore &operator=(const InfixStore &other) = default;
        ~InfixStore() = default;

        void Reset(const uint32_t slot_count, const uint32_t slot_size) {
            memset(ptr, 0, GetPtrWordCount(slot_count, slot_size) * sizeof(uint64_t));
        }

        static uint32_t GetPtrWordCount(const uint32_t slot_count, const uint32_t slot_size) {
            return Steroids::infix_store_target_size + (slot_count * (slot_size + 1) + 63) / 64;
        }

        uint32_t GetElemCount() const {
            return status & BITMASK(elem_count_bit_count);
        }

        void SetElemCount(const int32_t elem_count) {
            status &= ~BITMASK(elem_count_bit_count);
            status |= elem_count;
        }

        void UpdateElemCount(const int32_t delta) {
            status += delta;
        }

        uint32_t GetSizeGrade() const {
            return (status >> elem_count_bit_count) & BITMASK(size_grade_bit_count);
        }

        void SetSizeGrade(const uint32_t size_grade) {
            status &= ~(BITMASK(size_grade_bit_count) << elem_count_bit_count);
            status |= size_grade << elem_count_bit_count;
        }

        uint32_t GetInvalidBits() const {
            return status >> (elem_count_bit_count + size_grade_bit_count) & 7U;
        }

        void SetInvalidBits(const uint32_t invalid_bits) {
            status &= ~(7U << (elem_count_bit_count + size_grade_bit_count));
            status |= invalid_bits << (elem_count_bit_count + size_grade_bit_count);
        }

        bool IsPartialKey() const {
            return status >> 31;
        }

        void SetPartialKey(bool val) {
            if (val)
                status |= (1U << 31);
            else 
                status &= ~(1U << 31);
        }
    };

    uint32_t infix_size_;
    wormhole *wh_;
    wormref *better_tree_;
    wormhole_int *wh_int_;
    wormref_int *better_tree_int_;
    std::mt19937 rng_;
    const float load_factor_ = 0.95;
    uint64_t size_scalars_[size_scalar_count], scaled_sizes_[size_scalar_count];
    uint64_t implicit_scalars_[infix_store_target_size / 2 + 1];

    void AddTreeKey(const uint8_t *key, const uint32_t key_len);
    void InsertSimple(const InfiniteByteString key);
    void InsertSplit(const InfiniteByteString key);
    void DeleteMerge(void *const it_inp);
    template <class t_itr>
    void BulkLoadFixedLength(t_itr begin, t_itr end, const uint32_t key_len);
    template <class t_itr>
    void BulkLoad(t_itr begin, t_itr end);
    void SetupScaleFactors();
    std::tuple<uint32_t, uint32_t, uint32_t> 
        GetSharedIgnoreImplicitLengths(const InfiniteByteString key_1,
                                       const InfiniteByteString key_2) const;
    uint64_t ExtractPartialKey(const InfiniteByteString key,
                               const uint32_t shared, const uint32_t ignore,
                               const uint32_t implicit_size, const uint64_t msb) const;

    uint32_t RankOccupieds(const InfixStore &store, const uint32_t pos) const;
    uint32_t SelectRunends(const InfixStore &store, const uint32_t rank) const;
    int32_t NextOccupied(const InfixStore &store, const uint32_t pos) const;
    int32_t PreviousOccupied(const InfixStore &store, const uint32_t pos) const;
    int32_t NextRunend(const InfixStore &store, const uint32_t pos) const;
    int32_t PreviousRunend(const InfixStore &store, const uint32_t pos) const;

    uint64_t GetSlot(const InfixStore &store, const uint32_t pos) const;
    void SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value);
    void SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value, const uint32_t width);

    void ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    void ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    void ShiftRunendsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    void ShiftRunendsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);

    int32_t FindEmptySlotAfter(const InfixStore &store, const uint32_t runend_pos) const;
    int32_t FindEmptySlotBefore(const InfixStore &store, const uint32_t runend_pos) const;
    void InsertRawIntoInfixStore(InfixStore &store, const uint64_t key,
                                 const uint32_t total_implicit=infix_store_target_size);
    void DeleteRawFromInfixStore(InfixStore &store, const uint64_t key,
                                 const uint32_t total_implicit=infix_store_target_size);
    uint32_t GetLongestMatchingInfixSize(const InfixStore &store, const uint64_t key,
                                         const uint32_t total_implicit=infix_store_target_size) const;
    bool RangeQueryInfixStore(InfixStore &store, const uint64_t l_key, const uint64_t r_key,
                              const uint32_t total_implicit=infix_store_target_size) const;
    bool PointQueryInfixStore(InfixStore &store, const uint64_t key,
                              const uint32_t total_implicit=infix_store_target_size) const;
    void ResizeInfixStore(InfixStore &store, const bool expand=true,
                          const uint32_t total_implicit=infix_store_target_size);
    void ShrinkInfixStoreInfixSize(InfixStore &store, const uint32_t new_infix_size);
    void LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                              const uint32_t total_implicit=infix_store_target_size, const bool zero_out=false);
    InfixStore AllocateInfixStoreWithList(const uint64_t *list, const uint32_t list_len,
                                          const uint32_t total_implicit=infix_store_target_size);
    uint32_t GetInfixList(const InfixStore &store, uint64_t *res) const;
    uint32_t GetExpandedInfixListLength(const uint64_t *list, const uint32_t list_len, const uint32_t implicit_size,
                                        const uint32_t shamt, const uint64_t lower_lim, const uint64_t upper_lim);
    void UpdateInfixList(const uint64_t *list, const uint32_t list_len, const uint32_t shamt, 
                         const uint64_t lower_lim, const uint64_t upper_lim,
                         uint64_t *res, const uint32_t res_len);
    void UpdateInfixListDelete(const uint32_t shared, const uint32_t ignore, const uint32_t implicit_size,
                               const InfiniteByteString left_key, const InfiniteByteString right_key,
                               uint64_t *infix_list, const uint32_t infix_list_len);
};


template <bool int_optimized>
inline Steroids<int_optimized>::Steroids(const uint32_t infix_size, const uint32_t rng_seed, const float load_factor):
            wh_(nullptr),
            better_tree_(nullptr),
            wh_int_(nullptr),
            better_tree_int_(nullptr),
            infix_size_(infix_size),
            load_factor_(load_factor) {
    if constexpr (int_optimized) {
        wh_int_ = wh_int_create();
        better_tree_int_ = wh_int_ref(wh_int_);
    }
    else {
        wh_ = wh_create();
        better_tree_ = wh_ref(wh_);
    }
    rng_.seed(rng_seed);
    SetupScaleFactors();
}


template <bool int_optimized>
template <class t_itr>
Steroids<int_optimized>::Steroids(const uint32_t infix_size, t_itr begin, t_itr end, const uint32_t key_len,
         const uint32_t rng_seed, const float load_factor):
        wh_(nullptr),
        better_tree_(nullptr),
        wh_int_(nullptr),
        better_tree_int_(nullptr),
        infix_size_(infix_size),
        load_factor_(load_factor) {
    if constexpr (int_optimized) {
        wh_int_ = wh_int_create();
        better_tree_int_ = wh_int_ref(wh_int_);
    }
    else {
        wh_ = wh_create();
        better_tree_ = wh_ref(wh_);
    }

    rng_.seed(rng_seed);
    SetupScaleFactors();

    uint8_t key[key_len];
    memset(key, 0x00, key_len);
    AddTreeKey(key, key_len);
    memset(key, 0xFF, key_len);
    AddTreeKey(key, key_len);

    BulkLoadFixedLength(begin, end, key_len);
}


template <bool int_optimized>
template <class t_itr>
Steroids<int_optimized>::Steroids(const uint32_t infix_size, t_itr begin, t_itr end, 
         const uint32_t rng_seed, const float load_factor):
        wh_(nullptr),
        better_tree_(nullptr),
        wh_int_(nullptr),
        better_tree_int_(nullptr),
        infix_size_(infix_size),
        load_factor_(load_factor) {
    if constexpr (int_optimized) {
        wh_int_ = wh_int_create();
        better_tree_int_ = wh_int_ref(wh_int_);
    }
    else {
        wh_ = wh_create();
        better_tree_ = wh_ref(wh_);
    }

    rng_.seed(rng_seed);
    SetupScaleFactors();
    BulkLoad(begin, end);
}



template <bool int_optimized>
inline void Steroids<int_optimized>::SetupScaleFactors() {
    double pw = 1.0 / load_factor_;
    for (int32_t i = 0; i < size_scalar_count; i++) {
        size_scalars_[i] = static_cast<uint64_t>(pw * (1ULL << scale_shift));
        scaled_sizes_[i] = infix_store_target_size * size_scalars_[i] >> scale_shift;
        pw /= load_factor_;
    }
    for (int32_t i = 0; i < infix_store_target_size / 2; i++) {
        const double ratio = static_cast<double>(infix_store_target_size) 
                                / static_cast<double>(i + static_cast<double>(infix_store_target_size) / 2);
        implicit_scalars_[i] = static_cast<uint64_t>(ratio * (1ULL << scale_implicit_shift));
    }
    implicit_scalars_[infix_store_target_size / 2] = 1ULL << scale_implicit_shift;
}


template <bool int_optimized>
inline void Steroids<int_optimized>::Insert(std::string_view key) {
    Insert(reinterpret_cast<const uint8_t *>(key.data()), key.size());
}


template <bool int_optimized>
inline void Steroids<int_optimized>::Insert(const uint8_t *key, const uint32_t key_len) {
    const InfiniteByteString converted_key {key, static_cast<uint32_t>(key_len)};

    if (rng_() % infix_store_target_size == 0)
        InsertSplit(converted_key);
    else 
        InsertSimple(converted_key);
}

template <bool int_optimized>
inline void Steroids<int_optimized>::InsertSimple(const InfiniteByteString key) {
    InfixStore *infix_store_ptr, *dummy_infix_store_ptr;
    uint32_t dummy_val;

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, key.str, key.length);

        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                      reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key == key) {
            prev_key = next_key;
            wh_int_iter_skip1(&it_int);
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                          reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
        }
        else {
            wh_int_iter_skip1_rev(&it_int);
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                          reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        }
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, key.str, key.length);

        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                              reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key == key) {
            prev_key = next_key;
            wh_iter_skip1(&it);
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                  reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
        }
        else {
            wh_iter_skip1_rev(&it);
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                  reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        }
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }

    assert(prev_key <= key);
    assert(key < next_key);

    InfixStore& infix_store = *infix_store_ptr;

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);

    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t insertee = ((extraction | 1ULL) - (prev_implicit << infix_size_));
    InsertRawIntoInfixStore(infix_store, insertee, total_implicit);
}


template <bool int_optimized>
inline bool Steroids<int_optimized>::RangeQuery(std::string_view input_l, std::string_view input_r) const {
    return RangeQuery(reinterpret_cast<const uint8_t *>(input_l.data()), input_l.size(),
                      reinterpret_cast<const uint8_t *>(input_r.data()), input_r.size());
}


template <bool int_optimized>
inline bool Steroids<int_optimized>::RangeQuery(const uint8_t *input_l, const uint32_t input_l_len,
                                                const uint8_t *input_r, const uint32_t input_r_len) const {
    const InfiniteByteString l_key {input_l, static_cast<uint32_t>(input_l_len)};
    const InfiniteByteString r_key {input_r, static_cast<uint32_t>(input_r_len)};

    InfixStore *infix_store_ptr;
    uint32_t dummy_val;
    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, l_key.str, l_key.length);

        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                      reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key <= r_key) {
            if (it_int.leaf)
                wormleaf_int_unlock_read(it_int.leaf);
            return true;
        }
        wh_int_iter_skip1_rev(&it_int);
        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                      reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, l_key.str, l_key.length);
        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                              reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key <= r_key) {
            if (it.leaf)
                wormleaf_unlock_read(it.leaf);
            return true;
        }
        wh_iter_skip1_rev(&it);
        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                              reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }
    
    InfixStore& infix_store = *infix_store_ptr;

    if (infix_store.IsPartialKey() && prev_key.IsPrefixOf(l_key, infix_store.GetInvalidBits())) {
        // Previous key was a partial key and a prefix of the left query key
        return true;
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);

    const uint64_t l_extraction = ExtractPartialKey(l_key, shared, ignore, implicit_size, l_key.GetBit(shared));
    const uint64_t r_extraction = ExtractPartialKey(r_key, shared, ignore, implicit_size, r_key.GetBit(shared));
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t l_val = (l_extraction | 1ULL) - (prev_implicit << infix_size_);
    const uint64_t r_val = (r_extraction | 1ULL) - (prev_implicit << infix_size_);
    return RangeQueryInfixStore(infix_store, l_val, r_val, total_implicit);
}


template <bool int_optimized>
inline bool Steroids<int_optimized>::PointQuery(std::string_view key) const {
    return PointQuery(reinterpret_cast<const uint8_t *>(key.data()), key.size());
}


template <bool int_optimized>
inline bool Steroids<int_optimized>::PointQuery(const uint8_t *input_key, const uint32_t key_len) const {
    const InfiniteByteString key {input_key, static_cast<uint32_t>(key_len)};
    
    InfixStore *infix_store_ptr;
    uint32_t dummy_val;

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, input_key, key_len);

        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                      reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key == key) {
            if (it_int.leaf)
                wormleaf_int_unlock_read(it_int.leaf);
            return true;
        }
        wh_int_iter_skip1_rev(&it_int);
        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                      reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, input_key, key_len);

        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                              reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key == key) {
            if (it.leaf)
                wormleaf_unlock_read(it.leaf);
            return true;
        }
        wh_iter_skip1_rev(&it);
        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                              reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }
    
    assert(prev_key <= key);
    assert(key < next_key);

    InfixStore& infix_store = *infix_store_ptr;

    if (infix_store.IsPartialKey() && prev_key.IsPrefixOf(key, infix_store.GetInvalidBits())) {
        // Previous key was a partial key and a prefix of the query key
        return true;
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t query_key = extraction - (prev_implicit << infix_size_);
    return PointQueryInfixStore(infix_store, query_key, total_implicit);
}


template <bool int_optimized>
inline void Steroids<int_optimized>::AddTreeKey(const uint8_t *key, const uint32_t key_len) {
    InfixStore infix_store(scaled_sizes_[0], infix_size_);
    if constexpr (int_optimized)
        wh_int_put(better_tree_int_, key, key_len, &infix_store, sizeof(infix_store));
    else
        wh_put(better_tree_, key, key_len, &infix_store, sizeof(infix_store));
}


template <bool int_optimized>
inline void Steroids<int_optimized>::InsertSplit(InfiniteByteString key) {
    InfixStore *infix_store_ptr, *dummy_infix_store_ptr;
    uint32_t dummy_val;

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, key.str, key.length);

        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                      reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key == key) {
            prev_key = next_key;
            wh_int_iter_skip1(&it_int);
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                          reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
        }
        else {
            wh_int_iter_skip1_rev(&it_int);
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                          reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        }
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, key.str, key.length);

        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                              reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key == key) {
            prev_key = next_key;
            wh_iter_skip1(&it);
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                  reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
        }
        else {
            wh_iter_skip1_rev(&it);
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                  reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        }
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }

    assert(prev_key <= key);
    assert(key < next_key);

    InfixStore& infix_store = *infix_store_ptr;

    if (infix_store.IsPartialKey() && prev_key.IsPrefixOf(key, infix_store.GetInvalidBits())) {
        // Previous key was a partial key and a prefix of the new boundary key
        // Inserting using the simple method...
        InsertSimple(key);
        return;
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);
    uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    uint64_t prev_extraction = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0);
    uint64_t next_extraction = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1);
    const uint64_t separator = (extraction | 1ULL) - (prev_extraction & (BITMASK(implicit_size) << infix_size_));

    const uint32_t infix_list_len = infix_store.GetElemCount();
    uint64_t infix_list[infix_list_len + 1];
    const uint32_t infix_count = GetInfixList(infix_store, infix_list);

    int32_t sep_l = -1, sep_r = infix_count, sep_mid;
    while (sep_r - sep_l > 1) {
        sep_mid = (sep_l + sep_r) / 2;
        const uint64_t val = infix_list[sep_mid] - (infix_list[sep_mid] & -infix_list[sep_mid]);
        if (val <= separator - 1)
            sep_l = sep_mid;
        else
            sep_r = sep_mid;
    }
    uint32_t split_pos = sep_r;
    bool intersecting_range = false;
    int32_t zero_pos = -1;
    for (int32_t i = sep_l; i >= 0 && (infix_list[i] >> infix_size_) == (separator >> infix_size_); i--) {
        const uint64_t mask = ((infix_list[i] & -infix_list[i]) << 1) - 1;
        if ((infix_list[i] | mask) == (separator | mask)) {
            split_pos = i;
            zero_pos = shared + ignore + implicit_size + infix_size_ - lowbit_pos(infix_list[i]) - 1;
        }
    }
    uint32_t copied_key_len = key.length;
    uint8_t copied_key_str[copied_key_len];
    memcpy(copied_key_str, key.str, key.length);
    if (zero_pos != -1 && copied_key_len > (zero_pos - 1) / 8) {
        copied_key_str[(zero_pos - 1) / 8] &= ~BITMASK(7 - (zero_pos - 1) % 8);
        copied_key_len = (zero_pos - 1) / 8 + 1;
    }
    InfiniteByteString edited_key {copied_key_str, copied_key_len};
    if (zero_pos != -1)
        extraction = ExtractPartialKey(edited_key, shared, ignore, implicit_size, edited_key.GetBit(shared));

    const uint32_t shared_word_byte = (shared / 64) * 8;

    auto [shared_lt, ignore_lt, implicit_size_lt] = GetSharedIgnoreImplicitLengths(
            {prev_key.str + shared_word_byte, prev_key.length < shared_word_byte ? 0 : prev_key.length - shared_word_byte},
            {edited_key.str + shared_word_byte, edited_key.length < shared_word_byte ? 0 : edited_key.length - shared_word_byte});
    shared_lt += shared_word_byte * 8;
    const int32_t shamt_lt = shared_lt + ignore_lt + implicit_size_lt - shared - ignore - implicit_size;
    const uint64_t prev_extraction_lt = ExtractPartialKey(prev_key, shared_lt, ignore_lt, implicit_size_lt, 0);
    const uint64_t extraction_lt = ExtractPartialKey(edited_key, shared_lt, ignore_lt, implicit_size_lt, 1);
    const uint64_t left_start = prev_key.BitsAt(shared + ignore + implicit_size, shamt_lt) << infix_size_;
    const uint64_t left_end = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_lt))
                              | (edited_key.BitsAt(shared + ignore + implicit_size, shamt_lt) << infix_size_);
    const uint32_t total_implicit_lt = ((extraction_lt >> infix_size_) - (prev_extraction_lt >> infix_size_)) + 1;

    auto [shared_gt, ignore_gt, implicit_size_gt] = GetSharedIgnoreImplicitLengths(
            {edited_key.str + shared_word_byte, edited_key.length < shared_word_byte ? 0 : edited_key.length - shared_word_byte},
            {next_key.str + shared_word_byte, next_key.length < shared_word_byte ? 0 : next_key.length - shared_word_byte});
    shared_gt += shared_word_byte * 8;
    const int32_t shamt_gt = shared_gt + ignore_gt + implicit_size_gt - shared - ignore - implicit_size;
    const uint64_t extraction_gt = ExtractPartialKey(edited_key, shared_gt, ignore_gt, implicit_size_gt, 0);
    const uint64_t next_extraction_gt = ExtractPartialKey(next_key, shared_gt, ignore_gt, implicit_size_gt, 1);
    const uint64_t right_start = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                | (edited_key.BitsAt(shared + ignore + implicit_size, shamt_gt) << infix_size_);
    const uint64_t right_end = (((next_extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                | (next_key.BitsAt(shared + ignore + implicit_size, shamt_gt) << infix_size_);
    const uint32_t total_implicit_gt = ((next_extraction_gt >> infix_size_) - (extraction_gt >> infix_size_)) + 1;

    const uint32_t left_list_len = GetExpandedInfixListLength(infix_list,
                                                              split_pos,
                                                              implicit_size,
                                                              shamt_lt,
                                                              left_start, left_end);
    uint64_t left_infix_list[left_list_len];
    UpdateInfixList(infix_list, split_pos, shamt_lt,
                    left_start, left_end,
                    left_infix_list, left_list_len);

    const uint32_t right_list_len = GetExpandedInfixListLength(infix_list + split_pos,
                                                               infix_list_len - split_pos,
                                                               implicit_size,
                                                               shamt_gt,
                                                               right_start, right_end);
    uint64_t right_infix_list[right_list_len];
    UpdateInfixList(infix_list + split_pos, infix_list_len - split_pos, shamt_gt,
                    right_start, right_end,
                    right_infix_list, right_list_len);

    InfixStore store_lt = AllocateInfixStoreWithList(left_infix_list,
                                                     left_list_len,
                                                     total_implicit_lt);
    store_lt.SetInvalidBits(infix_store.GetInvalidBits());
    store_lt.SetPartialKey(infix_store.IsPartialKey());
    InfixStore store_gt = AllocateInfixStoreWithList(right_infix_list + (zero_pos != -1),
                                                     right_list_len - (zero_pos != -1),
                                                     total_implicit_gt);
    auto *ptr_to_free = infix_store.ptr;
    if constexpr (int_optimized)
        wh_int_put(better_tree_int_, prev_key.str, prev_key.length, &store_lt, sizeof(InfixStore));
    else 
        wh_put(better_tree_, prev_key.str, prev_key.length, &store_lt, sizeof(InfixStore));
    if (zero_pos != -1) {
        const uint64_t key_extraction = ExtractPartialKey(key, shared_gt, ignore_gt, implicit_size_gt, 0);
        InsertRawIntoInfixStore(store_gt, key_extraction & BITMASK(infix_size_) | 1, total_implicit_gt);
        store_gt.SetInvalidBits(7 - (zero_pos - 1) % 8);
        store_gt.SetPartialKey(true);
        if constexpr (int_optimized)
            wh_int_put(better_tree_int_, edited_key.str, edited_key.length, &store_gt, sizeof(InfixStore));
        else 
            wh_put(better_tree_, edited_key.str, edited_key.length, &store_gt, sizeof(InfixStore));
    }
    else {
        if constexpr (int_optimized)
            wh_int_put(better_tree_int_, key.str, key.length, &store_gt, sizeof(InfixStore));
        else
            wh_put(better_tree_, key.str, key.length, &store_gt, sizeof(InfixStore));
    }

    // No memory leaks!
    delete[] ptr_to_free;
}


template <bool int_optimized>
__attribute__((always_inline))
inline uint32_t Steroids<int_optimized>::GetExpandedInfixListLength(const uint64_t *list, const uint32_t list_len,
                                                                    const uint32_t implicit_size, const uint32_t shamt,
                                                                    const uint64_t lower_lim, const uint64_t upper_lim) {
    uint32_t actual_list_len = list_len;
    const uint64_t lower_implicit_lim = lower_lim >> infix_size_;
    const uint64_t upper_implicit_lim = upper_lim >> infix_size_;
    for (int32_t i = 0; i < list_len; i++) {
        const int32_t new_lowbit_position = lowbit_pos(list[i]) + shamt;
        assert(implicit_size + infix_size_ > new_lowbit_position);
        if (new_lowbit_position >= infix_size_) {
            const uint64_t implicit_part = (list[i] << shamt) >> infix_size_;
            const uint64_t start = implicit_part - (implicit_part & (-implicit_part));
            const uint64_t end = implicit_part | (implicit_part - 1);
            actual_list_len += std::min(end, upper_implicit_lim) - std::max(start, lower_implicit_lim);
        }
    }
    return actual_list_len;
}


template <bool int_optimized>
__attribute__((always_inline))
inline void Steroids<int_optimized>::UpdateInfixList(const uint64_t *list, const uint32_t list_len, const uint32_t shamt,
                                                     const uint64_t lower_lim, const uint64_t upper_lim,
                                                     uint64_t *res, const uint32_t res_len) {
    if (list_len == res_len) {
        for (int32_t i = 0; i < list_len; i++)
            res[i] = (list[i] << shamt) - lower_lim;
        return;
    }

    uint32_t res_ind = 0;
    const uint64_t lower_implicit_lim = lower_lim >> infix_size_;
    const uint64_t upper_implicit_lim = upper_lim >> infix_size_;
    for (int32_t i = 0; i < list_len; i++) {
        const uint64_t val = list[i] << shamt;
        const uint64_t implicit_part = val >> infix_size_;
        const uint64_t explicit_part = val & BITMASK(infix_size_);
        if (explicit_part == 0) {
            assert(implicit_part > 0);
            const uint64_t start = implicit_part - (implicit_part & (-implicit_part));
            const uint64_t end = implicit_part | (implicit_part - 1);
            for (uint64_t j = std::max(start, lower_implicit_lim); j <= std::min(end, upper_implicit_lim); j++)
                res[res_ind++] = ((j - lower_implicit_lim) << infix_size_) | (1ULL << (infix_size_ - 1));
        }
        else
            res[res_ind++] = val - lower_lim;
    }
    assert(res_ind == res_len);
    
    // Use qsort maybe?
    auto comp = [](uint64_t a, uint64_t b) {
                    const uint64_t a_lb = a & (-a), b_lb = b & (-b);
                    const uint64_t a_nolb = a - a_lb;
                    const uint64_t b_nolb = b - b_lb;
                    return a_nolb < b_nolb || (a_nolb == b_nolb && a_lb > b_lb);
                };
    std::sort(res, res + res_ind, comp);
}


template <bool int_optimized>
inline std::tuple<uint32_t, uint32_t, uint32_t> 
Steroids<int_optimized>::GetSharedIgnoreImplicitLengths(const InfiniteByteString key_1,
                                                        const InfiniteByteString key_2) const {
    uint32_t share = 0, ignore = 0, implicit = 0;

    uint32_t ind = 0, delta;
    do {
        const uint64_t read_1 = key_1.WordAt(ind * sizeof(uint64_t));
        const uint64_t read_2 = key_2.WordAt(ind * sizeof(uint64_t));
        delta = __builtin_ia32_lzcnt_u64(read_1 ^ read_2);
        share += delta;
        ind++;
    } while (delta == 64);

    ind--;
    do {
        const uint64_t read_1 = key_1.WordAt(ind * sizeof(uint64_t));
        const uint64_t read_2 = key_2.WordAt(ind * sizeof(uint64_t));
        const uint32_t offset = (ind > share / 64 ? 0 : share % 64 + 1);
        delta = __builtin_ia32_lzcnt_u64(((~read_1) | read_2) & BITMASK(64 - offset));
        ignore += delta - offset;
        ind++;
    } while (delta == 64);

    uint64_t implicit_size = base_implicit_size;
    const uint64_t implicit_1 = key_1.BitsAt(share + ignore + 1, base_implicit_size - 1);
    const uint64_t implicit_2 = (1ULL << (base_implicit_size - 1)) | key_2.BitsAt(share + ignore + 1, base_implicit_size - 1);
    implicit_size += (2 * (implicit_2 - implicit_1 + 1) < (1ULL << base_implicit_size));

    return {share, ignore, implicit_size};
}


template <bool int_optimized>
inline void Steroids<int_optimized>::ShrinkInfixSize(const uint32_t new_infix_size) {
    InfixStore *store_ptr;
    const uint8_t *key;
    uint32_t key_len, dummy_val;

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, nullptr, 0);
        do {
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&key), &key_len,
                                          reinterpret_cast<void **>(&store_ptr), &dummy_val);
            ShrinkInfixStoreInfixSize(*store_ptr, new_infix_size);
            wh_int_iter_skip1(&it_int);
        } while (wh_int_iter_valid(&it_int));
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, nullptr, 0);
        do {
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&key), &key_len,
                                  reinterpret_cast<void **>(&store_ptr), &dummy_val);
            ShrinkInfixStoreInfixSize(*store_ptr, new_infix_size);
            wh_iter_skip1(&it);
        } while (wh_iter_valid(&it));
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }

    infix_size_ = new_infix_size;
}


template <bool int_optimized>
inline uint32_t Steroids<int_optimized>::Size() const {
    uint32_t res = 0;
    const uint8_t *tree_key;
    uint32_t tree_key_len, dummy;
    InfixStore *store;

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        for (wh_int_iter_seek(&it_int, nullptr, 0); wh_int_iter_valid(&it_int); wh_int_iter_skip1(&it_int)) {
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key_len), &tree_key_len, 
                                          reinterpret_cast<void **>(&store), &dummy);
            res += (scaled_sizes_[store->GetSizeGrade()] * (1 + infix_size_) + infix_store_target_size + 7) / 8;
        }
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        for (wh_iter_seek(&it, nullptr, 0); wh_iter_valid(&it); wh_iter_skip1(&it)) {
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key_len), &tree_key_len, 
                                  reinterpret_cast<void **>(&store), &dummy);
            res += (scaled_sizes_[store->GetSizeGrade()] * (1 + infix_size_) + infix_store_target_size + 7) / 8;
        }
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }
    return res;
}


template <bool int_optimized>
__attribute__((always_inline))
inline uint64_t Steroids<int_optimized>::ExtractPartialKey(const InfiniteByteString key,
                                                           const uint32_t shared, const uint32_t ignore,
                                                           const uint32_t implicit_size, const uint64_t msb) const {
    const uint32_t real_diff_pos = shared + ignore;
    uint64_t res = key.WordAt(real_diff_pos / 8);
    res >>= (63 - (implicit_size - 1) - infix_size_ - real_diff_pos % 8);
    res &= BITMASK(implicit_size - 1 + infix_size_);
    res |= msb << (implicit_size - 1 + infix_size_);
    return res;
}


template <bool int_optimized>
inline void Steroids<int_optimized>::Delete(std::string_view input_key) {
    Delete(reinterpret_cast<const uint8_t *>(input_key.data()), input_key.size());
}


template <bool int_optimized>
inline void Steroids<int_optimized>::Delete(const uint8_t *input_key, const uint32_t input_key_len) {
    InfiniteByteString key {input_key, input_key_len};

    InfixStore *infix_store_ptr, *dummy_infix_store_ptr;
    uint32_t dummy_val;

    InfiniteByteString next_key {};
    InfiniteByteString prev_key {};

    wormhole_int_iter it_int;
    wormhole_iter it;
    if constexpr (int_optimized) {
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, key.str, key.length);

        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                      reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key == key) {
            if (!infix_store_ptr->IsPartialKey()) {
                DeleteMerge(&it_int);
                return;
            }
            prev_key = next_key;
            wh_int_iter_skip1(&it_int);
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                          reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
            wh_int_iter_skip1_rev(&it_int);
        }
        else {
            wh_int_iter_skip1_rev(&it_int);
            wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                          reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        }
    }
    else {
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, key.str, key.length);

        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                              reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        if (next_key == key) {
            if (!infix_store_ptr->IsPartialKey()) {
                DeleteMerge(&it);
                return;
            }
            prev_key = next_key;
            wh_iter_skip1(&it);
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&next_key.str), &next_key.length,
                                  reinterpret_cast<void **>(&dummy_infix_store_ptr), &dummy_val);
            wh_iter_skip1_rev(&it);
        }
        else {
            wh_iter_skip1_rev(&it);
            wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&prev_key.str), &prev_key.length,
                                  reinterpret_cast<void **>(&infix_store_ptr), &dummy_val);
        }
    }

    assert(prev_key <= key);
    assert(key < next_key);

    InfixStore& infix_store = *infix_store_ptr;
    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(prev_key, next_key);

    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t deletee = ((extraction | 1ULL) - (prev_implicit << infix_size_));

    if (infix_store.IsPartialKey()) {
        const uint32_t longest_match_len = GetLongestMatchingInfixSize(infix_store, deletee);
        if (longest_match_len == 0 || 8 * prev_key.length - infix_store.GetInvalidBits() 
                                        > shared + ignore + implicit_size + longest_match_len - 1) {
            if constexpr (int_optimized)
                DeleteMerge(&it_int);
            else
                DeleteMerge(&it);
            return;
        }
    }

    if constexpr (int_optimized) {
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }

    DeleteRawFromInfixStore(infix_store, deletee, total_implicit);
}


template <bool int_optimized>
inline void Steroids<int_optimized>::DeleteMerge(void *const it_inp) {

    InfiniteByteString middle_key {};
    InfiniteByteString left_key {};
    InfiniteByteString right_key {};
    InfixStore *store_l, *store_r;
    uint32_t dummy;

    if constexpr (int_optimized) {
        wormhole_int_iter *const it_int = reinterpret_cast<wormhole_int_iter *const>(it_inp);
        wh_int_iter_peek_ref(it_int, reinterpret_cast<const void **>(&middle_key.str), &middle_key.length, 
                                     reinterpret_cast<void **>(&store_r), &dummy);
        wh_int_iter_skip1(it_int);
        wh_int_iter_peek_ref(it_int, reinterpret_cast<const void **>(&right_key.str), &right_key.length, 
                                     reinterpret_cast<void **>(&store_l), &dummy);
        wh_int_iter_skip1_rev(it_int);
        wh_int_iter_skip1_rev(it_int);
        wh_int_iter_peek_ref(it_int, reinterpret_cast<const void **>(&left_key.str), &left_key.length, 
                                     reinterpret_cast<void **>(&store_l), &dummy);
        if (it_int->leaf)
            wormleaf_int_unlock_read(it_int->leaf);
    }
    else {
        wormhole_iter *const it = reinterpret_cast<wormhole_iter *const>(it_inp);
        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&middle_key.str), &middle_key.length, 
                             reinterpret_cast<void **>(&store_r), &dummy);
        wh_iter_skip1(it);
        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&right_key.str), &right_key.length, 
                             reinterpret_cast<void **>(&store_l), &dummy);
        wh_iter_skip1_rev(it);
        wh_iter_skip1_rev(it);
        wh_iter_peek_ref(it, reinterpret_cast<const void **>(&left_key.str), &left_key.length, 
                             reinterpret_cast<void **>(&store_l), &dummy);
        if (it->leaf)
            wormleaf_unlock_read(it->leaf);
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);

    uint32_t total_elem_count = store_l->GetElemCount() + store_r->GetElemCount();
    uint64_t infix_list[total_elem_count];
    GetInfixList(*store_l, infix_list);
    GetInfixList(*store_r, infix_list + store_l->GetElemCount());

    UpdateInfixListDelete(shared, ignore, implicit_size, left_key, middle_key, infix_list, store_l->GetElemCount());
    UpdateInfixListDelete(shared, ignore, implicit_size, middle_key, right_key, infix_list + store_l->GetElemCount(), store_r->GetElemCount());
    const uint64_t implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
    for (int32_t i = 0; i < total_elem_count; i++)
        infix_list[i] -= implicit << infix_size_;

    for (int32_t i = 1; i < total_elem_count; i++) {
        const uint64_t prev_l = infix_list[i - 1] - (infix_list[i - 1] & -infix_list[i - 1]);
        const uint64_t cur_l = infix_list[i] - (infix_list[i] & -infix_list[i]);
        assert(prev_l <= cur_l);
    }

    delete[] store_l->ptr;
    delete[] store_r->ptr;
    if constexpr (int_optimized)
        wh_int_del(better_tree_int_, middle_key.str, middle_key.length);
    else
        wh_del(better_tree_, middle_key.str, middle_key.length);

    const uint64_t left_extraction = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0);
    const uint64_t right_extraction = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1);
    const uint32_t total_implicit = ((right_extraction >> infix_size_) - (left_extraction >> infix_size_)) + 1;

    InfixStore store = AllocateInfixStoreWithList(infix_list, total_elem_count, total_implicit);
    store.SetPartialKey(store_l->IsPartialKey());
    store.SetInvalidBits(store_l->GetInvalidBits());
    if constexpr (int_optimized)
        wh_int_put(better_tree_int_, left_key.str, left_key.length, reinterpret_cast<const void *>(&store), sizeof(InfixStore));
    else
        wh_put(better_tree_, left_key.str, left_key.length, reinterpret_cast<const void *>(&store), sizeof(InfixStore));
}

template <bool int_optimized>
inline void Steroids<int_optimized>::UpdateInfixListDelete(const uint32_t shared, const uint32_t ignore, const uint32_t implicit_size,
                                                           const InfiniteByteString left_key, const InfiniteByteString right_key,
                                                           uint64_t *infix_list, const uint32_t infix_list_len) {
    const uint32_t shared_word_byte = (shared / 64) * 8;

    auto [old_shared, old_ignore, old_implicit_size] = GetSharedIgnoreImplicitLengths(
            {left_key.str + shared_word_byte, left_key.length < shared_word_byte ? 0 : left_key.length - shared_word_byte},
            {right_key.str + shared_word_byte, right_key.length < shared_word_byte ? 0 : right_key.length - shared_word_byte});
    old_shared += shared_word_byte * 8;

    const uint64_t old_left_implicit = ExtractPartialKey(left_key, old_shared, old_ignore,
                                                         old_implicit_size, 0) >> infix_size_;
    const uint32_t old_infix_size = old_implicit_size + infix_size_;
    const uint32_t new_infix_size = implicit_size + infix_size_;
    if (old_shared == shared) {
        for (int32_t i = 0; i < infix_list_len; i++) {
            infix_list[i] += old_left_implicit << infix_size_;
            const uint64_t old_diff_bit = infix_list[i] >> (old_infix_size - 1);
            infix_list[i] &= BITMASK(old_infix_size - 1);
            infix_list[i] = (new_infix_size > old_infix_size ? infix_list[i] << (new_infix_size - old_infix_size)
                                                             : (infix_list[i] >> (old_infix_size - new_infix_size))
                                                                    | (infix_list[i] & 1ULL));
            uint32_t recovered_bit_cnt = 0, recovery_bits = 1;
            recovered_bit_cnt += recovery_bits;
            uint64_t recovered_infix = old_diff_bit << (new_infix_size - recovered_bit_cnt);
            recovery_bits = std::min(old_ignore - ignore, new_infix_size - recovered_bit_cnt);
            recovered_bit_cnt += recovery_bits;
            recovered_infix |= (((1ULL << recovery_bits) - (1ULL ^ old_diff_bit)) & BITMASK(recovery_bits))
                                    << (new_infix_size - recovered_bit_cnt);
            if (recovered_bit_cnt < new_infix_size) {
                recovered_infix |= infix_list[i] >> (recovered_bit_cnt - 1);
                recovered_infix |= (lowbit_pos(infix_list[i]) < recovered_bit_cnt - 1 ? 1ULL : 0ULL);
            }
            else 
                recovered_infix |= 1ULL;
            assert(recovered_bit_cnt <= new_infix_size);
            infix_list[i] = recovered_infix;
        }
    }
    else {
        for (int32_t i = 0; i < infix_list_len; i++) {
            infix_list[i] += old_left_implicit << infix_size_;
            const uint64_t old_diff_bit = infix_list[i] >> (old_infix_size - 1);
            infix_list[i] &= BITMASK(old_infix_size - 1);
            infix_list[i] = (new_infix_size > old_infix_size ? infix_list[i] << (new_infix_size - old_infix_size)
                                                             : (infix_list[i] >> (old_infix_size - new_infix_size))
                                                                    | (infix_list[i] & 1ULL));
            uint32_t recovered_bit_cnt = 1, recovery_bits = 1;
            uint64_t recovered_infix = left_key.GetBit(shared) << (new_infix_size - recovered_bit_cnt);
            
            recovery_bits = std::min(old_shared - shared - ignore - 1, new_infix_size - recovered_bit_cnt);
            recovered_bit_cnt += recovery_bits;
            recovered_infix |= left_key.BitsAt(shared + ignore + 1, recovery_bits)
                                    << (new_infix_size - recovered_bit_cnt);
            if (recovered_bit_cnt < new_infix_size) {
                recovered_infix |= old_diff_bit << (new_infix_size - recovered_bit_cnt - 1);
                recovery_bits = std::min(old_ignore + 1, new_infix_size - recovered_bit_cnt);
                recovered_bit_cnt += recovery_bits;
                recovered_infix |= (recovery_bits > 1 ? (((1ULL << (recovery_bits - 1)) - (1ULL ^ old_diff_bit)) 
                                                            & BITMASK(recovery_bits - 1)) 
                                                                << (new_infix_size - recovered_bit_cnt)
                                                      : 0ULL);
            }
            if (recovered_bit_cnt < new_infix_size) {
                recovered_infix |= infix_list[i] >> (recovered_bit_cnt - 1);
                recovered_infix |= (lowbit_pos(infix_list[i]) < recovered_bit_cnt - 1 ? 1ULL : 0ULL);
            }
            else 
                recovered_infix |= 1ULL;
            assert(recovered_bit_cnt <= new_infix_size);
            infix_list[i] = recovered_infix;
        }
    }
}


template <bool int_optimized>
template <class t_itr>
inline void Steroids<int_optimized>::BulkLoadFixedLength(t_itr begin, t_itr end, const uint32_t key_len) {
    uint64_t infix_list[infix_store_target_size];
    t_itr last_key_it = begin, key_it = begin;
    InfiniteByteString left_key {reinterpret_cast<const uint8_t *>(&(*key_it)), key_len};
    InfiniteByteString right_key {};
    AddTreeKey(left_key.str, left_key.length);
    int32_t cnt = 1;
    for (++key_it; key_it != end; ++key_it) {
        if (cnt % infix_store_target_size == 0) {   // New boundary key
            right_key = {reinterpret_cast<const uint8_t *>(&(*key_it)), key_len};
            AddTreeKey(right_key.str, right_key.length);

            InfixStore *store;
            const uint8_t *tree_key;
            uint32_t tree_key_len, dummy;

            if constexpr (int_optimized) {
                wormhole_int_iter it_int;
                it_int.ref = better_tree_int_;
                it_int.map = better_tree_int_->map;
                it_int.leaf = nullptr;
                it_int.is = 0;
                wh_int_iter_seek(&it_int, left_key.str, left_key.length);
                wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key), &tree_key_len,
                                              reinterpret_cast<void **>(&store), &dummy);
                if (it_int.leaf)
                    wormleaf_int_unlock_read(it_int.leaf);
            }
            else {
                wormhole_iter it;
                it.ref = better_tree_;
                it.map = better_tree_->map;
                it.leaf = nullptr;
                it.is = 0;
                wh_iter_seek(&it, left_key.str, left_key.length);
                wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key), &tree_key_len,
                                      reinterpret_cast<void **>(&store), &dummy);
                if (it.leaf)
                    wormleaf_unlock_read(it.leaf);
            }

            auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
            const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
            const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
            const uint32_t total_implicit = next_implicit - prev_implicit + 1;
            ++last_key_it;
            for (int32_t i = 0; i < infix_store_target_size - 1; i++) {
                const InfiniteByteString key {reinterpret_cast<const uint8_t *>(&(*last_key_it)), key_len};
                const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
                infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
                ++last_key_it;
            }
            LoadListToInfixStore(*store, infix_list, infix_store_target_size - 1, total_implicit);
            left_key = right_key;
        }
        cnt++;
    }

    // Add what was left from the loop
    uint8_t min_str[key_len], max_str[key_len];
    memset(min_str, 0, key_len);
    memset(max_str, 0xFF, key_len);
    AddTreeKey(min_str, key_len);
    AddTreeKey(max_str, key_len);

    right_key = {max_str, key_len};

    InfixStore *store;
    const uint8_t *tree_key;
    uint32_t tree_key_len, dummy;

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, left_key.str, left_key.length);
        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key), &tree_key_len,
                                      reinterpret_cast<void **>(&store), &dummy);
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, left_key.str, left_key.length);
        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key), &tree_key_len,
                              reinterpret_cast<void **>(&store), &dummy);
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
    const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    int32_t i = 0;
    ++last_key_it;
    while (last_key_it != key_it) {
        const InfiniteByteString key {reinterpret_cast<const uint8_t *>(&(*last_key_it)), key_len};
        const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
        infix_list[i++] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
        ++last_key_it;
    }
    if (i)
        LoadListToInfixStore(*store, infix_list, i, total_implicit);
}


template <bool int_optimized>
template <class t_itr>
inline void Steroids<int_optimized>::BulkLoad(t_itr begin, t_itr end) {
    uint64_t infix_list[infix_store_target_size];
    t_itr last_key_it = begin, key_it = begin;
    std::string_view sv {*key_it};
    InfiniteByteString left_key {reinterpret_cast<const uint8_t *>(sv.data()), 
                                 static_cast<uint32_t>(sv.size())};
    InfiniteByteString right_key {};
    AddTreeKey(left_key.str, left_key.length);
    int32_t cnt = 1;
    uint32_t max_len = sv.size();
    for (++key_it; key_it != end; ++key_it) {
        if (cnt % infix_store_target_size == 0) {   // New boundary key
            std::string_view sv = *key_it;
            right_key = {reinterpret_cast<const uint8_t *>(sv.data()), 
                         static_cast<uint32_t>(sv.size())};
            AddTreeKey(right_key.str, right_key.length);

            InfixStore *store;
            const uint8_t *tree_key;
            uint32_t tree_key_len, dummy;

            if constexpr (int_optimized) {
                wormhole_int_iter it_int;
                it_int.ref = better_tree_int_;
                it_int.map = better_tree_int_->map;
                it_int.leaf = nullptr;
                it_int.is = 0;
                wh_int_iter_seek(&it_int, left_key.str, left_key.length);
                wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key), &tree_key_len,
                                              reinterpret_cast<void **>(&store), &dummy);
                if (it_int.leaf)
                    wormleaf_int_unlock_read(it_int.leaf);
            }
            else {
                wormhole_iter it;
                it.ref = better_tree_;
                it.map = better_tree_->map;
                it.leaf = nullptr;
                it.is = 0;
                wh_iter_seek(&it, left_key.str, left_key.length);
                wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key), &tree_key_len,
                                      reinterpret_cast<void **>(&store), &dummy);
                if (it.leaf)
                    wormleaf_unlock_read(it.leaf);
            }

            auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
            const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
            const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
            const uint32_t total_implicit = next_implicit - prev_implicit + 1;
            ++last_key_it;
            for (int32_t i = 0; i < infix_store_target_size - 1; i++) {
                sv = *last_key_it;
                const InfiniteByteString key {reinterpret_cast<const uint8_t *>(sv.data()), 
                                              static_cast<uint32_t>(sv.size())};
                const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
                infix_list[i] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
                ++last_key_it;
            }
            LoadListToInfixStore(*store, infix_list, infix_store_target_size - 1, total_implicit);
            left_key = right_key;
        }
        sv = *key_it;
        max_len = std::max<uint32_t>(max_len, sv.size());
        cnt++;
    }

    // Add what was left from the loop
    uint8_t min_str[max_len], max_str[max_len];
    memset(min_str, 0, max_len);
    memset(max_str, 0xFF, max_len);
    AddTreeKey(min_str, max_len);
    AddTreeKey(max_str, max_len);

    right_key = {max_str, max_len};

    InfixStore *store;
    const uint8_t *tree_key;
    uint32_t tree_key_len, dummy;

    if constexpr (int_optimized) {
        wormhole_int_iter it_int;
        it_int.ref = better_tree_int_;
        it_int.map = better_tree_int_->map;
        it_int.leaf = nullptr;
        it_int.is = 0;
        wh_int_iter_seek(&it_int, left_key.str, left_key.length);
        wh_int_iter_peek_ref(&it_int, reinterpret_cast<const void **>(&tree_key), &tree_key_len,
                                      reinterpret_cast<void **>(&store), &dummy);
        if (it_int.leaf)
            wormleaf_int_unlock_read(it_int.leaf);
    }
    else {
        wormhole_iter it;
        it.ref = better_tree_;
        it.map = better_tree_->map;
        it.leaf = nullptr;
        it.is = 0;
        wh_iter_seek(&it, left_key.str, left_key.length);
        wh_iter_peek_ref(&it, reinterpret_cast<const void **>(&tree_key), &tree_key_len,
                              reinterpret_cast<void **>(&store), &dummy);
        if (it.leaf)
            wormleaf_unlock_read(it.leaf);
    }

    auto [shared, ignore, implicit_size] = GetSharedIgnoreImplicitLengths(left_key, right_key);
    const uint64_t prev_implicit = ExtractPartialKey(left_key, shared, ignore, implicit_size, 0) >> infix_size_;
    const uint64_t next_implicit = ExtractPartialKey(right_key, shared, ignore, implicit_size, 1) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    int32_t i = 0;
    ++last_key_it;
    while (last_key_it != key_it) {
        sv = *last_key_it;
        const InfiniteByteString key {reinterpret_cast<const uint8_t *>(sv.data()), 
                                      static_cast<uint32_t>(sv.size())};
        const uint64_t extraction = ExtractPartialKey(key, shared, ignore, implicit_size, key.GetBit(shared));
        infix_list[i++] = ((extraction | 1ULL) - (prev_implicit << infix_size_));
        ++last_key_it;
    }
    if (i)
        LoadListToInfixStore(*store, infix_list, i, total_implicit);
}


template <bool int_optimized>
__attribute__((always_inline))
inline uint32_t Steroids<int_optimized>::RankOccupieds(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr;
    uint32_t res = 0;
    for (int32_t i = 0; i < pos / 64; i++)
        res += __builtin_popcountll(occupieds[i]);
    return res + bit_rank(occupieds[pos / 64], pos % 64);
}


template <bool int_optimized>
__attribute__((always_inline))
inline uint32_t Steroids<int_optimized>::SelectRunends(const InfixStore &store, const uint32_t rank) const {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t total_words = (scaled_sizes_[size_grade] + 63) / 64;
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    uint32_t i = 0, old_total_set_bits = 0, total_set_bits = 0;
    while (total_set_bits <= rank && i < total_words) {
        old_total_set_bits = total_set_bits;
        total_set_bits += __builtin_popcountll(runends[i]);
        i++;
    }
    i--;
    return i * 64 + bit_select(runends[i], rank - old_total_set_bits);
}


template <bool int_optimized>
inline int32_t Steroids<int_optimized>::NextOccupied(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr;
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(occupieds[res / 64] & (~BITMASK(res % 64)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < infix_store_target_size);
    return res;
}


template <bool int_optimized>
__attribute__((always_inline))
inline int32_t Steroids<int_optimized>::PreviousOccupied(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr;
    int32_t res = pos - 1, hb_pos;
    do {
        const int32_t offset = res % 64;
        hb_pos = highbit_pos(occupieds[res / 64] & BITMASK(offset + 1));
        res += (hb_pos == -1 ? -offset - 1 : hb_pos - offset);
    } while (hb_pos == -1 && res >= 0);
    assert(res >= -1);
    return res;
}


template <bool int_optimized>
__attribute__((always_inline))
inline int32_t Steroids<int_optimized>::NextRunend(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t runends_size = scaled_sizes_[size_grade];
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(runends[res / 64] & (~BITMASK(res % 64)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < runends_size);
    return res;
}


template <bool int_optimized>
__attribute__((always_inline))
inline int32_t Steroids<int_optimized>::PreviousRunend(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    int32_t res = pos - 1, hb_pos;
    do {
        const int32_t offset = res % 64;
        hb_pos = highbit_pos(runends[res / 64] & BITMASK(offset + 1));
        res += (hb_pos == -1 ? -offset - 1 : hb_pos - offset);
    } while (hb_pos == -1 && res >= 0);
    assert(res >= -1);
    return res;
}


template <bool int_optimized>
__attribute__((always_inline))
inline uint64_t Steroids<int_optimized>::GetSlot(const InfixStore &store, const uint32_t pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + pos * infix_size_;
    const uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t value;
    memcpy(&value, ptr, sizeof(value));
    return (value >> bit_pos % 8) & BITMASK(infix_size_);
}


template <bool int_optimized>
__attribute__((always_inline))
inline void Steroids<int_optimized>::SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + pos * infix_size_;
    uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t stamp;
    memcpy(&stamp, ptr, sizeof(stamp));
    stamp &= ~(BITMASK(infix_size_) << (bit_pos % 8));
    stamp |= value << (bit_pos % 8);
    memcpy(ptr, &stamp, sizeof(stamp));
}


template <bool int_optimized>
__attribute__((always_inline))
inline void Steroids<int_optimized>::SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value, const uint32_t width) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + pos * width;
    uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t stamp;
    memcpy(&stamp, ptr, sizeof(stamp));
    stamp &= ~(BITMASK(width) << (bit_pos % 8));
    stamp |= value << (bit_pos % 8);
    memcpy(ptr, &stamp, sizeof(stamp));
}


template <bool int_optimized>
__attribute__((always_inline))
inline void Steroids<int_optimized>::ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                     const uint32_t shamt) {
#ifdef NAIVE_SLOT_SHIFT
    for (int32_t i = r - 1; i >= l; i--)
        SetSlot(store, i + shamt, GetSlot(store, i));
    for (int32_t i = l; i < l + shamt; i++)
        SetSlot(store, i, 0ULL);
#else
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    shift_bitmap_right(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
#endif
}


template <bool int_optimized>
__attribute__((always_inline))
inline void Steroids<int_optimized>::ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                    const uint32_t shamt) {
#ifdef NAIVE_SLOT_SHIFT
    for (int32_t i = l; i < r; i--)
        SetSlot(store, i - shamt, GetSlot(store, i));
#else
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t l_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    shift_bitmap_left(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
#endif
}


template <bool int_optimized>
__attribute__((always_inline))
inline void Steroids<int_optimized>::ShiftRunendsRight(const InfixStore &store, const uint32_t l, const uint32_t r, 
                                                       const uint32_t shamt) {
    shift_bitmap_right(store.ptr + infix_store_target_size / 64, l, r - 1, shamt);
}


template <bool int_optimized>
__attribute__((always_inline))
inline void Steroids<int_optimized>::ShiftRunendsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                                      const uint32_t shamt) {
    shift_bitmap_left(store.ptr + infix_store_target_size / 64, l, r - 1, shamt);
}


template <bool int_optimized>
__attribute__((always_inline))
inline int32_t Steroids<int_optimized>::FindEmptySlotAfter(const InfixStore &store, const uint32_t runend_pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    int32_t current_pos = runend_pos;
    while (current_pos < scaled_sizes_[size_grade] && GetSlot(store, current_pos + 1)) {
        current_pos = NextRunend(store, current_pos);
    }
    return current_pos + 1;
}


template <bool int_optimized>
__attribute__((always_inline))
inline int32_t Steroids<int_optimized>::FindEmptySlotBefore(const InfixStore &store, const uint32_t runend_pos) const {
    int32_t current_pos = runend_pos, previous_pos;
    do {
        previous_pos = current_pos;
        current_pos = PreviousRunend(store, current_pos);
    } while (current_pos >= 0 && GetSlot(store, current_pos + 1));

    do {
        previous_pos--;
    } while (current_pos < previous_pos && GetSlot(store, previous_pos));
    return previous_pos;

    // Maybe binary searching would be better?
    int32_t l = current_pos, r = previous_pos, mid;
    while (r - l > 1) {
        mid = (l + r) / 2;
        const bool cond = GetSlot(store, mid) == 0;
        l = cond ? mid : l;
        r = cond ? r : mid;
    }
    return l;
}


template <bool int_optimized>
inline void Steroids<int_optimized>::InsertRawIntoInfixStore(InfixStore &store, const uint64_t key, const uint32_t total_implicit) {
    uint32_t size_grade = store.GetSizeGrade();
    const uint32_t elem_count = store.GetElemCount();
    if (elem_count >= (size_grade ? scaled_sizes_[size_grade - 1] : infix_store_target_size)) {
        assert(size_grade < size_scalar_count);
        ResizeInfixStore(store, true, total_implicit);
        size_grade++;
    }

    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    assert(implicit_part < total_implicit);

    uint64_t *occupieds = store.ptr;
    uint64_t *runends = store.ptr + infix_store_target_size / 64;

    const uint32_t mapped_pos = (implicit_part * size_scalars_[size_grade] * implicit_scalar) 
                                        >> (scale_shift + scale_implicit_shift);
    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const bool is_occupied = get_bitmap_bit(occupieds, implicit_part);
    if (!is_occupied && GetSlot(store, mapped_pos) == 0) {
        SetSlot(store, mapped_pos, explicit_part);
        set_bitmap_bit(runends, mapped_pos);
    }
    else if (is_occupied) {
        const int32_t runend_pos = SelectRunends(store, key_rank);
        const int32_t next_empty = FindEmptySlotAfter(store, mapped_pos);
        const int32_t previous_empty = FindEmptySlotBefore(store, mapped_pos);

        int32_t l = std::max(PreviousRunend(store, runend_pos), previous_empty);
        int32_t r = runend_pos + 1;
        int32_t mid;
        while (r - l > 1) {
            mid = (l + r) / 2;
            const uint64_t range_l = GetSlot(store, mid);
            const bool cond = (range_l - (range_l & -range_l)) <= explicit_part - 1;
            l = cond ? mid : l;
            r = cond ? r : mid;
        }
        if (next_empty < scaled_sizes_[size_grade]) {
            ShiftSlotsRight(store, r, next_empty, 1);
            ShiftRunendsRight(store, runend_pos, next_empty, 1);
            SetSlot(store, r, explicit_part);
        }
        else {
            ShiftSlotsLeft(store, previous_empty + 1, r, 1);
            ShiftRunendsLeft(store, previous_empty + 1, std::min(runend_pos, r), 1);
            SetSlot(store, r - 1, explicit_part);
        }
    }
    else {
        const int32_t runend_pos = key_rank == 0 ? -1 : SelectRunends(store, key_rank - 1);
        const int32_t next_empty = FindEmptySlotAfter(store, mapped_pos);
        if (next_empty < scaled_sizes_[size_grade]) {
            ShiftSlotsRight(store, runend_pos + 1, next_empty, 1);
            ShiftRunendsRight(store, runend_pos + 1, next_empty, 1);
            SetSlot(store, runend_pos + 1, explicit_part);
            set_bitmap_bit(runends, runend_pos + 1);
        }
        else {
            const int32_t previous_empty = FindEmptySlotBefore(store, mapped_pos);
            const int32_t target_pos = std::max(runend_pos, previous_empty);
            ShiftSlotsLeft(store, previous_empty + 1, target_pos + 1, 1);
            ShiftRunendsLeft(store, previous_empty + 1, target_pos + 1, 1);
            SetSlot(store, target_pos, explicit_part);
            set_bitmap_bit(runends, target_pos);
        }
    }
    set_bitmap_bit(occupieds, implicit_part);
    store.UpdateElemCount(1);

    uint32_t occupied_count = 0, runend_count = 0;
    for (int32_t i = 0; i < infix_store_target_size / 64; i++)
        occupied_count += __builtin_popcountll(occupieds[i]);
    for (int32_t i = 0; i < scaled_sizes_[size_grade]; i++)
        runend_count += get_bitmap_bit(runends, i);
    assert(occupied_count == runend_count);
}


template <bool int_optimized>
inline void Steroids<int_optimized>::DeleteRawFromInfixStore(InfixStore &store, const uint64_t key, const uint32_t total_implicit) {
    uint32_t size_grade = store.GetSizeGrade();
    const uint32_t elem_count = store.GetElemCount();
    if (size_grade > 0 && elem_count <= (size_grade > 1 ? scaled_sizes_[size_grade - 2] : infix_store_target_size)) {
        ResizeInfixStore(store, true, total_implicit);
        size_grade--;
    }

    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    uint64_t *occupieds = store.ptr;
    const bool is_occupied = get_bitmap_bit(occupieds, implicit_part);
    assert(is_occupied);

    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const int32_t runend_pos = SelectRunends(store, key_rank);
    const int32_t runstart_pos = std::max(key_rank ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1,
                                          static_cast<int32_t>(FindEmptySlotBefore(store, runend_pos))) + 1;
    const bool run_destroyed = runstart_pos == runend_pos;

    int32_t l = runstart_pos - 1, r = runend_pos + 1, mid;
    while (r - l > 1) {
        mid = (l + r) / 2;
        uint64_t value = GetSlot(store, mid);
        value -= value & -value;
        if (value <= key - 1)
            l = mid;
        else 
            r = mid;
    }
    int32_t match_pos;
    for (match_pos = l; match_pos >= runstart_pos; match_pos--) {
        const uint64_t value = GetSlot(store, match_pos);
        const uint64_t mask = ((value & -value) << 1) - 1;
        if ((value | mask) == (explicit_part | mask))
            break;
    }

    // TODO: Perhaps I can avoid shifting the cluster by changing the inserts?
    // But that may slow down inserts...
    
    bool found_empty_right = false;     // If we find an empty to the right, we
                                        // can be sure we have to shift to the
                                        // left. But if we don't, we have to do
                                        // further checking.
    int32_t cur_occupied = implicit_part;
    int32_t cur_runend = runend_pos, prev_runend;
    int32_t shift_start = -1, shift_end = runend_pos;
    while (cur_runend < scaled_sizes_[size_grade]) {
        // Find last run that starts in its canonical slot.
        prev_runend = cur_runend;
        if (prev_runend + 1 < scaled_sizes_[size_grade] && GetSlot(store, prev_runend + 1) == 0) {
            found_empty_right = true;
            break;
        }
        cur_runend = NextRunend(store, cur_runend);
        cur_occupied = NextOccupied(store, cur_occupied);
        const uint32_t mapped_pos = (cur_occupied * size_scalars_[size_grade] * implicit_scalar) 
                                            >> (scale_shift + scale_implicit_shift);
        shift_end = (mapped_pos < prev_runend + 1 ? cur_runend : shift_end);
        if (mapped_pos > prev_runend + 1)
            break;
    }
    if (!found_empty_right) {
        // Check if should shift to the right.
        int32_t cur_occupied = implicit_part;
        int32_t cur_runend = PreviousRunend(store, runend_pos), prev_runend = runend_pos;
        while (cur_runend >= 0) {
            if (GetSlot(store, cur_runend + 1) == 0) {
                const int32_t runstart = FindEmptySlotBefore(store, prev_runend) + 1;
                const uint32_t mapped_pos = (cur_occupied * size_scalars_[size_grade] * implicit_scalar) 
                                                    >> (scale_shift + scale_implicit_shift);
                shift_start = (mapped_pos > runstart ? runstart : shift_start);
                break;
            }
            const uint32_t mapped_pos = (cur_occupied * size_scalars_[size_grade] * implicit_scalar) 
                                                >> (scale_shift + scale_implicit_shift);
            shift_start = (mapped_pos > cur_runend + 1 ? cur_runend + 1 : shift_start);
            prev_runend = cur_runend;
            cur_runend = PreviousRunend(store, cur_runend);
            cur_occupied = PreviousOccupied(store, cur_occupied);
        }
    }

    uint64_t *runends = store.ptr + infix_store_target_size / 64;
    if (shift_start == -1) {    // Shift to the left
        ShiftRunendsLeft(store, match_pos + 1, shift_end + 1, 1);
        ShiftSlotsLeft(store, match_pos + 1, shift_end + 1, 1);
        if (!run_destroyed)
            set_bitmap_bit(runends, runend_pos - 1);
    }
    else {  // Shift to the right
        ShiftRunendsRight(store, shift_start, match_pos, 1);
        ShiftSlotsRight(store, shift_start, match_pos, 1);
        if (!run_destroyed)
            set_bitmap_bit(runends, runend_pos);
    }
    
    if (run_destroyed)
        reset_bitmap_bit(occupieds, implicit_part);
    store.UpdateElemCount(-1);
}


template <bool int_optimized>
inline uint32_t Steroids<int_optimized>::GetLongestMatchingInfixSize(const InfixStore &store, const uint64_t key,
                                                                     const uint32_t total_implicit) const {
    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    uint64_t *occupieds = store.ptr;
    if (!get_bitmap_bit(occupieds, implicit_part))
        return 0;   // No matching infix found

    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const int32_t runend_pos = SelectRunends(store, key_rank);
    const int32_t runstart_pos = std::max(key_rank ? static_cast<int32_t>(SelectRunends(store, key_rank - 1)) : -1,
                                          static_cast<int32_t>(FindEmptySlotBefore(store, runend_pos))) + 1;
    const bool run_destroyed = runstart_pos == runend_pos;

    int32_t l = runstart_pos - 1, r = runend_pos + 1, mid;
    while (r - l > 1) {
        mid = (l + r) / 2;
        uint64_t value = GetSlot(store, mid);
        value -= value & -value;
        if (value <= key - 1)
            l = mid;
        else 
            r = mid;
    }
    int32_t match_pos;
    for (match_pos = l; match_pos >= runstart_pos; match_pos--) {
        const uint64_t value = GetSlot(store, match_pos);
        const uint64_t mask = ((value & -value) << 1) - 1;
        if ((value | mask) == (explicit_part | mask))
            return infix_size_ - lowbit_pos(value);
    }
    return 0;   // No matching infix found
}


template <bool int_optimized>
inline bool Steroids<int_optimized>::RangeQueryInfixStore(InfixStore &store, const uint64_t l_key, const uint64_t r_key,
                                                          const uint32_t total_implicit) const {
    const uint64_t l_implicit_part = l_key >> infix_size_;
    const uint64_t l_explicit_part = l_key & BITMASK(infix_size_);
    const uint64_t r_implicit_part = r_key >> infix_size_;
    const uint64_t r_explicit_part = r_key & BITMASK(infix_size_);
    const uint64_t *occupieds = store.ptr;
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;

    if (l_implicit_part < r_implicit_part) {
        if (NextOccupied(store, l_implicit_part) < r_implicit_part)
            return true;

        if (get_bitmap_bit(occupieds, r_implicit_part)) {
            const uint32_t r_rank = RankOccupieds(store, r_implicit_part);
            const uint32_t runend_pos = SelectRunends(store, r_rank);
            const uint32_t runstart_pos = std::max(r_rank ? static_cast<int32_t>(SelectRunends(store, r_rank - 1)) : -1,
                                                   static_cast<int32_t>(FindEmptySlotBefore(store, runend_pos))) + 1;
            const uint64_t slot_value = GetSlot(store, runstart_pos);
            if (slot_value - (slot_value & -slot_value) <= r_explicit_part)
                return true;
        }
        if (get_bitmap_bit(occupieds, l_implicit_part)) {
            const uint32_t l_rank = RankOccupieds(store, l_implicit_part);
            const uint32_t runend_pos = SelectRunends(store, l_rank);
            uint32_t pos = runend_pos;
            uint64_t slot_value = GetSlot(store, pos);
            do {
                if (l_explicit_part <= (slot_value | (slot_value - 1)))
                    return true;
                if (pos == 0)
                    break;
                slot_value = GetSlot(store, --pos);
            } while (slot_value && !get_bitmap_bit(runends, pos));
        }
        return false;
    }

    // l_implicit_part == r_implicit_part
    if (!get_bitmap_bit(occupieds, l_implicit_part))
        return false;
    const uint32_t rank = RankOccupieds(store, l_implicit_part);
    const uint32_t runend_pos = SelectRunends(store, rank);
    uint32_t pos = runend_pos;
    uint64_t slot_value = GetSlot(store, pos);
    // TODO: Faster implementation via broadword operations?
    do {
        if (l_explicit_part <= (slot_value | (slot_value - 1))
            && (slot_value - (slot_value & (-slot_value))) <= r_explicit_part)
            return true;
        if (pos == 0)
            break;
        slot_value = GetSlot(store, --pos);
    } while (slot_value && !get_bitmap_bit(runends, pos));
    return false;
}


template <bool int_optimized>
inline bool Steroids<int_optimized>::PointQueryInfixStore(InfixStore &store, const uint64_t key, const uint32_t total_implicit) const {
    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    const uint64_t *occupieds = store.ptr;
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;

    if (!get_bitmap_bit(occupieds, implicit_part))
        return false;

    const uint32_t rank = RankOccupieds(store, implicit_part);
    const uint32_t runend_pos = SelectRunends(store, rank);
    uint32_t pos = runend_pos;
    uint64_t slot_value = GetSlot(store, pos);
    // TODO: Faster implementation via broadword operations?
    do {
        const uint64_t mask = ((slot_value & (-slot_value)) << 1) - 1;
        if ((explicit_part | mask) == (slot_value | mask))
            return true;
        if (pos == 0)
            break;
        slot_value = GetSlot(store, --pos);
    } while (slot_value && !get_bitmap_bit(runends, pos));
    return false;
}


template <bool int_optimized>
inline void Steroids<int_optimized>::ResizeInfixStore(InfixStore &store, const bool expand, const uint32_t total_implicit) {
    // TODO: Optimize further?
    uint32_t size_grade = store.GetSizeGrade();
    const uint32_t infix_count = store.GetElemCount();
    const uint32_t current_size = scaled_sizes_[size_grade];

    uint64_t infix_list[infix_count];
    GetInfixList(store, infix_list);
    delete[] store.ptr;

    size_grade += expand ? 1 : -1;
    store.SetSizeGrade(size_grade);
    const uint32_t next_size = scaled_sizes_[size_grade];
    const uint32_t word_count = InfixStore::GetPtrWordCount(next_size, infix_size_);
    store.ptr = new uint64_t[word_count];
    LoadListToInfixStore(store, infix_list, infix_count, total_implicit, true);
}


template <bool int_optimized>
inline void Steroids<int_optimized>::ShrinkInfixStoreInfixSize(InfixStore &store, const uint32_t new_infix_size) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t infix_count = store.GetElemCount();
    const uint32_t slot_count = scaled_sizes_[size_grade];

    InfixStore new_store(slot_count, new_infix_size);

    // Copy the occupieds and runends bitmaps
    const uint32_t total_bitmap_size = infix_store_target_size + scaled_sizes_[size_grade];
    memcpy(new_store.ptr, store.ptr, (total_bitmap_size + 7) / 8);
    new_store.ptr[(total_bitmap_size + 63) / 64 - 1] &= BITMASK(64 - total_bitmap_size % 64);

    for (int32_t i = 0; i < slot_count; i++) {
        const uint64_t old_slot = GetSlot(store, i);
        const uint64_t new_slot = (old_slot >> (infix_size_ - new_infix_size))
                                | (infix_size_ > new_infix_size + lowbit_pos(old_slot) ? 1ULL : 0ULL);
        SetSlot(new_store, i, new_slot, new_infix_size);
    }
    delete[] store.ptr;
    store.ptr = new_store.ptr;
}


template <bool int_optimized>
inline void Steroids<int_optimized>::LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                                                          const uint32_t total_implicit, const bool zero_out) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t total_size = scaled_sizes_[size_grade];
    assert(total_implicit >= infix_store_target_size / 2);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    if (zero_out)
        store.Reset(total_size, infix_size_);
    store.SetElemCount(list_len);
    if (list_len == 0)
        return;

    int32_t l[list_len + 1], r[list_len + 1], ind = 0;

    // Make sure everything is in increasing order
    for (int32_t i = 1; i < list_len; i++)
        assert((list[i - 1] >> infix_size_) <= (list[i] >> infix_size_));
    for (int32_t i = 0; i < list_len; i++)
        assert((list[i] >> infix_size_) < total_implicit);

    uint64_t *occupieds = store.ptr;
    uint64_t *runends = store.ptr + infix_store_target_size / 64;
    uint64_t old_implicit_part = list[0] >> infix_size_;
    l[0] = (old_implicit_part * size_scalars_[size_grade] * implicit_scalar)
                >> (scale_shift + scale_implicit_shift);
    r[0] = l[0];
    for (uint32_t i = 0; i < list_len; i++) {
        const uint64_t implicit_part = list[i] >> infix_size_;
        if (implicit_part != old_implicit_part) {
            ind++;
            l[ind] = std::max<int32_t>(r[ind - 1],
                                       (implicit_scalar * size_scalars_[size_grade] * implicit_part)
                                            >> (scale_shift + scale_implicit_shift));
            r[ind] = l[ind];
        }
        r[ind]++;
        old_implicit_part = implicit_part;
    }

    ind++;
    l[ind] = r[ind] = total_size;
    for (int32_t i = ind - 1; i >= 0; i--) {
        const int32_t diff = std::min(0, l[i + 1] - r[i]);
        l[i] += diff;
        r[i] += diff;
    }

    uint32_t write_head = 0;
    for (uint32_t i = 0; i < ind; i++) {
        for (uint32_t j = l[i]; j < r[i]; j++) {
            const uint64_t implicit_part = list[write_head] >> infix_size_;
            assert(implicit_part < total_implicit);
            set_bitmap_bit(occupieds, implicit_part);
            const uint64_t explicit_part = list[write_head] & BITMASK(infix_size_);
            write_head++;

            SetSlot(store, j, explicit_part);
        }
        set_bitmap_bit(runends, r[i] - 1);
    }
}


template <bool int_optimized>
inline typename Steroids<int_optimized>::InfixStore Steroids<int_optimized>::AllocateInfixStoreWithList(const uint64_t *list,
                                                                                                        const uint32_t list_len,
                                                                                                        const uint32_t total_implicit) {
    const uint32_t scaled_len = (size_scalars_[0] * list_len) >> scale_shift;
    uint32_t size_grade;
    for (size_grade = 0; size_grade < size_scalar_count && scaled_sizes_[size_grade] < scaled_len; size_grade++);
    InfixStore res(scaled_sizes_[size_grade], infix_size_, size_grade);
    LoadListToInfixStore(res, list, list_len, total_implicit);
    return res;
}


template <bool int_optimized>
inline uint32_t Steroids<int_optimized>::GetInfixList(const InfixStore &store, uint64_t *res) const {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t store_size = scaled_sizes_[size_grade];
    const uint64_t *occupieds = store.ptr;
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    uint64_t implicit_part = occupieds[0] & 1ULL ? 0 : NextOccupied(store, 0);
    uint32_t ind = 0;
    for (int32_t i = 0; i < store_size; i++) {
        const uint64_t explicit_part = GetSlot(store, i);
        if (explicit_part) {
            assert(implicit_part < infix_store_target_size);
            res[ind++] = (implicit_part << infix_size_) | explicit_part;
        }
        if (get_bitmap_bit(runends, i))
            implicit_part = NextOccupied(store, implicit_part);
    }
    return ind;
}


