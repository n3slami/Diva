#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <endian.h>
#include <iomanip>
#include <iostream>
#include <random>
#include <tuple>

#include "art.hpp"
#include "art/tree_it.hpp"
#include "util.hpp"

static void print_key(const uint8_t *key, const uint32_t key_len, const bool binary=false) {
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


static void print_key(const char *key, const uint32_t key_len, const bool binary=false) {
    print_key(reinterpret_cast<const uint8_t *>(key), key_len, binary);
}


class Steroids {
    friend class SteroidsTests;
    friend class InfixStoreTests;

public:
    Steroids(const uint32_t infix_size, const uint32_t rng_seed,
             const float load_factor): infix_size_(infix_size), load_factor_(load_factor) {
        rng_.seed(rng_seed);
        for (int32_t i = 0; i < 8; i++)
            zero_padding += '\0';
        SetupScaleFactors();
    }

    void Insert(const uint8_t *key, const uint32_t key_len);
    bool RangeQuery(const uint8_t *l_key, const uint32_t l_key_len, const uint8_t *r_key, const uint32_t r_key_len);
    bool PointQuery(const uint8_t *key, const uint32_t key_len);
    void ShrinkInfixSize(const uint32_t new_infix_size);

private:
    static const uint32_t infix_store_target_size = 512;
    static const uint32_t implicit_size = 9;
    static const uint32_t scale_shift = 15;
    static const uint32_t scale_implicit_shift = 15;
    static const uint32_t size_scalar_count = 50;

    struct InfiniteByteString {
        const uint8_t *str;
        const size_t length;

        uint64_t wordAt(const uint32_t byte_pos) const {
            uint64_t res = 0;
            memcpy(&res, str + byte_pos, std::min(sizeof(res), length - byte_pos));
            return __bswap_64(res);
        };

        uint64_t bitsAt(const uint32_t bit_pos, const uint32_t res_width) const {
            uint64_t res = 0;
            memcpy(&res, str + bit_pos / 8, std::min(sizeof(res), length - bit_pos / 8));
            res = __bswap_64(res) >> (8 * sizeof(res) - res_width - bit_pos % 8);
            return res & BITMASK(res_width);
        };

        uint32_t getBit(const uint32_t pos) const {
            return (pos / 8 < length ? (str[pos / 8] >> (7 - pos % 8)) & 1 : 0);
        };

        bool isPrefixOf(const InfiniteByteString& other, const uint32_t bits_to_ignore=0) const {
            if (length <= other.length && memcmp(str, other.str, length - 1) == 0)
                return (str[length - 1] | BITMASK(bits_to_ignore)) == (other.str[length - 1] | BITMASK(bits_to_ignore));
            return false;
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

        inline uint32_t GetElemCount() const {
            return status & BITMASK(elem_count_bit_count);
        }

        inline void SetElemCount(const int32_t elem_count) {
            status &= ~BITMASK(elem_count_bit_count);
            status |= elem_count;
        }

        inline void UpdateElemCount(const int32_t delta) {
            status += delta;
        }

        inline uint32_t GetSizeGrade() const {
            return (status >> elem_count_bit_count) & BITMASK(size_grade_bit_count);
        }

        inline void SetSizeGrade(const uint32_t size_grade) {
            status &= ~(BITMASK(size_grade_bit_count) << elem_count_bit_count);
            status |= size_grade << elem_count_bit_count;
        }

        inline uint32_t GetInvalidBits() const {
            return status >> (elem_count_bit_count + size_grade_bit_count) & 7U;
        }

        inline void SetInvalidBits(const uint32_t invalid_bits) {
            status &= ~(7U << (elem_count_bit_count + size_grade_bit_count));
            status |= invalid_bits << (elem_count_bit_count + size_grade_bit_count);
        }

        inline bool IsPartialKey() const {
            return status >> 31;
        }

        inline void SetPartialKey(bool val) {
            if (val)
                status |= (1U << 31);
            else 
                status &= ~(1U << 31);
        }
    };

    uint32_t infix_size_;
    art::art<InfixStore> tree_;
    std::mt19937 rng_;
    std::string zero_padding;
    const float load_factor_ = 0.95;
    uint64_t size_scalars_[size_scalar_count], scaled_sizes_[size_scalar_count];
    uint64_t implicit_scalars_[infix_store_target_size / 2 + 1];

    inline void AddTreeKey(const uint8_t *key, const uint32_t key_len);
    inline void InsertSimple(const InfiniteByteString key);
    inline void InsertSplit(const InfiniteByteString key);
    inline void SetupScaleFactors();
    inline std::tuple<uint32_t, uint32_t> GetSharedIgnoreLengths(const InfiniteByteString key_1,
                                                                 const InfiniteByteString key_2);
    inline uint64_t ExtractPartialKey(const InfiniteByteString key,
                                      const uint32_t shared, const uint32_t ignore,
                                      const uint64_t msb);

    inline uint32_t RankOccupieds(const InfixStore &store, const uint32_t pos) const;
    inline uint32_t SelectRunends(const InfixStore &store, const uint32_t rank) const;
    inline int32_t NextRunend(const InfixStore &store, const uint32_t pos) const;
    inline int32_t NextOccupied(const InfixStore &store, const uint32_t pos) const;
    inline int32_t PreviousRunend(const InfixStore &store, const uint32_t pos) const;

    inline uint64_t GetSlot(const InfixStore &store, const uint32_t pos) const;
    inline void SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value);

    inline void ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftRunendsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftRunendsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);

    inline int32_t FindEmptySlotAfter(const InfixStore &store, const uint32_t runend_pos) const;
    inline int32_t FindEmptySlotBefore(const InfixStore &store, const uint32_t runend_pos) const;
    inline void InsertRawIntoInfixStore(InfixStore &store, const uint64_t key,
                                        const uint32_t total_implicit=infix_store_target_size);
    inline bool RangeQueryInfixStore(InfixStore &store, const uint64_t l_key, const uint64_t r_key,
                                     const uint32_t total_implicit=infix_store_target_size);
    inline bool PointQueryInfixStore(InfixStore &store, const uint64_t key,
                                     const uint32_t total_implicit=infix_store_target_size);
    inline void ResizeInfixStore(InfixStore &store, const bool expand=true,
                                 const uint32_t total_implicit=infix_store_target_size);
    inline void ShrinkInfixStoreInfixSize(InfixStore &store, const uint32_t new_infix_size);
    inline void LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                                     const uint32_t total_implicit=infix_store_target_size, const bool zero_out=false);
    inline InfixStore AllocateInfixStoreWithList(const uint64_t *list, const uint32_t list_len,
                                                 const uint32_t total_implicit=infix_store_target_size);
    inline uint32_t GetInfixList(const InfixStore &store, uint64_t *res) const;
    inline uint32_t GetExpandedInfixListLength(const uint64_t *list, const uint32_t list_len, const uint32_t shamt,
                                               const uint64_t lower_lim, const uint64_t upper_lim);
    inline void UpdateInfixList(const uint64_t *list, const uint32_t list_len, const uint32_t shamt, 
                                     const uint64_t lower_lim, const uint64_t upper_lim,
                                     uint64_t *res, const uint32_t res_len);
};


inline void Steroids::SetupScaleFactors() {
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


inline void Steroids::Insert(const uint8_t *key, const uint32_t key_len) {
    const InfiniteByteString converted_key {key, key_len};

    if (rng_() % infix_store_target_size == 0)
        InsertSplit(converted_key);
    else 
        InsertSimple(converted_key);
}

inline void Steroids::InsertSimple(const InfiniteByteString key) {
    auto it = tree_.begin(key.str, key.length);
    std::string next_tree_key, prev_tree_key;
    next_tree_key = it.key();
    if (next_tree_key.size() <= key.length && memcmp(key.str, next_tree_key.c_str(), next_tree_key.size()) == 0) {
        prev_tree_key = next_tree_key;
        it++;
        next_tree_key = it.key();
        it--;
    }
    else {
        it--;
        prev_tree_key = it.key();
    }
    const InfiniteByteString next_key {reinterpret_cast<const uint8_t *>(next_tree_key.c_str()),
                                       next_tree_key.size()};
    const InfiniteByteString prev_key {reinterpret_cast<const uint8_t *>(prev_tree_key.c_str()),
                                       prev_tree_key.size()};
    InfixStore &infix_store = it.ref();

    auto [shared, ignore] = GetSharedIgnoreLengths(prev_key, next_key);
    std::cerr << "prev_key=";
    print_key(prev_key.str, prev_key.length, true);
    std::cerr << "next_key=";
    print_key(next_key.str, next_key.length, true);
    std::cerr << "shared=" << shared << " ignore=" << ignore << std::endl;

    std::cerr << "key=";
    print_key(key.str, key.length, true);

    const uint64_t extraction = ExtractPartialKey(key, shared, ignore, key.getBit(shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key, shared, ignore, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key, shared, ignore, 0) >> infix_size_;
    std::cerr << "extraction=";
    for (int32_t i = infix_size_ + implicit_size - 1; i >= 0; i--)
        std::cerr << ((extraction >> i) & 1);
    std::cerr << std::endl;
    std::cerr << "prev_implicit=";
    for (int32_t i = implicit_size - 1; i >= 0; i--)
        std::cerr << ((prev_implicit >> i) & 1);
    std::cerr << std::endl;
    std::cerr << "next_implicit=";
    for (int32_t i = implicit_size - 1; i >= 0; i--)
        std::cerr << ((next_implicit >> i) & 1);
    std::cerr << std::endl;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t insertee = ((extraction | 1ULL) - (prev_implicit << infix_size_));
    InsertRawIntoInfixStore(infix_store, insertee, total_implicit);
}


/*
inline bool Steroids::RangeQuery(const uint8_t *l_key, const uint32_t l_key_len,
                                 const uint8_t *r_key, const uint32_t r_key_len) {
    auto it = tree_.begin(l_key, l_key_len);
    const std::string next_key = it.key();
    const int32_t cmp_result = memcmp(next_key.c_str(), r_key, std::min(static_cast<uint32_t>(next_key.size()), r_key_len));
    if (cmp_result < 0 || (cmp_result == 0 && next_key.size() < r_key_len))
        return true;
    it--;
    const std::string prev_key = it.key();
    const std::string padded_next_key = next_key + zero_padding;
    const std::string padded_prev_key = prev_key + zero_padding;
    InfixStore &infix_store = it.ref();

    const uint32_t max_len = std::max(prev_key.size(), next_key.size());
    auto [shared, ignore] = GetSharedIgnoreLengths(padded_prev_key.c_str(), padded_next_key.c_str(), max_len);

    const uint32_t padded_key_len = max_len + 8;
    uint8_t l_padded_key[padded_key_len], r_padded_key[padded_key_len];
    memcpy(l_padded_key, l_key, std::min(padded_key_len, l_key_len));
    if (l_key_len < padded_key_len)
        memset(l_padded_key + l_key_len, 0, padded_key_len - l_key_len);
    memcpy(r_padded_key, r_key, std::min(padded_key_len, r_key_len));
    if (r_key_len < padded_key_len)
        memset(r_padded_key + r_key_len, 0, padded_key_len - r_key_len);

    const uint64_t l_extraction = ExtractPartialKey(l_padded_key, shared, ignore, get_string_kth_bit(l_padded_key, shared));
    const uint64_t r_extraction = ExtractPartialKey(r_padded_key, shared, ignore, get_string_kth_bit(r_padded_key, shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key.c_str(), shared, ignore, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key.c_str(), shared, ignore, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t l_val = ((l_extraction | 1ULL) - (prev_implicit << infix_size_));
    const uint64_t r_val = ((l_extraction | 1ULL) - (prev_implicit << infix_size_));
    return RangeQueryInfixStore(infix_store, l_val, r_val, total_implicit);
}
*/


/*
inline bool Steroids::PointQuery(const uint8_t *key, const uint32_t key_len) {
    auto it = tree_.begin(key, key_len);
    const std::string next_key = it.key();
    if (key_len == next_key.size() && memcmp(key, next_key.c_str(), key_len) == 0)
        return true;
    it--;
    const std::string prev_key = it.key();
    const std::string padded_next_key = next_key + zero_padding;
    const std::string padded_prev_key = prev_key + zero_padding;
    InfixStore &infix_store = it.ref();

    const uint32_t max_len = std::max(prev_key.size(), next_key.size());
    auto [shared, ignore] = GetSharedIgnoreLengths(padded_prev_key.c_str(), padded_next_key.c_str(), max_len);

    const uint32_t padded_key_len = max_len + 8;
    uint8_t padded_key[padded_key_len];
    memcpy(padded_key, key, std::min(padded_key_len, key_len));
    if (key_len < padded_key_len)
        memset(padded_key + key_len, 0, padded_key_len - key_len);

    const uint64_t extraction = ExtractPartialKey(padded_key, shared, ignore, get_string_kth_bit(padded_key, shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key.c_str(), shared, ignore, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key.c_str(), shared, ignore, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t query_key = (extraction - (prev_implicit << infix_size_));
    return PointQueryInfixStore(infix_store, query_key, total_implicit);
}
*/


inline void Steroids::AddTreeKey(const uint8_t *key, const uint32_t key_len) {
    std::cerr << "adding key to tree key=";
    print_key(key, key_len, true);

    InfixStore infix_store(scaled_sizes_[0], infix_size_);
    tree_.set(key, key_len, infix_store);
}


inline void Steroids::InsertSplit(InfiniteByteString key) {
    auto it = tree_.begin(key.str, key.length);
    std::string next_tree_key, prev_tree_key;
    next_tree_key = it.key();
    if (next_tree_key.size() <= key.length && memcmp(key.str, next_tree_key.c_str(), next_tree_key.size()) == 0) {
        prev_tree_key = next_tree_key;
        it++;
        next_tree_key = it.key();
        it--;
    }
    else {
        it--;
        prev_tree_key = it.key();
    }
    const InfiniteByteString next_key {reinterpret_cast<const uint8_t *>(next_tree_key.c_str()),
                                       next_tree_key.size()};
    const InfiniteByteString prev_key {reinterpret_cast<const uint8_t *>(prev_tree_key.c_str()),
                                       prev_tree_key.size()};
    InfixStore& infix_store = it.ref();

    if (infix_store.IsPartialKey() && prev_key.isPrefixOf(key, infix_store.GetInvalidBits())) {
        std::cerr << "previous key was a partial key and a prefix of the new boundary key, inserting using the simple method..." << std::endl;
        InsertSimple(key);
        return;
    }

    auto [shared, ignore] = GetSharedIgnoreLengths(prev_key, next_key);
    std::cerr << "shared=" << shared << " ignore=" << ignore << std::endl;
    uint64_t extraction = ExtractPartialKey(key, shared, ignore, key.getBit(shared));
    uint64_t prev_extraction = ExtractPartialKey(prev_key, shared, ignore, 0);
    uint64_t next_extraction = ExtractPartialKey(next_key, shared, ignore, 1);
    const uint64_t separator = (extraction | 1ULL) - (prev_extraction & (BITMASK(implicit_size) << infix_size_));

    const uint32_t infix_list_len = infix_store.GetElemCount();
    uint64_t infix_list[infix_list_len + 1];
    const uint32_t infix_count = GetInfixList(infix_store, infix_list);

    std::cerr << "infix list: ";
    for (int32_t i = 0; i < infix_count; i++) {
        std::cerr << (infix_list[i] >> infix_size_) << ',';
        for (int32_t j = infix_size_ - 1; j >= 0; j--)
            std::cerr << ((infix_list[i] >> j) & 1);
        std::cerr << ' ';
    }
    std::cerr << std::endl;

    int32_t sep_l = -1, sep_r = infix_count, sep_mid;
    while (sep_r - sep_l > 1) {
        sep_mid = (sep_l + sep_r) / 2;
        std::cerr << "sep_l=" << sep_l << " sep_r=" << sep_r << " sep_mid=" << sep_mid << " --- " << infix_list[sep_mid] << " vs. separator=" << separator << std::endl;
        const uint64_t val = infix_list[sep_mid] - (infix_list[sep_mid] & (-infix_list[sep_mid]));
        if (val < separator - 1)
            sep_l = sep_mid;
        else
            sep_r = sep_mid;
    }
    uint32_t split_pos = sep_r;
    bool intersecting_range = false;
    int32_t zero_pos = -1;
    for (int32_t i = split_pos; i >= 0; i--) {
        const uint64_t mask = ((infix_list[i] & (-infix_list[i])) << 1) - 1;
        if ((infix_list[i] | mask) != (separator | mask))
            break;
        split_pos = i;
        zero_pos = shared + ignore + implicit_size + infix_size_ - lowbit_pos(infix_list[i]) - 1;
    }
    std::cerr << "split_pos=" << split_pos << std::endl;
    std::cerr << "ZEEEEEEEEROOOOOOOOOOOOOOOO zero_pos=" << zero_pos << std::endl;
    uint32_t copied_key_len = key.length;
    uint8_t copied_key_str[copied_key_len];
    memcpy(copied_key_str, key.str, key.length);
    if (zero_pos != -1) {
        copied_key_str[zero_pos / 8] &= BITMASK(7 - zero_pos % 8);
        copied_key_len = zero_pos / 8 + 1;
    }
    InfiniteByteString edited_key {copied_key_str, copied_key_len};
    if (zero_pos != -1)
        extraction = ExtractPartialKey(edited_key, shared, ignore, edited_key.getBit(shared));

    std::cerr << "       key=";
    print_key(key.str, key.length, true);
    std::cerr << "edited_key=";
    print_key(edited_key.str, edited_key.length, true);
    std::cerr << "  prev_key=";
    print_key(prev_key.str, prev_key.length, true);
    std::cerr << "  next_key=";
    print_key(next_key.str, next_key.length, true);

    const uint32_t shared_word_byte = (shared / 64) * 8;
    const uint32_t total_infix_size = implicit_size + infix_size_ - 1;

    auto [shared_lt, ignore_lt] = GetSharedIgnoreLengths({prev_key.str + shared_word_byte,
                                                          prev_key.length < shared_word_byte ? 0 : prev_key.length - shared_word_byte},
                                                         {edited_key.str + shared_word_byte, 
                                                          edited_key.length < shared_word_byte ? 0 : edited_key.length - shared_word_byte});
    shared_lt += shared_word_byte * 8;
    const int32_t shamt_lt = shared_lt + ignore_lt - shared - ignore;
    const uint64_t prev_extraction_lt = ExtractPartialKey(prev_key, shared_lt, ignore_lt, 0);
    const uint64_t extraction_lt = ExtractPartialKey(edited_key, shared_lt, ignore_lt, 1);
    const uint64_t left_start = prev_key.bitsAt(shared + ignore + implicit_size, shamt_lt) << infix_size_;
    const uint64_t left_end = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_lt))
                              | (edited_key.bitsAt(shared + ignore + implicit_size, shamt_lt) << infix_size_);
    const uint32_t total_implicit_lt = ((extraction_lt >> infix_size_) - (prev_extraction_lt >> infix_size_)) + 1;
    std::cerr << "shared_lt=" << shared_lt << " ignore_lt=" << ignore_lt << " --- shamt_lt=" << shamt_lt << " total_implicit_lt=" << total_implicit_lt << std::endl;

    auto [shared_gt, ignore_gt] = GetSharedIgnoreLengths({edited_key.str + shared_word_byte, 
                                                          edited_key.length < shared_word_byte ? 0 : edited_key.length - shared_word_byte},
                                                         {next_key.str + shared_word_byte,
                                                          next_key.length < shared_word_byte ? 0 : next_key.length - shared_word_byte});
    shared_gt += shared_word_byte * 8;
    const int32_t shamt_gt = shared_gt + ignore_gt - shared - ignore;
    const uint64_t extraction_gt = ExtractPartialKey(edited_key, shared_gt, ignore_gt, 0);
    const uint64_t next_extraction_gt = ExtractPartialKey(next_key, shared_gt, ignore_gt, 1);
    const uint64_t right_start = (((extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                | (edited_key.bitsAt(shared + ignore + implicit_size, shamt_gt) << infix_size_);
    const uint64_t right_end = (((next_extraction >> infix_size_) - (prev_extraction >> infix_size_)) << (infix_size_ + shamt_gt))
                                | (next_key.bitsAt(shared + ignore + implicit_size, shamt_gt) << infix_size_);
    const uint32_t total_implicit_gt = ((next_extraction_gt >> infix_size_) - (extraction_gt >> infix_size_)) + 1;
    std::cerr << "shared_gt=" << shared_gt << " ignore_gt=" << ignore_gt << " --- shamt_gt=" << shamt_gt << " total_implicit_gt="  << total_implicit_gt << std::endl;

    std::cerr << "extraction=";
    for (int32_t i = implicit_size + infix_size_ - 1; i >= 0; i--)
        std::cerr << ((extraction >> i) & 1);
    std::cerr << std::endl << "prev_extraction=";
    for (int32_t i = implicit_size + infix_size_ - 1; i >= 0; i--)
        std::cerr << ((prev_extraction >> i) & 1);
    std::cerr << std::endl << "next_extraction=";
    for (int32_t i = implicit_size + infix_size_ - 1; i >= 0; i--)
        std::cerr << ((next_extraction >> i) & 1);
    std::cerr << std::endl << std::endl << "extraction_lt=";
    for (int32_t i = implicit_size + infix_size_ - 1; i >= 0; i--)
        std::cerr << ((extraction_lt >> i) & 1);
    std::cerr << std::endl << "prev_extraction_lt=";
    for (int32_t i = implicit_size + infix_size_ - 1; i >= 0; i--)
        std::cerr << ((prev_extraction_lt >> i) & 1);
    std::cerr << std::endl << std::endl << "extraction_gt=";
    for (int32_t i = implicit_size + infix_size_ - 1; i >= 0; i--)
        std::cerr << ((extraction_gt >> i) & 1);
    std::cerr << std::endl << "next_extraction_gt=";
    for (int32_t i = implicit_size + infix_size_ - 1; i >= 0; i--)
        std::cerr << ((next_extraction_gt >> i) & 1);
    std::cerr << std::endl << std::endl;

    const uint32_t left_list_len = GetExpandedInfixListLength(infix_list,
                                                              split_pos,
                                                              shamt_lt,
                                                              left_start, left_end);
    std::cerr << "left_list_len=" << left_list_len << std::endl;
    uint64_t left_infix_list[left_list_len];
    UpdateInfixList(infix_list, split_pos, shamt_lt,
                    left_start, left_end,
                    left_infix_list, left_list_len);
    std::cerr << "left expansion done" << std::endl;

    const uint32_t right_list_len = GetExpandedInfixListLength(infix_list + split_pos,
                                                               infix_list_len - split_pos,
                                                               shamt_gt,
                                                               right_start, right_end);
    std::cerr << "right_list_len=" << right_list_len << std::endl;
    uint64_t right_infix_list[right_list_len];
    UpdateInfixList(infix_list + split_pos, infix_list_len - split_pos, shamt_gt,
                    right_start, right_end,
                    right_infix_list, right_list_len);
    std::cerr << "############################################################" << std::endl;
    std::cerr << "left_list with len=" << left_list_len << ": ";
    for (int32_t i = 0; i < left_list_len; i++) {
        std::cerr << (left_infix_list[i] >> infix_size_) << ',';
        for (int32_t j = infix_size_ - 1; j >= 0; j--) {
            std::cerr << ((left_infix_list[i] >> j) & 1);
        }
        std::cerr << ' ';
    }
    std::cerr << std::endl;
    std::cerr << "right_list with len=" << right_list_len << ": ";
    for (int32_t i = 0; i < right_list_len; i++) {
        std::cerr << (right_infix_list[i] >> infix_size_) << ',';
        for (int32_t j = infix_size_ - 1; j >= 0; j--) {
            std::cerr << ((right_infix_list[i] >> j) & 1);
        }
        std::cerr << ' ';
    }
    std::cerr << std::endl;
    std::cerr << "############################################################" << std::endl;

    InfixStore store_lt = AllocateInfixStoreWithList(left_infix_list,
                                                     left_list_len,
                                                     total_implicit_lt);
    std::cerr << "allocated store_lt" << std::endl;
    store_lt.SetInvalidBits(infix_store.GetInvalidBits());
    store_lt.SetPartialKey(infix_store.IsPartialKey());
    InfixStore store_gt = AllocateInfixStoreWithList(right_infix_list + (zero_pos != -1),
                                                     right_list_len - (zero_pos != -1),
                                                     total_implicit_gt);
    std::cerr << "allocated store_gt" << std::endl;
    tree_.set(prev_key.str, prev_key.length, store_lt);
    if (zero_pos != -1) {
        std::cerr << "HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE" << std::endl;
        const uint64_t key_extraction = ExtractPartialKey(key, shared_gt, ignore_gt, 0);
        InsertRawIntoInfixStore(store_gt, key_extraction & BITMASK(infix_size_) | 1, total_implicit_gt);
        store_gt.SetInvalidBits((8 - zero_pos) % 8);
        store_gt.SetPartialKey(true);
        tree_.set(edited_key.str, edited_key.length, store_gt);
    }
    else
        tree_.set(key.str, key.length, store_gt);

    // TODO: fix memory leak
}


__attribute__((always_inline))
inline uint32_t Steroids::GetExpandedInfixListLength(const uint64_t *list, const uint32_t list_len, const uint32_t shamt,
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
            std::cerr << "start=" << start << " end=" << end << " --- lower_implicit_lim=" << lower_implicit_lim << " upper_implicit_lim=" << upper_implicit_lim << std::endl;
            actual_list_len += std::min(end, upper_implicit_lim) - std::max(start, lower_implicit_lim);
        }
    }
    return actual_list_len;
}


__attribute__((always_inline))
inline void Steroids::UpdateInfixList(const uint64_t *list, const uint32_t list_len, const uint32_t shamt,
                                      const uint64_t lower_lim, const uint64_t upper_lim,
                                      uint64_t *res, const uint32_t res_len) {
    std::cerr << "list_len=" << list_len << " vs. res_len=" << res_len << std::endl;
    std::cerr << "shamt=" << shamt << " --- lower_lim=";
    for (int32_t i = implicit_size + infix_size_ + shamt - 1; i >= 0; i--)
        std::cerr << ((lower_lim >> i) & 1);
    std::cerr << "=" << lower_lim << " upper_lim=";
    for (int32_t i = implicit_size + infix_size_ + shamt - 1; i >= 0; i--)
        std::cerr << ((upper_lim >> i) & 1);
    std::cerr << "=" << upper_lim << std::endl;

    if (list_len == res_len) {
        for (int32_t i = 0; i < list_len; i++)
            res[i] = (list[i] << shamt) - lower_lim;
        return;
    }

    uint32_t res_ind = 0;
    const uint64_t lower_implicit_lim = lower_lim >> infix_size_;
    const uint64_t upper_implicit_lim = upper_lim >> infix_size_;
    for (int32_t i = 0; i < list_len; i++) {
        std::cerr << "@list[" << i << "]=" << (list[i] >> infix_size_) << ',' << (list[i] & BITMASK(infix_size_)) << std::endl;
        const uint64_t val = list[i] << shamt;
        const uint64_t implicit_part = val >> infix_size_;
        const uint64_t explicit_part = val & BITMASK(infix_size_);
        if (explicit_part == 0) {
            assert(implicit_part > 0);
            const uint64_t start = implicit_part - (implicit_part & (-implicit_part));
            const uint64_t end = implicit_part | (implicit_part - 1);
            std::cerr << "WELP start=" << start << " end=" << end << " --- res_ind=" << res_ind << std::endl;
            std::cerr << "implicit_part=" << implicit_part << " explicit_part=" << explicit_part << std::endl;
            for (uint64_t j = 0; j <= std::min(end, upper_implicit_lim) - std::max(start, lower_implicit_lim); j++)
                res[res_ind++] = (j << infix_size_) | (1ULL << (infix_size_ - 1));
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


inline std::tuple<uint32_t, uint32_t> 
Steroids::GetSharedIgnoreLengths(InfiniteByteString key_1, InfiniteByteString key_2) {
    uint32_t share = 0, ignore = 0, implicit = 0;

    uint32_t ind = 0, delta;
    do {
        const uint64_t read_1 = key_1.wordAt(ind * sizeof(uint64_t));
        const uint64_t read_2 = key_2.wordAt(ind * sizeof(uint64_t));
        delta = __builtin_ia32_lzcnt_u64(read_1 ^ read_2);
        share += delta;
        ind++;
    } while (delta == 64);

    ind--;
    do {
        const uint64_t read_1 = key_1.wordAt(ind * sizeof(uint64_t));
        const uint64_t read_2 = key_2.wordAt(ind * sizeof(uint64_t));
        const uint32_t offset = (ind > share / 64 ? 0 : share % 64 + 1);
        delta = __builtin_ia32_lzcnt_u64(((~read_1) | read_2) & BITMASK(64 - offset));
        ignore += delta - offset;
        ind++;
    } while (delta == 64);

    return {share, ignore};
}


inline void Steroids::ShrinkInfixSize(const uint32_t new_infix_size) {
    auto it = tree_.begin();
    do {
        InfixStore &store = it.ref();
        ShrinkInfixStoreInfixSize(store, new_infix_size);
        it++;
    } while (it != tree_.end());

    infix_size_ = new_infix_size;
}


__attribute__((always_inline))
inline uint64_t Steroids::ExtractPartialKey(const InfiniteByteString key,
                                            const uint32_t shared, const uint32_t ignore,
                                            const uint64_t msb) {
    const uint32_t real_diff_pos = shared + ignore;
    uint64_t res = key.wordAt(real_diff_pos / 8);
    res >>= (63 - (implicit_size - 1) - infix_size_ - real_diff_pos % 8);
    res &= BITMASK(implicit_size - 1 + infix_size_);
    res |= msb << (implicit_size - 1 + infix_size_);
    print_key(key.str, key.length, true);
    return res;
}

__attribute__((always_inline))
inline uint32_t Steroids::RankOccupieds(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr;
    uint32_t res = 0;
    for (int32_t i = 0; i < pos / 64; i++)
        res += __builtin_popcountll(occupieds[i]);
    return res + bit_rank(occupieds[pos / 64], pos % 64);
}


__attribute__((always_inline))
inline uint32_t Steroids::SelectRunends(const InfixStore &store, const uint32_t rank) const {
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


__attribute__((always_inline))
inline int32_t Steroids::NextRunend(const InfixStore &store, const uint32_t pos) const {
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


inline int32_t Steroids::NextOccupied(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *occupieds = store.ptr;
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(occupieds[res / 64] & (~BITMASK(res % 64)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < infix_store_target_size);
    return res;
}


__attribute__((always_inline))
inline int32_t Steroids::PreviousRunend(const InfixStore &store, const uint32_t pos) const {
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    int32_t res = pos, hb_pos;
    do {
        const int32_t offset = res % 64;
        hb_pos = highbit_pos(runends[res / 64] & BITMASK(offset));
        res += (hb_pos == 64 ? -offset - 1 : hb_pos - offset);
    } while (hb_pos == 64 && res >= 0);
    return res;
}


__attribute__((always_inline))
inline uint64_t Steroids::GetSlot(const InfixStore &store, const uint32_t pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + pos * infix_size_;
    const uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t value;
    memcpy(&value, ptr, sizeof(value));
    return (value >> bit_pos % 8) & BITMASK(infix_size_);
}


__attribute__((always_inline))
inline void Steroids::SetSlot(InfixStore &store, const uint32_t pos, const uint64_t value) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + pos * infix_size_;
    uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t stamp;
    memcpy(&stamp, ptr, sizeof(stamp));
    stamp &= ~(BITMASK(infix_size_) << (bit_pos % 8));
    stamp |= value << (bit_pos % 8);
    memcpy(ptr, &stamp, sizeof(stamp));
}


__attribute__((always_inline))
inline void Steroids::ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r,
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


__attribute__((always_inline))
inline void Steroids::ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
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


__attribute__((always_inline))
inline void Steroids::ShiftRunendsRight(const InfixStore &store, const uint32_t l, const uint32_t r, 
                                        const uint32_t shamt) {
    shift_bitmap_right(store.ptr + infix_store_target_size / 64, l, r - 1, shamt);
}


__attribute__((always_inline))
inline void Steroids::ShiftRunendsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                       const uint32_t shamt) {
    shift_bitmap_left(store.ptr + infix_store_target_size / 64, l, r - 1, shamt);
}


__attribute__((always_inline))
inline int32_t Steroids::FindEmptySlotAfter(const InfixStore &store, const uint32_t runend_pos) const {
    const uint32_t size_grade = store.GetSizeGrade();
    int32_t current_pos = runend_pos;
    while (current_pos < scaled_sizes_[size_grade] && GetSlot(store, current_pos + 1)) {
        current_pos = NextRunend(store, current_pos);
    }
    return current_pos + 1;
}


__attribute__((always_inline))
inline int32_t Steroids::FindEmptySlotBefore(const InfixStore &store, const uint32_t runend_pos) const {
    int32_t current_pos = runend_pos, previous_pos;
    do {
        previous_pos = current_pos;
        current_pos = PreviousRunend(store, current_pos);
    } while (current_pos >= 0 && GetSlot(store, current_pos + 1));

    while (GetSlot(store, --previous_pos));
    return previous_pos;

    // Maybe binary searching would be better?
    int32_t l = current_pos, r = previous_pos, mid;
    while (r - l > 1) {
        mid = (l + r) / 2;
        if (GetSlot(store, mid) == 0)
            l = mid;
        else
            r = mid;
    }
    return l;
}


inline void Steroids::InsertRawIntoInfixStore(InfixStore &store, const uint64_t key, const uint32_t total_implicit) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t elem_count = store.GetElemCount();
    if (elem_count == (size_grade ? scaled_sizes_[size_grade - 1] : infix_store_target_size))
        ResizeInfixStore(store, true, total_implicit);

    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    std::cerr << "INSERTING RAW implicit_part=" << implicit_part << " explicit_part=" << explicit_part << std::endl;
    std::cerr << "size_grade=" << size_grade << " size_scalar=" << size_scalars_[size_grade] << std::endl;
    std::cerr << "elem_count=" << elem_count << " total_implicit=" << total_implicit << std::endl;

    uint64_t *occupieds = store.ptr;
    uint64_t *runends = store.ptr + infix_store_target_size / 64;

    const uint32_t mapped_pos = (implicit_part * size_scalars_[size_grade] * implicit_scalar) 
                                        >> (scale_shift + scale_implicit_shift);
    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const bool is_occupied = get_bitmap_bit(occupieds, implicit_part);
    std::cerr << "mapped_pos=" << mapped_pos << " --- key_rank=" << key_rank << " is_occupied=" << is_occupied << std::endl;
    std::cerr << "test=" << ((implicit_part * implicit_scalar) >> scale_implicit_shift) << std::endl;
    if (!is_occupied && GetSlot(store, mapped_pos) == 0) {
        SetSlot(store, mapped_pos, explicit_part);
        set_bitmap_bit(runends, mapped_pos);
    }
    else if (is_occupied) {
        const int32_t runend_pos = SelectRunends(store, key_rank);
        const int32_t next_empty = FindEmptySlotAfter(store, mapped_pos);
        const int32_t previous_empty = FindEmptySlotBefore(store, mapped_pos);

        std::cerr << "### runend_pos=" << runend_pos << " next_empty=" << next_empty << " previous_empty=" << previous_empty << std::endl;

        int32_t l = std::max(PreviousRunend(store, runend_pos), previous_empty);
        int32_t r = runend_pos + 1;
        int32_t mid;
        while (r - l > 1) {
            mid = (l + r) / 2;
            uint64_t range_l = GetSlot(store, mid);
            range_l -= range_l & (-range_l);
            if (range_l <= explicit_part - 1)
                l = mid;
            else 
                r = mid;
        }
        std::cerr << "IS OCCUPIED l=" << l << " r=" << r << std::endl;
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
        const int32_t runend_pos = SelectRunends(store, key_rank - 1);
        const int32_t next_empty = FindEmptySlotAfter(store, mapped_pos);
        if (next_empty < scaled_sizes_[size_grade]) {
            ShiftSlotsRight(store, runend_pos + 1, next_empty, 1);
            ShiftRunendsRight(store, runend_pos + 1, next_empty, 1);
            SetSlot(store, runend_pos + 1, explicit_part);
            set_bitmap_bit(runends, runend_pos + 1);
        }
        else {
            const int32_t previous_empty = FindEmptySlotBefore(store, mapped_pos);
            ShiftSlotsLeft(store, previous_empty + 1, runend_pos + 1, 1);
            ShiftRunendsLeft(store, previous_empty + 1, runend_pos + 1, 1);
            SetSlot(store, runend_pos, explicit_part);
            set_bitmap_bit(runends, runend_pos);
        }
    }
    set_bitmap_bit(occupieds, implicit_part);
    store.UpdateElemCount(1);
}


inline bool Steroids::RangeQueryInfixStore(InfixStore &store, const uint64_t l_key, const uint64_t r_key,
                                           const uint32_t total_implicit) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t elem_count = store.GetElemCount();
    if (elem_count == (size_grade ? scaled_sizes_[size_grade - 1] : infix_store_target_size))
        ResizeInfixStore(store, true, total_implicit);

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
            if (slot_value - (slot_value & (-slot_value)) <= r_explicit_part)
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
    }

    // l_implicit_part == r_implicit_part
    if (!get_bitmap_bit(occupieds, l_implicit_part))
        return false;
    const uint32_t rank = RankOccupieds(store, l_implicit_part);
    const uint32_t runend_pos = SelectRunends(store, rank);
    uint32_t pos = runend_pos;
    uint64_t slot_value = GetSlot(store, pos);
    // TODO: Faster implementation via binary search?
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


inline bool Steroids::PointQueryInfixStore(InfixStore &store, const uint64_t key, const uint32_t total_implicit) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t elem_count = store.GetElemCount();
    if (elem_count == (size_grade ? scaled_sizes_[size_grade - 1] : infix_store_target_size))
        ResizeInfixStore(store, true, total_implicit);

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
    // TODO: Faster implementation via binary search?
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


inline void Steroids::ResizeInfixStore(InfixStore &store, const bool expand, const uint32_t total_implicit) {
    // TODO: Optimize further?
    uint32_t size_grade = store.GetSizeGrade();
    const uint32_t infix_count = store.GetElemCount();
    const uint32_t current_size = scaled_sizes_[size_grade];
    const int32_t delta = expand ? 1 : -1;
    size_grade += delta;
    store.SetSizeGrade(size_grade);
    const uint32_t next_size = scaled_sizes_[size_grade];

    uint64_t infix_list[infix_count];
    GetInfixList(store, infix_list);
    delete store.ptr;

    const uint32_t word_count = InfixStore::GetPtrWordCount(next_size, infix_size_);
    store.ptr = new uint64_t[word_count];
    LoadListToInfixStore(store, infix_list, infix_count, total_implicit, true);
}


inline void Steroids::ShrinkInfixStoreInfixSize(InfixStore &store, const uint32_t new_infix_size) {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t infix_count = store.GetElemCount();
    const uint32_t slot_count = scaled_sizes_[size_grade];

    InfixStore new_store(slot_count, new_infix_size);
    for (int32_t i = 0; i < slot_count; i++) {
        const uint64_t old_slot = GetSlot(store, i);
        const uint64_t new_slot = (old_slot >> (infix_size_ - new_infix_size))
                                | (infix_size_ - lowbit_pos(old_slot) > new_infix_size ? 1ULL : 0ULL);
        SetSlot(new_store, i, new_slot);
    }
    const uint64_t *old_runends = store.ptr;
    uint64_t *new_runends = new_store.ptr;
    for (int32_t bit_pos = 0; bit_pos < slot_count; bit_pos += 64) {
        const uint32_t move_amount = std::min(slot_count - bit_pos, 64U);
        new_runends[bit_pos / 64] |= old_runends[bit_pos / 64] & BITMASK(move_amount);
    }
    delete store.ptr;
    store.ptr = new_store.ptr;
}


inline void Steroids::LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                                           const uint32_t total_implicit, const bool zero_out) {
    const uint32_t scaled_len = (size_scalars_[0] * list_len) >> scale_shift;
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t total_size = scaled_sizes_[size_grade];
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    if (zero_out)
        store.Reset(total_size, infix_size_);
    store.SetElemCount(list_len);

    int32_t l[list_len + 1], r[list_len + 1], ind = 0;

    uint64_t *occupieds = store.ptr;
    uint64_t *runends = store.ptr + infix_store_target_size / 64;
    uint64_t old_implicit_part = list[0] >> infix_size_;
    l[0] = (size_scalars_[size_grade] * implicit_scalar * old_implicit_part) >> (scale_shift + scale_implicit_shift);
    r[0] = l[0];
    for (uint32_t i = 0; i < list_len; i++) {
        const uint64_t implicit_part = list[i] >> infix_size_;
        assert(implicit_part <= total_implicit);
        if (implicit_part != old_implicit_part) {
            ind++;
            l[ind] = std::max(r[ind - 1] + 1,
                              static_cast<int32_t>((size_scalars_[size_grade] * implicit_scalar * implicit_part) 
                                                            >> (scale_shift + scale_implicit_shift)));
            const uint64_t baseline = (size_scalars_[size_grade] * implicit_scalar * implicit_part) 
                                                >> (scale_shift + scale_implicit_shift);
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
            set_bitmap_bit(occupieds, implicit_part);
            const uint64_t explicit_part = list[write_head] & BITMASK(infix_size_);
            write_head++;

            SetSlot(store, j, explicit_part);
        }
        set_bitmap_bit(runends, r[i] - 1);
    }
}


inline Steroids::InfixStore Steroids::AllocateInfixStoreWithList(const uint64_t *list, const uint32_t list_len,
                                                                 const uint32_t total_implicit) {
    const uint32_t scaled_len = (list_len * size_scalars_[0]) >> scale_shift;
    uint32_t size_grade;
    for (size_grade = 0; size_grade < size_scalar_count && scaled_sizes_[size_grade] < scaled_len; size_grade++);
    std::cerr  << "allocating: scaled_len=" << scaled_len << " size_grade=" << size_grade << std::endl;
    InfixStore res(scaled_sizes_[size_grade], infix_size_, size_grade);
    LoadListToInfixStore(res, list, list_len, total_implicit);
    return res;
}


inline uint32_t Steroids::GetInfixList(const InfixStore &store, uint64_t *res) const {
    const uint32_t size_grade = store.GetSizeGrade();
    const uint32_t list_size = scaled_sizes_[size_grade];
    const uint64_t *occupieds = store.ptr;
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    uint64_t implicit_part = occupieds[0] & 1ULL ? 0 : NextOccupied(store, 0);
    uint32_t ind = 0;
    for (int32_t i = 0; i < list_size; i++) {
        const uint64_t explicit_part = GetSlot(store, i);
        if (explicit_part)
            res[ind++] = (implicit_part << infix_size_) | explicit_part;
        if (get_bitmap_bit(runends, i))
            implicit_part = NextOccupied(store, implicit_part);
    }
    return ind;
}

