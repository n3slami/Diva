#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <endian.h>
#include <iomanip>
#include <random>
#include <tuple>

#include "art.hpp"
#include "art/tree_it.hpp"
#include "util.hpp"

static void print_key(const char *key, const uint32_t key_len, const bool binary=false) {
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

    void Insert(const char *key, const uint32_t key_len);
    bool RangeQuery(const char *l_key, const uint32_t l_key_len, const char *r_key, const uint32_t r_key_len);
    bool PointQuery(const char *key, const uint32_t key_len);
    void ShrinkInfixSize(const uint32_t new_infix_size);

private:
    static const uint32_t infix_store_target_size = 512;
    static const uint32_t implicit_size = 9;
    static const uint32_t scale_shift = 15;
    static const uint32_t scale_implicit_shift = 15;
    static const uint32_t size_scalar_count = 50;

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

    inline void AddTreeKey(const char *key, const uint32_t key_len);
    inline void InsertSplitInfixStore(const char *key, const uint32_t key_len);
    inline void SetupScaleFactors();
    inline std::tuple<uint32_t, uint32_t> GetSharedIgnoreLengths(const char *key_1, const char *key_2,
                                                                 const uint32_t key_len);
    inline uint64_t ExtractPartialKey(const char *key, const uint32_t shared,
                                      const uint32_t ignore, const uint64_t msb);

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
                                / static_cast<double>(i + infix_store_target_size / 2);
        implicit_scalars_[i] = static_cast<uint64_t>(ratio * (1ULL << scale_implicit_shift));
    }
    implicit_scalars_[infix_store_target_size / 2] = 1ULL << scale_implicit_shift;
}


void Steroids::Insert(const char *key, const uint32_t key_len) {
    if (rng_() % infix_store_target_size == 0) {
        InsertSplitInfixStore(key, key_len);
        return;
    }

    auto it = tree_.begin(key, key_len);
    std::string next_key, prev_key;
    next_key = it.key();
    if (key_len == next_key.size() && memcmp(key, next_key.c_str(), key_len) == 0) {
        prev_key = next_key;
        it++;
        next_key = it.key();
        it--;
    }
    else {
        it--;
        prev_key = it.key();
    }
    InfixStore &infix_store = it.ref();

    next_key += zero_padding;
    prev_key += zero_padding;
    const uint32_t max_len = std::max(prev_key.size(), next_key.size());
    auto [shared, ignore] = GetSharedIgnoreLengths(prev_key.c_str(), next_key.c_str(), max_len);

    const uint32_t padded_key_len = std::max(max_len, key_len);
    char padded_key[padded_key_len];
    memcpy(padded_key, key, std::min(padded_key_len, key_len));
    if (key_len < padded_key_len)
        memset(padded_key + key_len, 0, padded_key_len - key_len);

    const uint64_t extraction = ExtractPartialKey(padded_key, shared, ignore, get_string_kth_bit(padded_key, shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key.c_str(), shared, ignore, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key.c_str(), shared, ignore, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t insertee = ((extraction | 1ULL) - (prev_implicit << infix_size_));
    InsertRawIntoInfixStore(infix_store, insertee, total_implicit);
}


bool Steroids::RangeQuery(const char *l_key, const uint32_t l_key_len, const char *r_key, const uint32_t r_key_len) {
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
    char l_padded_key[padded_key_len], r_padded_key[padded_key_len];
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


bool Steroids::PointQuery(const char *key, const uint32_t key_len) {
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
    char padded_key[padded_key_len];
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


inline void Steroids::AddTreeKey(const char *key, const uint32_t key_len) {
    std::cerr << "adding key to tree key=";
    print_key(key, key_len, true);

    InfixStore infix_store(scaled_sizes_[0], infix_size_);
    tree_.set(key, key_len, infix_store);
}


inline void Steroids::InsertSplitInfixStore(const char *key, const uint32_t key_len) {
    auto it = tree_.begin(key, key_len);
    std::string next_key, prev_key;
    next_key = it.key();
    std::cerr << "WAAAAAAAAAAH next_key=";
    print_key(next_key.c_str(), next_key.size(), true);
    if (key_len == next_key.size() && memcmp(key, next_key.c_str(), key_len) == 0) {
        std::cerr << "A" << std::endl;
        prev_key = next_key;
        it++;
        next_key = it.key();
        it--;
    }
    else {
        std::cerr << "B" << std::endl;
        it--;
        prev_key = it.key();
    }
    const std::string padded_next_key = next_key + zero_padding;
    const std::string padded_prev_key = prev_key + zero_padding;
    InfixStore &infix_store = it.ref();

    const uint32_t max_len = std::max(padded_prev_key.size(), padded_next_key.size());
    auto [shared, ignore] = GetSharedIgnoreLengths(padded_prev_key.c_str(), padded_next_key.c_str(), max_len);
    std::cerr << "shared=" << shared << " ignore=" << ignore << std::endl;
    const uint32_t real_diff_pos = shared + ignore;

    const uint32_t padded_key_len = std::max(max_len, key_len);
    char padded_key[padded_key_len];
    memcpy(padded_key, key, std::min(padded_key_len, key_len));
    if (key_len < padded_key_len)
        memset(padded_key + key_len, 0, padded_key_len - key_len);
    uint64_t extraction = ExtractPartialKey(padded_key, shared, ignore, get_string_kth_bit(padded_key, shared));
    uint64_t prev_extraction = ExtractPartialKey(padded_prev_key.c_str(), shared, ignore, 0);
    uint64_t next_extraction = ExtractPartialKey(padded_next_key.c_str(), shared, ignore, 1);
    const uint64_t separator = (extraction | 1ULL) - (prev_extraction & (BITMASK(implicit_size) << infix_size_));

    uint64_t infix_list[infix_store.GetElemCount() + 1];
    const uint32_t infix_count = GetInfixList(infix_store, infix_list);
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
    std::cerr << "ZEEEEEEEEROOOOOOOOOOOOOOOO zero_pos=" << zero_pos << std::endl;
    if (zero_pos != -1) {
        padded_key[zero_pos / 8] &= ~BITMASK(7 - zero_pos % 8);
        memset(padded_key + zero_pos / 8 + 1, 0, padded_key_len - zero_pos / 8 - 1);
    }

    const uint32_t shared_word_byte = (shared / 64) * 8;
    auto [shared_lt, ignore_lt] = GetSharedIgnoreLengths(padded_prev_key.c_str() + shared_word_byte,
                                                         padded_key + shared_word_byte,
                                                         padded_key_len);
    shared_lt += shared_word_byte * 8;
    std::cerr << "shared_lt=" << shared_lt << " ignore_lt=" << ignore_lt << std::endl;
    auto [shared_gt, ignore_gt] = GetSharedIgnoreLengths(padded_key + shared_word_byte,
                                                         padded_next_key.c_str() + shared_word_byte,
                                                         padded_key_len);
    shared_gt += shared_word_byte * 8;
    std::cerr << "shared_gt=" << shared_gt << " ignore_gt=" << ignore_gt << std::endl;
    
    const int32_t shamt_lt = shared_lt + ignore_lt - shared - ignore;
    std::cerr << "shared_lt=" << shared_lt << " ignore_lt=" << ignore_lt << " --- shamt_lt=" << shamt_lt << std::endl;
    for (int32_t i = 0; i < split_pos; i++) {
        const uint64_t delta = prev_extraction & (BITMASK(shamt_lt) << (infix_size_ - shamt_lt));
        const int32_t infix_len = infix_size_ - lowbit_pos(infix_list[i]);
        assert(infix_len - shamt_lt > 0);
        infix_list[i] -= delta;
        infix_list[i] <<= shamt_lt;
    }

    const int32_t shamt_gt = shared_gt + ignore_gt - shared - ignore;
    std::cerr << "shared_gt=" << shared_gt << " ignore_gt=" << ignore_gt << " --- shamt_gt=" << shamt_gt << std::endl;
    for (int32_t i = split_pos; i < infix_count; i++) {
        const int32_t infix_len = infix_size_ - lowbit_pos(infix_list[i]);
        assert(infix_len - shamt_gt > 0);
        infix_list[i] += prev_extraction & (BITMASK(implicit_size) << infix_size_);
        infix_list[i] -= extraction & (BITMASK(implicit_size) << infix_size_);
        infix_list[i] -= extraction & (BITMASK(shamt_gt) << (infix_size_ - shamt_gt));
        infix_list[i] <<= shamt_gt;
    }

    extraction = ExtractPartialKey(padded_key, shared_lt, ignore_lt, get_string_kth_bit(padded_key, shared_lt));
    prev_extraction = ExtractPartialKey(padded_prev_key.c_str(), shared_lt, ignore_lt, 0);
    const uint32_t total_implicit_lt = (extraction >> infix_size_) - (prev_extraction >> infix_size_) + 1;

    extraction = ExtractPartialKey(padded_key, shared_gt, ignore_gt, get_string_kth_bit(padded_key, shared_gt));
    next_extraction = ExtractPartialKey(padded_next_key.c_str(), shared_gt, ignore_gt, 1);
    const uint32_t total_implicit_gt = (next_extraction >> infix_size_) - (extraction >> infix_size_) + 1;

    InfixStore store_lt = AllocateInfixStoreWithList(infix_list,
                                                     split_pos,
                                                     total_implicit_lt);
    store_lt.SetInvalidBits(infix_store.GetInvalidBits());
    store_lt.SetPartialKey(infix_store.IsPartialKey());
    InfixStore store_gt = AllocateInfixStoreWithList(infix_list + split_pos + (zero_pos != -1),
                                                     infix_count - split_pos - (zero_pos != -1),
                                                     total_implicit_gt);
    std::cerr << "SETTING TREE prev_key=";
    print_key(prev_key.c_str(), prev_key.size());
    tree_.set(prev_key.c_str(), prev_key.size(), store_lt);
    std::cerr << "SETTING TREE padded_key=";
    print_key(padded_key, (zero_pos + 7) / 8);
    if (zero_pos != -1) {
        InsertRawIntoInfixStore(store_gt, (extraction & BITMASK(infix_size_)) | 1, total_implicit_gt);
        store_gt.SetInvalidBits(7 - zero_pos % 8);
        store_gt.SetPartialKey(true);
        tree_.set(padded_key, (zero_pos + 7) / 8, store_gt);
    }
    else
        tree_.set(padded_key, key_len, store_gt);
}


inline std::tuple<uint32_t, uint32_t> 
Steroids::GetSharedIgnoreLengths(const char *key_1, const char *key_2, const uint32_t key_len) {
    const uint64_t *ptr_1 = reinterpret_cast<const uint64_t *>(key_1);
    const uint64_t *ptr_2 = reinterpret_cast<const uint64_t *>(key_2);
    uint32_t share = 0, ignore = 0, implicit = 0;

    uint32_t ind = 0, delta;
    do {
        delta = __builtin_ia32_lzcnt_u64(__bswap_64(ptr_1[ind] ^ ptr_2[ind]));
        share += delta;
        ind++;
    } while (delta == 64);

    ind--;
    do {
        const uint32_t offset = (ind > share / 64 ? 0 : share % 64 + 1);
        delta = __builtin_ia32_lzcnt_u64(__bswap_64((~ptr_1[ind]) | ptr_2[ind]) & BITMASK(64 - offset));
        ignore += delta - offset;
        ind++;
    } while (delta == 64);

    return {share, ignore};
}


void Steroids::ShrinkInfixSize(const uint32_t new_infix_size) {
    auto it = tree_.begin();
    do {
        InfixStore &store = it.ref();
        ShrinkInfixStoreInfixSize(store, new_infix_size);
        it++;
    } while (it != tree_.end());

    infix_size_ = new_infix_size;
}


__attribute__((always_inline))
inline uint64_t Steroids::ExtractPartialKey(const char *key, const uint32_t shared,
                                            const uint32_t ignore, const uint64_t msb) {
    const uint32_t real_diff_pos = shared + ignore;
    uint64_t res;
    memcpy(&res, key + real_diff_pos / 8, sizeof(res));
    res = __bswap_64(res) >> (63 - (implicit_size - 1) - infix_size_ - real_diff_pos % 8);
    res &= BITMASK(implicit_size - 1 + infix_size_);
    res |= msb << (implicit_size - 1 + infix_size_);
    print_key(key, 16, true);
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
        const uint32_t runend_pos = SelectRunends(store, key_rank);
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
            if (range_l < explicit_part)
                l = mid;
            else 
                r = mid;
        }
        if (next_empty < scaled_sizes_[size_grade]) {
            ShiftSlotsRight(store, r, next_empty, 1);
            ShiftRunendsRight(store, runend_pos, next_empty, 1);
            SetSlot(store, r, explicit_part);
        }
        else {
            const int32_t corner_case_shift = ((r == runend_pos + 1) & (GetSlot(store, l) > explicit_part));
            ShiftSlotsLeft(store, previous_empty + 1, r - corner_case_shift, 1);
            ShiftRunendsLeft(store, previous_empty + 1, r - 1, 1);
            SetSlot(store, r - 1 - corner_case_shift, explicit_part);
        }
    }
    else {
        const uint32_t runend_pos = SelectRunends(store, key_rank - 1);
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

