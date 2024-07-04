#pragma once

#include <cstdint>
#include <cstring>
#include <random>

#include "art.hpp"
#include "util.hpp"

static int cnt = 0;

class Steroids {
public:
    Steroids(const uint32_t infix_size, const uint32_t rng_seed,
             const float load_factor): infix_size_(infix_size), load_factor_(load_factor) {
        rng_.seed(rng_seed);
        SetupScaleFactors();
    }

    void Insert(const char *key);

private:
    static const uint32_t infix_store_target_size = 512;
    static const uint32_t infix_implicit_bit_count = 9;
    static const uint32_t scale_shift = 30;
    static const uint32_t scale_factor_count = 50;

    struct InfixStore {
        static constexpr uint32_t elem_count_bit_count = 12;

        uint16_t size_grade_elem_count = 0;
        uint64_t *ptr = nullptr;

        InfixStore(const uint32_t slot_count, const uint32_t slot_size): size_grade_elem_count(0) {
            ptr = new uint64_t[(slot_count * (slot_size + 1) + 63) / 64];
        }
        InfixStore(uint64_t *ptr): size_grade_elem_count(0), ptr(ptr) {};
        InfixStore() = default;
        InfixStore(const InfixStore &other) = default;
        InfixStore(InfixStore &&other) = default;
        ~InfixStore() {
            delete ptr;
        }
    };

    const uint32_t infix_size_;
    art::art<InfixStore> tree_;
    std::mt19937 rng_;
    const float load_factor_ = 0.95;
    uint64_t scale_factors_[scale_factor_count], scaled_sizes_[scale_factor_count];

    inline void AddTreeKey(const char *key);
    inline void SetupScaleFactors();
    inline uint64_t ScaleValue(const uint64_t value, const uint32_t grade);
    inline uint32_t RankOccupieds(const InfixStore &store, const uint32_t pos);
    inline uint32_t SelectRunends(const InfixStore &store, const uint32_t rank);
    inline int32_t NextRunend(const InfixStore &store, const uint32_t pos);
    inline int32_t PreviousRunend(const InfixStore &store, const uint32_t pos);
    inline uint64_t GetSlot(const InfixStore &store, const uint32_t pos);
    inline void SetSlot(const InfixStore &store, const uint32_t pos, const uint64_t value);
    inline void ShiftBitmapRight(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftBitmapLeft(uint64_t *ptr, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline void ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r, const uint32_t shamt);
    inline int32_t FindEmptySlotAfter(InfixStore &store, const uint32_t runend_pos);
    inline int32_t FindEmptySlotBefore(InfixStore &store, const uint32_t runend_pos);
    inline void InsertToInfixStore(InfixStore &store, const uint64_t key);
};


inline void Steroids::SetupScaleFactors() {
    double pw = 1.0 / load_factor_;
    for (int i = 0; i < scale_factor_count; i++) {
        scale_factors_[i] = static_cast<uint64_t>(pw * (1ULL << scale_shift));
        scaled_sizes_[i] = ScaleValue(infix_store_target_size, i);
        pw /= load_factor_;
    }
}


void Steroids::Insert(const char *key) {
    if (rng_() % infix_store_target_size == 0)
        AddTreeKey(key);
}


inline void Steroids::AddTreeKey(const char *key) {
    InfixStore infix_store(scaled_sizes_[0], infix_size_);
    tree_.set(key, infix_store);
}


inline uint64_t Steroids::ScaleValue(const uint64_t value, const uint32_t grade) {
    return value * scale_factors_[grade] >> scale_shift;
}


__attribute__((always_inline))
inline uint32_t Steroids::RankOccupieds(const InfixStore &store, const uint32_t pos) {
    const uint64_t *occupieds = store.ptr;
    uint32_t res = 0;
    for (int i = 0; i < pos / 64; i++)
        res += __builtin_popcountll(occupieds[i]);
    return res + bit_rank(occupieds[pos / 64], pos % 64);
}


__attribute__((always_inline))
inline uint32_t Steroids::SelectRunends(const InfixStore &store, const uint32_t rank) {
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    uint32_t i = 0, old_total_set_bits = 0, total_set_bits = 0;
    while (total_set_bits < rank) {
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
    int32_t res = pos, lb_pos;
    do {
        lb_pos = lowbit_pos(runends[res / 64] & (~BITMASK(res % 64 + 1)));
        res += lb_pos - res % 64;
    } while (lb_pos == 64 && res < runends_size);
    return res;
}


__attribute__((always_inline))
inline int32_t Steroids::PreviousRunend(const InfixStore &store, const uint32_t pos) {
    const uint64_t *runends = store.ptr + infix_store_target_size / 64;
    int32_t res = pos, hb_pos;
    do {
        hb_pos = highbit_pos(runends[res / 64] & BITMASK(res % 64));
        res += (hb_pos == 64 ? -res % 64 - 1 : hb_pos - res % 64);
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
    const uint32_t l_src_bit_pos = l;
    uint32_t r_src_bit_pos = r;
    uint32_t dst_bit_pos = r_src_bit_pos + shamt;
    while (r_src_bit_pos >= l_src_bit_pos) {
        const uint32_t src_offset = r_src_bit_pos % 64;
        const uint32_t dst_offset = dst_bit_pos % 64;
        const uint32_t move_amount = std::min(r_src_bit_pos - l_src_bit_pos,
                                              std::min(src_offset, dst_offset)) + 1;
        const uint64_t move_mask = BITMASK(move_amount);
        const uint64_t payload = (ptr[r_src_bit_pos / 64] >> (src_offset - move_amount + 1)) & move_mask;

        ptr[dst_bit_pos / 64] &= ~(move_mask << (dst_offset - move_amount + 1));
        ptr[dst_bit_pos / 64] |= payload << (dst_offset - move_amount + 1);

        r_src_bit_pos -= move_amount;
        dst_bit_pos -= move_amount;
    }
}


__attribute__((always_inline))
inline void Steroids::ShiftSlotsRight(const InfixStore &store, const uint32_t l, const uint32_t r,
                                      const uint32_t shamt) {
#ifdef NAIVE_SHIFT
    for (int i = r - 1; i >= l; i--)
        SetSlot(store, i + shamt, GetSlot(store, i));
#else
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint32_t l_src_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    uint32_t r_src_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    uint32_t dst_bit_pos = r_src_bit_pos + shamt * infix_size_;
    while (r_src_bit_pos >= l_src_bit_pos) {
        const uint32_t src_offset = r_src_bit_pos % 64;
        const uint32_t dst_offset = dst_bit_pos % 64;
        const uint32_t move_amount = std::min(r_src_bit_pos - l_src_bit_pos,
                                              std::min(src_offset, dst_offset)) + 1;
        const uint64_t move_mask = BITMASK(move_amount);
        const uint64_t payload = (store.ptr[r_src_bit_pos / 64] >> (src_offset - move_amount + 1)) & move_mask;

        store.ptr[dst_bit_pos / 64] &= ~(move_mask << (dst_offset - move_amount + 1));
        store.ptr[dst_bit_pos / 64] |= payload << (dst_offset - move_amount + 1);

        r_src_bit_pos -= move_amount;
        dst_bit_pos -= move_amount;
    }
#endif
}


__attribute__((always_inline))
inline void Steroids::ShiftSlotsLeft(const InfixStore &store, const uint32_t l, const uint32_t r,
                                     const uint32_t shamt) {
#ifdef NAIVE_SHIFT
    for (int i = l; i < r; i--)
        SetSlot(store, i - shamt, GetSlot(store, i));
#else
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    uint32_t l_src_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + l * infix_size_;
    const uint32_t r_src_bit_pos = infix_store_target_size + scaled_sizes_[size_grade] + r * infix_size_ - 1;
    uint32_t dst_bit_pos = l_src_bit_pos - shamt * infix_size_;
    while (l_src_bit_pos <= r_src_bit_pos) {
        const uint32_t src_offset = l_src_bit_pos % 64;
        const uint32_t dst_offset = dst_bit_pos % 64;
        const uint32_t move_amount = std::min(r_src_bit_pos - l_src_bit_pos,
                                              63 - std::max(src_offset, dst_offset)) + 1;
        const uint64_t move_mask = BITMASK(move_amount);
        const uint64_t payload = (store.ptr[l_src_bit_pos / 64] >> src_offset) & move_mask;

        store.ptr[dst_bit_pos / 64] &= ~(move_mask << dst_offset);
        store.ptr[dst_bit_pos / 64] |= payload << dst_offset;

        l_src_bit_pos += move_amount;
        dst_bit_pos += move_amount;
    }
#endif
}


__attribute__((always_inline))
inline int32_t Steroids::FindEmptySlotAfter(InfixStore &store, const uint32_t runend_pos) {
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    int32_t current_pos;
    do {
        current_pos = NextRunend(store, runend_pos);
    } while (current_pos < scaled_sizes_[size_grade] && GetSlot(store, current_pos + 1));
    return current_pos + 1;
}


__attribute__((always_inline))
inline int32_t Steroids::FindEmptySlotBefore(InfixStore &store, const uint32_t runend_pos) {
    int32_t current_pos = runend_pos, previous_pos;
    do {
        previous_pos = current_pos;
        current_pos = PreviousRunend(store, runend_pos);
    } while (current_pos >= 0 && GetSlot(store, current_pos + 1));
    if (current_pos < 0)
        return current_pos;

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


inline void Steroids::InsertToInfixStore(InfixStore &store, const uint64_t key) {
    const uint32_t size_grade = store.size_grade_elem_count >> InfixStore::elem_count_bit_count;
    const uint64_t implicit_part = key >> infix_size_;
    const uint64_t explicit_part = key & BITMASK(infix_size_);

    uint64_t *occupieds = store.ptr;
    uint64_t *runends = store.ptr + infix_store_target_size / 64;

    const uint32_t mapped_pos = (implicit_part * scale_factors_[size_grade]) >> scale_shift;
    const uint32_t key_rank = RankOccupieds(store, implicit_part);
    const bool is_occupied = (occupieds[implicit_part / 64] >> (implicit_part % 64)) & 1ULL;
    uint32_t runend_pos = SelectRunends(store, key_rank);
    if (is_occupied) {
    }
    else {
        if (GetSlot(store, mapped_pos) == 0) {
            SetSlot(store, mapped_pos, explicit_part);
            runends[mapped_pos / 64] |= (1ULL << (mapped_pos % 64));
        }
        else if (key_rank == 0) {
        }
        else {
        }
    }
    occupieds[implicit_part / 64] |= 1ULL << (implicit_part % 64);
    store.size_grade_elem_count++;
}


