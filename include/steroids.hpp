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

private:
    static const uint32_t infix_store_target_size = 512;
    static const uint32_t implicit_size = 9;
    static const uint32_t scale_shift = 15;
    static const uint32_t scale_implicit_shift = 15;
    static const uint32_t size_scalar_count = 50;

    struct InfixStore {
        static constexpr uint32_t elem_count_bit_count = 12;

        uint16_t size_grade_elem_count = 0;
        uint64_t *ptr = nullptr;

        InfixStore(const uint32_t slot_count, const uint32_t slot_size, const uint32_t size_grade=0): 
                                            size_grade_elem_count(size_grade << elem_count_bit_count) {
            const uint32_t word_count = GetPtrWordCount(slot_count, slot_size);
            ptr = new uint64_t[word_count];
            memset(ptr, 0, sizeof(uint64_t) * word_count);
        }
        InfixStore(uint64_t *ptr): size_grade_elem_count(0), ptr(ptr) {};
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
    };

    const uint32_t infix_size_;
    art::art<InfixStore> tree_;
    std::mt19937 rng_;
    std::string zero_padding;
    const float load_factor_ = 0.95;
    uint64_t size_scalars_[size_scalar_count], scaled_sizes_[size_scalar_count];
    uint64_t implicit_scalars_[infix_store_target_size / 2 + 1];

    inline void AddTreeKey(const char *key, const uint32_t key_len);
    inline void AddTreeKeySplitInfixStore(const char *key, const uint32_t key_len);
    inline void SetupScaleFactors();
    inline std::tuple<uint32_t, uint32_t> GetSharedIgnoreLengths(const char *key_1, const char *key_2,
                                                                 const uint32_t key_len);
    inline uint64_t ExtractPartialKey(const char *key, const uint32_t shared,
                                      const uint32_t ignore, const uint64_t msb);

    inline uint32_t RankOccupieds(const InfixStore &store, const uint32_t pos);
    inline uint32_t SelectRunends(const InfixStore &store, const uint32_t rank);
    inline int32_t NextRunend(const InfixStore &store, const uint32_t pos);
    inline int32_t NextOccupied(const InfixStore &store, const uint32_t pos);
    inline int32_t PreviousRunend(const InfixStore &store, const uint32_t pos);

    inline uint64_t GetSlot(const InfixStore &store, const uint32_t pos);
    inline void SetSlot(const InfixStore &store, const uint32_t pos, const uint64_t value);

    inline void ShiftBitmapRight(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftBitmapLeft(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftRunendsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftRunendsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);

    inline int32_t FindEmptySlotAfter(InfixStore &store, const uint32_t runend_pos);
    inline int32_t FindEmptySlotBefore(InfixStore &store, const uint32_t runend_pos);
    inline void InsertRawIntoInfixStore(InfixStore &store, const uint64_t key,
                                        const uint32_t total_implicit=infix_store_target_size);
    inline void LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                                     const uint32_t total_implicit=infix_store_target_size, const bool zero_out=false);
    inline InfixStore AllocateInfixStoreWithList(const uint64_t *list, const uint32_t list_len,
                                                 const uint32_t total_implicit=infix_store_target_size);
    inline uint32_t GetInfixList(InfixStore &store, uint64_t *res);
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
        std::cerr << "NOT YET BUCKO" << std::endl;
        //AddTreeKeySplitInfixStore(key, key_len);
        //return;
    }

    print_key(key, key_len, true);

    auto it = tree_.begin(key, key_len);
    std::string next_key = it.key();
    InfixStore &infix_store = it.ref();
    it--;
    std::string prev_key = it.key();

    next_key += zero_padding;
    prev_key += zero_padding;
    const uint32_t max_len = std::max(prev_key.size(), next_key.size());
    auto [shared, ignore] = GetSharedIgnoreLengths(prev_key.c_str(), next_key.c_str(), max_len);

    const uint32_t padded_key_len = std::max(max_len, key_len);
    char padded_key[padded_key_len];
    memcpy(padded_key, key, key_len);
    memset(padded_key + key_len, 0, padded_key_len - key_len);

    const uint64_t extraction = ExtractPartialKey(padded_key, shared, ignore, get_string_kth_bit(padded_key, shared));
    const uint64_t next_implicit = ExtractPartialKey(next_key.c_str(), shared, ignore, 1) >> infix_size_;
    const uint64_t prev_implicit = ExtractPartialKey(prev_key.c_str(), shared, ignore, 0) >> infix_size_;
    const uint32_t total_implicit = next_implicit - prev_implicit + 1;
    const uint64_t insertee = ((extraction | 1ULL) - (prev_implicit << infix_size_));
    InsertRawIntoInfixStore(infix_store, insertee, total_implicit);
}


inline void Steroids::AddTreeKey(const char *key, const uint32_t key_len) {
    std::cerr << "adding key to tree key=";
    print_key(key, key_len, true);

    InfixStore infix_store(scaled_sizes_[0], infix_size_);
    tree_.set(key, key_len, infix_store);
}


inline void Steroids::AddTreeKeySplitInfixStore(const char *key, const uint32_t key_len) {
    auto it = tree_.begin(key, key_len);
    const std::string next_key = it.key();
    InfixStore &infix_store = it.ref();
    it--;
    const std::string padded_next_key = next_key + zero_padding;
    const std::string padded_prev_key = it.key() + zero_padding;

    const uint32_t max_len = std::max(padded_prev_key.size(), padded_next_key.size());
    auto [shared, ignore] = GetSharedIgnoreLengths(padded_prev_key.c_str(), padded_next_key.c_str(), max_len);
    const uint32_t real_diff_pos = shared + ignore;

    const uint32_t padded_key_len = std::max(max_len, key_len);
    char padded_key[padded_key_len];
    memcpy(padded_key, key, key_len);
    memset(padded_key + key_len, 0, padded_key_len - key_len);
    uint64_t extraction = ExtractPartialKey(padded_key, shared, ignore, get_string_kth_bit(padded_key, shared));
    uint64_t prev_extraction = ExtractPartialKey(padded_prev_key.c_str(), shared, ignore, 0);
    uint64_t next_extraction = ExtractPartialKey(padded_next_key.c_str(), shared, ignore, 1);
    const uint64_t separator = (extraction | 1ULL) - (prev_extraction & (BITMASK(implicit_size) << infix_size_));

    uint64_t infix_list[(infix_store.size_grade_elem_count & BITMASK(InfixStore::elem_count_bit_count)) + 1];
    const uint32_t infix_count = GetInfixList(infix_store, infix_list);
    uint64_t separated_list[infix_count + 1];
    uint32_t separated_ind = 0;
    for (int32_t i = 0; i < infix_count; i++) {
        const bool cond = infix_list[i] - (infix_list[i] & (-infix_list[i])) < separator - 1;
        if (cond)
            separated_list[separated_ind++] = infix_list[i];
    }
    const uint32_t split_pos = separated_ind;
    for (int32_t i = 0; i < infix_count; i++) {
        const bool cond = infix_list[i] - (infix_list[i] & (-infix_list[i])) < separator - 1;
        if (!cond)
            separated_list[separated_ind++] = infix_list[i];
    }

    const uint32_t shared_word_byte = (shared / 64) * 8;
    auto [shared_lt, ignore_lt] = GetSharedIgnoreLengths(padded_prev_key.c_str() + shared_word_byte,
                                                         padded_key + shared_word_byte,
                                                         padded_key_len);
    shared_lt += shared_word_byte * 8;
    auto [shared_gt, ignore_gt] = GetSharedIgnoreLengths(padded_key + shared_word_byte,
                                                         padded_next_key.c_str() + shared_word_byte,
                                                         padded_key_len);
    shared_gt += shared_word_byte * 8;
    
    const int32_t shamt_lt = shared_lt + ignore_lt - shared - ignore;
    for (int32_t i = 0; i < split_pos; i++) {
        const uint64_t delta = prev_extraction & (BITMASK(shamt_lt) << (infix_size_ - shamt_lt));
        const int32_t infix_len = infix_size_ - lowbit_pos(separated_list[i]);
        assert(infix_len - shamt_lt > 0);
        separated_list[i] -= delta;
        separated_list[i] <<= shamt_lt;
    }

    const int32_t shamt_gt = shared_gt + ignore_gt - shared - ignore;
    for (int32_t i = split_pos; i < infix_count; i++) {
        const int32_t infix_len = infix_size_ - lowbit_pos(separated_list[i]);
        assert(infix_len - shamt_gt > 0);
        separated_list[i] += prev_extraction & (BITMASK(implicit_size) << infix_size_);
        separated_list[i] -= extraction & (BITMASK(implicit_size) << infix_size_);
        separated_list[i] -= extraction & (BITMASK(shamt_gt) << (infix_size_ - shamt_gt));
        separated_list[i] <<= shamt_gt;
    }

    extraction = ExtractPartialKey(padded_key, shared_lt, ignore_lt, get_string_kth_bit(padded_key, shared_lt));
    prev_extraction = ExtractPartialKey(padded_prev_key.c_str(), shared_lt, ignore_lt, 0);
    const uint32_t total_implicit_lt = (extraction >> infix_size_) - (prev_extraction >> infix_size_) + 1;

    extraction = ExtractPartialKey(padded_key, shared_gt, ignore_gt, get_string_kth_bit(padded_key, shared_gt));
    next_extraction = ExtractPartialKey(padded_next_key.c_str(), shared_gt, ignore_gt, 1);
    const uint32_t total_implicit_gt = (next_extraction >> infix_size_) - (extraction >> infix_size_) + 1;

    InfixStore store_lt = AllocateInfixStoreWithList(separated_list, split_pos, total_implicit_lt);
    InfixStore store_gt = AllocateInfixStoreWithList(separated_list + split_pos, infix_count - split_pos, total_implicit_gt);
    tree_.set(next_key.c_str(), next_key.size(), store_gt);
    tree_.set(key, key_len, store_lt);
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
inline uint32_t Steroids::RankOccupieds(const InfixStore &store, const uint32_t pos) {
    const uint64_t *occupieds = store.ptr;
    uint32_t res = 0;
    for (int32_t i = 0; i < pos / 64; i++)
        res += __builtin_popcountll(occupieds[i]);
    return res + bit_rank(occupieds[pos / 64], pos % 64);
}


__attribute__((always_inline))
inline uint32_t Steroids::SelectRunends(const InfixStore &store, const uint32_t rank) {
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
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
inline int32_t Steroids::NextRunend(const InfixStore &store, const uint32_t pos) {
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint32_t runends_size = scaled_sizes_[size_grade];
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(runends[res / 64] & (~BITMASK(res % 64 + 1)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < runends_size);
    return res;
}


inline int32_t Steroids::NextOccupied(const InfixStore &store, const uint32_t pos) {
    const uint64_t *occupieds = store.ptr;
    int32_t res = pos + 1, lb_pos;
    do {
        lb_pos = lowbit_pos(occupieds[res / 64] & (~BITMASK(res % 64)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < infix_store_target_size);
    return res;
}


__attribute__((always_inline))
inline int32_t Steroids::PreviousRunend(const InfixStore &store, const uint32_t pos) {
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
inline uint64_t Steroids::GetSlot(const InfixStore &store, const uint32_t pos) {
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint32_t bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + pos * infix_size_;
    const uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t value;
    memcpy(&value, ptr, sizeof(value));
    return (value >> bit_pos % 8) & BITMASK(infix_size_);
}


__attribute__((always_inline))
inline void Steroids::SetSlot(const InfixStore &store, const uint32_t pos, const uint64_t value) {
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint32_t bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + pos * infix_size_;
    uint8_t *ptr = ((uint8_t *) store.ptr) + bit_pos / 8;
    uint64_t stamp;
    memcpy(&stamp, ptr, sizeof(stamp));
    stamp &= ~(BITMASK(infix_size_) << (bit_pos % 8));
    stamp |= value << (bit_pos % 8);
    memcpy(ptr, &stamp, sizeof(stamp));
}


__attribute__((always_inline))
inline void Steroids::ShiftBitmapRight(uint64_t *ptr, const uint32_t l, const uint32_t r,
                                       const uint32_t shamt) {
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
    
    // Zero out shifted part
    r_src_bit_pos = l_src_bit_pos + shamt - 1;
    while (r_src_bit_pos >= l_src_bit_pos) {
        const int32_t offset = r_src_bit_pos % 64;
        const uint32_t erase_amount = std::min(r_src_bit_pos - l_src_bit_pos, offset) + 1;
        ptr[r_src_bit_pos / 64] &= ~(BITMASK(erase_amount) << (offset - erase_amount + 1));
        r_src_bit_pos -= erase_amount;
    }
}


__attribute__((always_inline))
inline void Steroids::ShiftBitmapLeft(uint64_t *ptr, const uint32_t l, const uint32_t r,
                                      const uint32_t shamt) {
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

    // Zero out shifted part
    l_src_bit_pos = r_src_bit_pos - shamt + 1;
    while (l_src_bit_pos <= r_src_bit_pos) {
        const int32_t offset = l_src_bit_pos % 64;
        const uint32_t erase_amount = std::min(r_src_bit_pos - l_src_bit_pos, 63 - offset) + 1;
        ptr[l_src_bit_pos / 64] &= ~(BITMASK(erase_amount) << offset);
        l_src_bit_pos += erase_amount;
    }
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
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint32_t l_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    ShiftBitmapRight(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
#endif
}


__attribute__((always_inline))
inline void Steroids::ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                     const uint32_t shamt) {
#ifdef NAIVE_SLOT_SHIFT
    for (int32_t i = l; i < r; i--)
        SetSlot(store, i - shamt, GetSlot(store, i));
#else
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint32_t l_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    ShiftBitmapLeft(store.ptr, l_bit_pos, r_bit_pos, shamt * infix_size_);
#endif
}


__attribute__((always_inline))
inline void Steroids::ShiftRunendsRight(const InfixStore &store, const uint32_t l, const uint32_t r, 
                                        const uint32_t shamt) {
    ShiftBitmapRight(store.ptr + infix_store_target_size / 64, l, r - 1, shamt);
}


__attribute__((always_inline))
inline void Steroids::ShiftRunendsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                       const uint32_t shamt) {
    ShiftBitmapLeft(store.ptr + infix_store_target_size / 64, l, r - 1, shamt);
}


__attribute__((always_inline))
inline int32_t Steroids::FindEmptySlotAfter(InfixStore &store, const uint32_t runend_pos) {
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    int32_t current_pos = runend_pos;
    while (current_pos < scaled_sizes_[size_grade] && GetSlot(store, current_pos + 1)) {
        current_pos = NextRunend(store, current_pos);
    }
    return current_pos + 1;
}


__attribute__((always_inline))
inline int32_t Steroids::FindEmptySlotBefore(InfixStore &store, const uint32_t runend_pos) {
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
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint32_t elem_count = store.size_grade_elem_count & BITMASK(InfixStore::elem_count_bit_count);
    if (elem_count == (size_grade ? scaled_sizes_[size_grade - 1] : infix_store_target_size)) {
        std::cerr << "SHOULD EXPAND" << std::endl;
    }

    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    std::cerr << "INSERTING RAW implicit_part=" << implicit_part << " explicit_part=" << explicit_part << std::endl;
    std::cerr << "size_grade=" << size_grade << " size_scalar=" << size_scalars_[size_grade] << std::endl;
    std::cerr << "total_implicit=" << total_implicit << std::endl;

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

        int32_t l = std::max(PreviousRunend(store, runend_pos) + 1, previous_empty);
        int32_t r = runend_pos + 1;
        int32_t mid;
        while (r - l > 1) {
            mid = (l + r) / 2;
            uint64_t range_endpoint = GetSlot(store, mid);
            range_endpoint |= (range_endpoint - 1);
            if (range_endpoint <= explicit_part)
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
    store.size_grade_elem_count++;
}


inline void Steroids::LoadListToInfixStore(InfixStore &store, const uint64_t *list, const uint32_t list_len,
                                           const uint32_t total_implicit, const bool zero_out) {
    const uint32_t scaled_len = (size_scalars_[0] * list_len) >> scale_shift;
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint32_t total_size = scaled_sizes_[size_grade];
    const uint64_t implicit_scalar = implicit_scalars_[total_implicit - infix_store_target_size / 2];

    if (zero_out)
        store.Reset(total_size, infix_size_);
    store.size_grade_elem_count &= ~BITMASK(InfixStore::elem_count_bit_count);
    store.size_grade_elem_count |= list_len;

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
            std::cerr << "@ind=" << ind << " l=" << l[ind] << " vs. baseline=" << baseline << " --- implicit_part=" << implicit_part << std::endl;
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


inline uint32_t Steroids::GetInfixList(InfixStore &store, uint64_t *res) {
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
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

